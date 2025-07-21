#include <stdio.h>
#include <complex.h>

int is_in_mandelbrot(double real, double imag, int max_iters) {
    double z_re = 0.0, z_im = 0.0;
    for (int i = 0; i < max_iters; ++i) {
        double z_re2 = z_re * z_re - z_im * z_im + real;
        z_im = 2.0 * z_re * z_im + imag;
        if (z_re2 + z_im * z_im > 4.0) return 0;
    }
    return 1;
}

int main() {
    int width = 80, height = 25;
    for(int y = 0; y < height; ++y) {
        for(int x = 0; x < width; ++x) {
            double real = -2.0 + (4.0 * (double)x / (width-1));
            double imag = -1.5 + (3.0 * (double)y / (height-1));
            if(is_in_mandelbrot(real, imag, 100)) putchar('@');
            else putchar('.');
        }
        putchar('\n');
    }
    return 0;
}
