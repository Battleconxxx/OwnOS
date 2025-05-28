#include <kernel/stdio.h>
#include <kernel/tty.h>
#include <kernel/memory.h>
#include <kernel/multiboot.h>
#include <kernel/interrupts.h>
#include <kernel/fs.h>
#include <kernel/gdt.h>


void kernel_main(uint32_t magic, multiboot_info_t* mbi) {
	gdt_install();
	terminal_initialize();
	printf("Hello, kernel World!123\n %s %d %x","Welcome to Sudhan Operating System", 123, 0x123);
	parse_memory_map(mbi);
	init_interrupts();
	init_memory();
	init_paging();
	ramfs_init();

	// Example: create and use files
    ramfs_create("hello.txt");
    ramfs_write("hello.txt", "Hello, RAMFS!", 13);
    
    char buffer[32];
    ramfs_read("hello.txt", buffer, 13);
    buffer[13] = '\0';
    printf("Read from RAMFS: %s\n", buffer);

	asm volatile ("sti");
	
	volatile uint32_t *ptr = (uint32_t*)0x5000000;  // 80 MB (outside identity map)
	uint32_t val = *ptr;  // Should cause a page fault interrupt
}
