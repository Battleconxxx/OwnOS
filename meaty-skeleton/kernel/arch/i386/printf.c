#include <stdarg.h>
#include "vga.h"

void printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    terminal_write("hello");

    for (size_t i = 0; fmt[i] != '\0'; i++) {

        if (fmt[i] == '%' && fmt[i + 1]) {
            i++;
            switch (fmt[i]) {
                case 's': {
                    const char* str = va_arg(args, const char*);
                    terminal_write(str);
                    break;
                }
                case 'd': {
                    int num = va_arg(args, int);
                    terminal_write_dec(num);
                    break;
                }
                case 'x': {
                    unsigned int num = va_arg(args, unsigned int);
                    terminal_write_hex(num);
                    break;
                }
                case 'c': {
                    char c = (char) va_arg(args, int);  // promoted to int
                    terminal_putchar(c);
                    break;
                }
                case '%': {
                    terminal_putchar('%');
                    break;
                }
                default:
                    terminal_putchar('%');
                    terminal_putchar(fmt[i]);
            }
        } else {
            terminal_putchar(fmt[i]);
        }
    }

    va_end(args);
}
