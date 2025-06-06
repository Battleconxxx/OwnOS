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
#include <kernel/user_entry.h>
#include <string.h>


#define USER_PROGRAM_START 0x101000
#define USER_PROGRAM_SIZE (30 * FRAME_SIZE)  // Map 3 pages (12KB)

typedef void (*entry_point_t)(void);

void map_user_program() {
    for (uint32_t addr = USER_PROGRAM_START; addr < USER_PROGRAM_START + USER_PROGRAM_SIZE; addr += FRAME_SIZE) {
        uint32_t frame = first_free_frame();
        if (frame == (uint32_t)-1) {
            printf("No free frame for user program at 0x%x\n", addr);
            return;
        }

        set_frame(frame);
        uint32_t phys = frame * FRAME_SIZE;
        //printf("Mapping VA 0x%x to PA 0x%x\n", addr, phys);

        map_page(addr, phys, PAGE_PRESENT | PAGE_RW | PAGE_USER);
    }
}


// void jump_to_user_mode(uint32_t entry, uint32_t user_stack_top) {
//     asm volatile (
//         "cli\n"
//         "mov $0x23, %%ax\n"     // User data segment selector (DPL=3)
//         "mov %%ax, %%ds\n"
//         "mov %%ax, %%es\n"
//         "mov %%ax, %%fs\n"
//         "mov %%ax, %%gs\n"

//         "pushl $0x23\n"         // SS = user data selector
//         "push %[user_stack]\n" // ESP = top of user stack

//         "pushl $0x200\n"        // EFLAGS with IF = 1
//         "pushl $0x1B\n"         // CS = user code selector
//         "push %[entry_point]\n" // EIP = entry point
//         "iret\n"
//         :
//         : [entry_point] "r" (entry),
//           [user_stack] "r" (SAFE_USER_STACK_TOP)
//         : "eax"
//     );
// }


void jump_to_user_mode(uint32_t entry, uint32_t user_stack_top) {
    asm volatile (
        "cli\n"
        "mov $0x23, %%eax\n"
        "mov %%eax, %%ds\n"
        "mov %%eax, %%es\n"
        "mov %%eax, %%fs\n"
        "mov %%eax, %%gs\n"
        "pushl $0x23\n"
        "pushl %1\n"
        "pushl $0x200\n"
        "pushl $0x1B\n"
        "pushl %0\n"
        "iret\n"
        : : "r" (entry), "r" (SAFE_USER_STACK_TOP) : "eax"
    );
}





#define PAGE_DIR_INDEX(x) (((x) >> 22) & 0x3FF)
#define PAGE_TABLE_INDEX(x) (((x) >> 12) & 0x3FF)

extern uint32_t* page_directory; // your page directory base


void map_user_stack(uint32_t stack_top, uint32_t num_pages) {
    for (uint32_t i = 0; i < num_pages; ++i) {
        uint32_t virt_addr = stack_top - (i + 1) * FRAME_SIZE;

        uint32_t frame = first_free_frame();
        if (frame == (uint32_t)-1) {
            printf("No free frame for user stack at 0x%x\n", virt_addr);
            return;
        }

        set_frame(frame);
        uint32_t phys = frame * FRAME_SIZE;
        map_page(virt_addr, phys, PAGE_PRESENT | PAGE_RW | PAGE_USER);
        //printf("Mapping User Stack VA 0x%x to PA 0x%x\n", virt_addr, phys);
    }
}


void kernel_main(uint32_t magic, multiboot_info_t* mbi) {
	gdt_install();
	terminal_initialize();
	printf("Hello, kernel World!123\n %s %d %x","Welcome to Sudhan Operating System", 123, 0x123);
	parse_memory_map(mbi);
    init_memory();
    printf("Memory mapped");
    mark_usable_frames();
    printf("Usable frames marked");
    init_interrupts();
    printf("Interrupts initiated");
	init_paging();
    printf("Paging initialied");
    init_kernel_heap_mapping();
	printf("kernel heap mapping done");
    //init_threading();
	ramfs_init();
    printf("RAMFS initialized");

    
    map_user_program();
    printf("Map user program done");
    map_user_stack(SAFE_USER_STACK_TOP, USER_STACK_PAGES);
    printf("User stack mapped");
    
    extern void user_mode_entry();
    printf("About to jump to 0x%x\n", (uint32_t)user_mode_entry);

    printf("kernel_stack: 0x%x, kernel_stack_top: 0x%x\n", (uint32_t)&kernel_stack, (uint32_t)&kernel_stack_top);

    jump_to_user_mode((uint32_t)user_mode_entry, SAFE_USER_STACK_TOP);
	asm volatile ("sti");
}