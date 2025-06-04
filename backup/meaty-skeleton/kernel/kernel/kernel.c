#include <kernel/stdio.h>
#include <kernel/tty.h>
#include <kernel/memory.h>
#include <kernel/multiboot.h>
#include <kernel/interrupts.h>
#include <kernel/fs.h>
#include <kernel/gdt.h>
#include <kernel/thread.h>

void thread_A() {
    while (1) {
        printf("A");
        for (volatile int i = 0; i < 1000; i++); // crude delay
    }
}

void thread_B() {
    while (1) {
        printf("B");
        for (volatile int i = 0; i < 1000; i++); // crude delay
    }
}


void kernel_main(uint32_t magic, multiboot_info_t* mbi) {
	gdt_install();
	terminal_initialize();
	printf("Hello, kernel World!123\n %s %d %x","Welcome to Sudhan Operating System", 123, 0x123);
	parse_memory_map(mbi);
	init_interrupts();
    init_threading();
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

    create_thread(thread_A);
    //create_thread(thread_B);

	asm volatile ("sti");

}
