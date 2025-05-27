#include <kernel/stdio.h>
#include <kernel/tty.h>
#include <kernel/memory.h>


void kernel_main(multiboot_info_t* mbi) {
	terminal_initialize();
	printf("Hello, kernel World!123\n %s %c %d","Welcome to Sudhan Operating System ", ' ', 123);
	parse_memory_map(mbi);
}
