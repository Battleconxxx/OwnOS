#include <kernel/stdio.h>
#include <kernel/tty.h>
#include <kernel/memory.h>
#include <kernel/multiboot.h>
#include <kernel/interrupts.h>


void kernel_main(uint32_t magic, multiboot_info_t* mbi) {
	terminal_initialize();
	printf("Hello, kernel World!123\n %s %d %x","Welcome to Sudhan Operating System", 123, 0x123);
	parse_memory_map(mbi);
	init_interrupts();
	init_memory();
	init_paging();
	asm volatile ("sti");
	
	volatile uint32_t *ptr = (uint32_t*)0x5000000;  // 80 MB (outside identity map)
	uint32_t val = *ptr;  // Should cause a page fault interrupt
}
