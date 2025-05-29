#include <kernel/stdio.h>
#include <kernel/tty.h>
#include <kernel/memory.h>
#include <kernel/multiboot.h>
#include <kernel/interrupts.h>
#include <kernel/fs.h>
#include <kernel/gdt.h>
#include <kernel/thread.h>
#include <kernel/elf_loader.h>
#include <kernel/hello_elf.h>
#include <kernel/user.h>


#define USER_PROGRAM_START 0x101000
#define USER_PROGRAM_SIZE (20 * FRAME_SIZE)  // Map 3 pages (12KB)

#define USER_STACK_BASE 0x00700000  // 7 MB mark, must be mapped
#define USER_STACK_SIZE 0x2000      // 8 KB stack


extern uint8_t _binary_hello_elf_start[];
extern uint8_t _binary_hello_elf_end[];

typedef void (*entry_point_t)(void);

void map_user_program() {
    for (uint32_t addr = USER_PROGRAM_START; addr < USER_PROGRAM_START + USER_PROGRAM_SIZE; addr += FRAME_SIZE) {
        uint32_t frame = first_free_frame();
        if (frame == (uint32_t)-1) {
            printf("No free frame for user program page at %x!\n", addr);
            return;
        }
        set_frame(frame);
        uint32_t phys = frame * FRAME_SIZE;
        map_page(addr, phys, PAGE_PRESENT | PAGE_RW | PAGE_USER);
    }
}

// Call this somewhere before jumping to ELF
void map_user_stack() {
    for (uint32_t addr = USER_STACK_BASE; addr < USER_STACK_BASE + USER_STACK_SIZE; addr += FRAME_SIZE) {
        uint32_t frame = first_free_frame();
        if (frame == (uint32_t)-1) {
            printf("map_user_stack: no free frames!\n");
            return;
        }
        set_frame(frame);
        uint32_t phys = frame * FRAME_SIZE;
        map_page(addr, phys, PAGE_PRESENT | PAGE_RW | PAGE_USER);
    }
    
}

void jump_to_user_mode(uint32_t entry, uint32_t user_stack_top) {
    asm volatile (
        "cli\n"
        "mov $0x23, %%ax\n"     // User data segment selector (DPL=3)
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"

        "mov %[user_stack], %%eax\n"
        "pushl $0x23\n"         // User data segment selector
        "pushl %%eax\n"         // Stack pointer
        "pushf\n"               // Push EFLAGS
        "pushl $0x1B\n"         // User code segment selector (DPL=3)
        "push %[entry_point]\n" // Entry point of user code
        "iret\n"
        :
        : [entry_point] "r" (entry),
          [user_stack] "r" (user_stack_top)
        : "eax"
    );
}


void kernel_main(uint32_t magic, multiboot_info_t* mbi) {
	gdt_install();
	terminal_initialize();
	printf("Hello, kernel World!123\n %s %d %x","Welcome to Sudhan Operating System", 123, 0x123);
	parse_memory_map(mbi);
    init_memory();
    mark_usable_frames();
	init_paging();
    init_kernel_heap_mapping();
	init_interrupts();
    init_threading();
	ramfs_init();

    map_user_program();
    map_user_stack();

    extern void switch_to_user_mode();
    switch_to_user_mode(); // Will call user_mode_entry() in ring 3

	asm volatile ("sti");

}