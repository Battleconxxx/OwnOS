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

#define USER_STACK_BASE 0x00700000  // 7 MB mark, must be mapped
#define USER_STACK_SIZE 0x2000      // 8 KB stack

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

//         "mov %[user_stack], %%eax\n"
//         "pushl $0x23\n"         // User data segment selector
//         "pushl %%eax\n"         // Stack pointer
//         "pushf\n"               // Push EFLAGS
//         "pushl $0x1B\n"         // User code segment selector (DPL=3)
//         "push %[entry_point]\n" // Entry point of user code
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
        "mov $0x23, %%ax\n"     // User data segment selector (DPL=3)
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"

        "pushl $0x23\n"         // SS = user data selector
        "push %[user_stack]\n" // ESP = top of user stack

        "pushl $0x200\n"        // EFLAGS with IF = 1
        "pushl $0x1B\n"         // CS = user code selector
        "push %[entry_point]\n" // EIP = entry point
        "iret\n"
        :
        : [entry_point] "r" (entry),
          [user_stack] "r" (user_stack_top)
        : "memory"
    );
}



#define PAGE_DIR_INDEX(x) (((x) >> 22) & 0x3FF)
#define PAGE_TABLE_INDEX(x) (((x) >> 12) & 0x3FF)

extern uint32_t* page_directory; // your page directory base

void dump_page_table_for(uint32_t vaddr) {
    
    uint32_t pdi = PAGE_DIR_INDEX(vaddr);
    uint32_t pti = PAGE_TABLE_INDEX(vaddr);
    uint32_t* pd = (uint32_t*)0xFFFFF000;  // recursive mapping of the page directory
    uint32_t pd_entry = pd[pdi];
    printf("YOYOYOYOYOY");
    printf("Here");
    printf("pd_entry = 0x%x\n", pd_entry);
    if (!(pd_entry & 0x1)) {
        printf("Page directory entry not present for 0x%x\n", vaddr);
        return;
    } 

    uint32_t* page_table = (uint32_t*)(0xFFC00000 + (pdi * 0x1000));

    printf("PT[%x] = 0x%x\n", pti, page_table[pti]);

    uint32_t pt_entry = page_table[pti];
    printf("Here2");
    if (pt_entry & 0x1) {
        uint32_t phys_addr = (pt_entry & 0xFFFFF000) | (vaddr & 0xFFF);
        printf("Here3");
        printf("VA 0x%x => PA 0x%x (flags: 0x%x)\n", vaddr, phys_addr, pt_entry & 0xFFF);
    } else {
        printf("here4");
        printf("Page table entry not present for 0x%x\n", vaddr);
    }
}

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
    mark_usable_frames();
	init_paging();
    init_kernel_heap_mapping();
	init_interrupts();
    //init_threading();
	ramfs_init();

    
    map_user_program();
    map_user_stack(SAFE_USER_STACK_TOP, USER_STACK_PAGES);
    printf("User stack mapped");
    
    extern void user_mode_entry();
    printf("About to jump to 0x%x\n", (uint32_t)user_mode_entry);

    jump_to_user_mode((uint32_t)user_mode_entry, SAFE_USER_STACK_TOP);
	asm volatile ("sti");
}