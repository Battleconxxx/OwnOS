#include <stdio.h>

#include <kernel/tty.h>

void kernel_main(void) {
	terminal_initialize();
	printf("Hello, kernel World!\n");
	printf("Decimal: %d, Hex: %x, Char: %c, String: %s\n", 123, 123, 'A', "Hello");
}
