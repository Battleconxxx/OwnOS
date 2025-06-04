#include <kernel/stdio.h>

int puts(const char* string) {
	printf(string);
	return 0;
}
