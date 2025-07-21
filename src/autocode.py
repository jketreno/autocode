from phoenix.otel import register
from agents import Agent, OpenAIChatCompletionsModel, function_tool, Runner
from openai import AsyncOpenAI
import os
import subprocess
from typing import Optional
from pydantic import BaseModel

model_name = os.getenv("MODEL_NAME", "qwen2.5-32k")
openai_api_key = os.getenv("OPENAI_API_KEY", "not-used")
openai_base_url = os.getenv("OPENAI_BASE_URL", None)

generic_model = OpenAIChatCompletionsModel(
    model=model_name,
    openai_client=AsyncOpenAI(base_url=openai_base_url, api_key=openai_api_key),
)

coding_model = OpenAIChatCompletionsModel(
    model="qwen2.5-coder-32k",
    openai_client=AsyncOpenAI(base_url=openai_base_url, api_key=openai_api_key),
)

# configure the Phoenix tracer
tracer_endpoint = os.getenv('PHOENIX_COLLECTOR_ENDPOINT', None)
if tracer_endpoint:
  tracer_provider = register(
      endpoint=f"{tracer_endpoint}/v1/traces",
      project_name="AutoCode",  # Default is 'default'
      auto_instrument=True,  # Auto-instrument your app based on installed dependencies
  )


class BashCommandResult(BaseModel):
    stdout: str
    stderr: str
    return_code: int
    working_directory: str
    command: str
    success: bool


class BashShellTool:
    def __init__(self, working_directory: Optional[str] = None, timeout: int = 30):
        """
        Initialize the bash shell tool.

        Args:
            working_directory: Initial working directory for commands
            timeout: Maximum execution time for commands (seconds)
        """
        self.working_directory = working_directory or os.getcwd()
        self.timeout = timeout
        self.env = os.environ.copy()

    def execute_command(self, command: str) -> BashCommandResult:
        """
        Execute a bash command and return the result.

        Args:
            command: The bash command to execute

        Returns:
            Dictionary containing stdout, stderr, return_code, and working_directory
        """
        original_dir = self.working_directory
        try:
            # Change to working directory
            original_dir = os.getcwd()
            os.chdir(self.working_directory)

            # Execute command
            result = subprocess.run(
                command,
                shell=True,
                capture_output=True,
                text=True,
                timeout=self.timeout,
                env=self.env,
                cwd=self.working_directory,
            )

            # Update working directory if command changed it
            self.working_directory = os.getcwd()

            return BashCommandResult(
                stdout=result.stdout,
                stderr=result.stderr,
                return_code=result.returncode,
                working_directory=self.working_directory,
                command=command,
                success=result.returncode == 0,
            )

        except subprocess.TimeoutExpired:
            return BashCommandResult(
                stdout="",
                stderr="Command timed out after {self.timeout} seconds",
                return_code=-1,
                working_directory=self.working_directory,
                command=command,
                success=False,
            )
        except Exception as e:
            return BashCommandResult(
                stdout="",
                stderr=str(e),
                return_code=-1,
                working_directory=self.working_directory,
                command=command,
                success=False,
            )
        finally:
            try:
                os.chdir(original_dir)
            except Exception as e:
                print(
                    f"Failed to change back to original directory {original_dir}: {e}"
                )
                pass


# Example with security restrictions
class RestrictedBashShellTool(BashShellTool):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        # Commands that are not allowed
        self.blocked_commands = {
            "rm -rf /",
            "dd if=",
            "mkfs",
            "fdisk",
            "parted",
            "passwd",
            "sudo su",
            "su -",
            "chmod 777",
        }
        # Directories that cannot be accessed
        self.blocked_paths = {"/etc/passwd", "/etc/shadow", "/root"}

    def execute_command(self, command: str) -> BashCommandResult:
        # Check for blocked commands
        for blocked in self.blocked_commands:
            if blocked in command.lower():
                return BashCommandResult(
                    stdout="",
                    stderr="Command blocked for security: contains '{blocked}'",
                    return_code=-1,
                    working_directory=self.working_directory,
                    command=command,
                    success=False,
                )

        return super().execute_command(command)


# Global shell instance
shell_tool = RestrictedBashShellTool()


@function_tool
def bash_execute(command: str) -> BashCommandResult:
    """
    Execute bash commands in a persistent shell environment. The shell maintains state
    between commands (working directory, environment variables, etc.). Use this to run
    system commands, manage files, install packages, or perform any bash operations.

    Args:
        command: The bash command to execute. Can be any valid bash command including pipes, redirections, and complex operations.

    Returns:
        Formatted string containing command output and status
    """
    print(f"🤖 {command}")
    result = shell_tool.execute_command(command)
    print(result)
    return result


@function_tool
def create_file(filename: str, content: str) -> str:
    """
    Create a file with the specified content.

    Args:
        filename: The name of the file to create.
        content: The content to write into the file.

    Returns:
        A message indicating success or failure.
    """
    try:
        print(f"Creating file: {filename}")
        with open(filename, "w") as f:
            f.write(content)
        return f"File '{filename}' created successfully."
    except Exception as e:
        return f"Failed to create file '{filename}': {str(e)}"


coder = Agent(
    model=coding_model,
    name="AutoCode Agent",
    instructions="""
You are a C coding expert.

You can write file content to disk using the create_file tool.

You can compile and run programs by using the bash_execute tool.

If you create a file, do so in the current working directory. 

When you create a file, provide it ONLY the file content, no markdown or other 
formatting.

Respond with the name of the file you created, and what it is intended to do.
""",
    tools=[bash_execute, create_file],
)
coder_as_tool = coder.as_tool(
    tool_name="coder",
    tool_description="A coding agent that can write, compile, and run code.",
)

manager = Agent(
    model=generic_model,
    name="Shell Manager",
    instructions="""
You are bash expert software architect.

You are running in a Docker container in virtual environment.

Using tools, you can run shell commands, and interact with the file system.

If you need to write, compile, or debug a program, use the coder tool--tell it what you want to do, and it will write the code for you.

When you use the coder tool, ask it to verify the code it writes, and to check for errors.

Perform the actions requested by the user, or the coder tool, and return the results.

If you need to perform multiple actions, use the bash_execute tool to run additional
commands, create_file, and coder tools as needed.
""",
    tools=[bash_execute, create_file, coder_as_tool],
)


async def main():
    # Initialize the application state; just tracking if 'done' is set by the exit_chat tool.
    while True:
        try:
            user_input = input("> ").strip()
        except (EOFError, KeyboardInterrupt):
            print("\nInterrupted.")
            break

        # If the user input is empty, continue to the next iteration
        if not user_input:
            continue

        # Run with streaming response
        result = await Runner.run(manager, user_input)
        print(result.final_output)


if __name__ == "__main__":
    import asyncio

    asyncio.run(main())
