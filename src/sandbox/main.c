#include <ncurses.h>
#include <complex.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_ITER 1000

int mandelbrot(double x, double y) {
    double real = x;
    double imag = y;
    int iter;
    for (iter = 0; real * real + imag * real <= 4.0 && iter < MAX_ITER; ++iter) {
        double temp = real * real - imag * imag + x;
        imag = 2 * real * imag + y;
        real = temp;
    }
    return iter;
}

int main() {
    int n_rows, n_cols;
    double zoom_factor = 1.0;
    double center_x = -0.75, center_y = 0.0;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    getmaxyx(stdscr, n_rows, n_cols);

    while (1) {
        clear();

        for (int i = 0; i < n_rows; ++i) {
            for (int j = 0; j < n_cols; ++j) {
                double x = center_x + (double) j / n_cols * 2.0 - zoom_factor;
                double y = center_y - (double) i / n_rows * 2.0 + zoom_factor;

                int color = mandelbrot(x, y);
                if (color == MAX_ITER)
                    attron(COLOR_PAIR(1));
                else
                    attron(COLOR_PAIR(color % 8 + 2));
                mvaddch(i, j, '#');
                attroff(COLOR_PAIR(color % 8 + 2));
            }
        }

        refresh();

        int ch = getch();
        if (ch == 'q')
            break;
        else if (ch == '+' && zoom_factor > 0.1)
            zoom_factor *= 0.95;
        else if (ch == '-' && zoom_factor < 4.0)
            zoom_factor *= 1.05;
    }

    endwin();
    return 0;
}
