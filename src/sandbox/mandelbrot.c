#include <stdio.h>

#define WIDTH 80
#define HEIGHT 25
#define MAX_ITER 1000

typedef struct {
    double x, y;
} Complex;

Complex add(Complex a, Complex b) {
    return (Complex){a.x + b.x, a.y + b.y};
}

Complex multiply(Complex a, Complex b) {
    return (Complex){a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x};
}

int is_in_mandelbrot(Complex z, int max_iter) {
    Complex c = {0.0, 0.0};
    for (int iter = 0; iter < max_iter; iter++) {
        if ((z.x * z.x + z.y * z.y) > 2.0) {
            return iter;
        }
        z = add(multiply(z, z), c);
    }
    return max_iter;
}

int main() {
    for (int y = HEIGHT - 1; y >= 0; y--) {
        for (int x = 0; x < WIDTH; x++) {
            double zx = -2.5 + x * 3.5 / WIDTH;
            double zy = -1.25 + y * 2.5 / HEIGHT;
            Complex c = {zx, zy};
            int iter = is_in_mandelbrot(c, MAX_ITER);
            if (iter == MAX_ITER) {
                putchar(' ');
            } else {
                putchar("0123456789abcdef"[iter % 16]);
            }
        }
        putchar('\n');
    }

    return 0;
}
