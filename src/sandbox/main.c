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
    // Fix: should be imag * imag, not imag * real
    for (iter = 0; real * real + imag * imag <= 4.0 && iter < MAX_ITER; ++iter) {
        double temp = real * real - imag * imag + x;
        imag = 2 * real * imag + y;
        real = temp;
    }
    return iter;
}

int main() {
    int n_rows, n_cols;
    double zoom = 1.0;
    double center_x = -0.75, center_y = 0.0;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    
    // Initialize colors
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_BLACK, COLOR_BLACK);   // Set color
        init_pair(2, COLOR_RED, COLOR_BLACK);
        init_pair(3, COLOR_GREEN, COLOR_BLACK);
        init_pair(4, COLOR_YELLOW, COLOR_BLACK);
        init_pair(5, COLOR_BLUE, COLOR_BLACK);
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(7, COLOR_CYAN, COLOR_BLACK);
        init_pair(8, COLOR_WHITE, COLOR_BLACK);
        init_pair(9, COLOR_RED, COLOR_BLACK);
    }
    
    getmaxyx(stdscr, n_rows, n_cols);

    while (1) {
        clear();

        for (int i = 0; i < n_rows; ++i) {
            for (int j = 0; j < n_cols; ++j) {
                // Proper coordinate mapping with zoom
                double x = center_x + (j - n_cols/2.0) * (4.0 / zoom) / n_cols;
                double y = center_y + (i - n_rows/2.0) * (4.0 / zoom) / n_rows;

                int color = mandelbrot(x, y);
                if (color == MAX_ITER) {
                    if (has_colors()) attron(COLOR_PAIR(1));
                    mvaddch(i, j, ' ');  // Use space for set points
                } else {
                    if (has_colors()) attron(COLOR_PAIR(color % 7 + 2));
                    mvaddch(i, j, '#');
                }
                if (has_colors()) attroff(COLOR_PAIR(color % 7 + 2));
            }
        }

        // Display instructions
        mvprintw(0, 0, "Mandelbrot Set - Zoom: %.2f", zoom);
        mvprintw(1, 0, "Controls: +/- zoom, arrows move, q quit");

        refresh();

        int ch = getch();
        if (ch == 'q') {
            break;
        } else if (ch == '+' || ch == '=') {
            zoom *= 1.2;  // Zoom in
        } else if (ch == '-') {
            zoom /= 1.2;  // Zoom out
        } else if (ch == KEY_LEFT) {
            center_x -= 0.1 / zoom;
        } else if (ch == KEY_RIGHT) {
            center_x += 0.1 / zoom;
        } else if (ch == KEY_UP) {
            center_y -= 0.1 / zoom;
        } else if (ch == KEY_DOWN) {
            center_y += 0.1 / zoom;
        }
    }

    endwin();
    return 0;
}