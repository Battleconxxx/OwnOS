#include <stdint.h>
#include <kernel/memory.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

uint8_t memory_bitmap[MAX_FRAMES / 8];
memory_region_t usable_regions[MAX_MEMORY_REGIONS];
int usable_region_count = 0;

//paging stuff
uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));

//heap stuff for multithreading
static uint32_t heap_next = KERNEL_HEAP_START;
static uint32_t heap_end = KERNEL_HEAP_START + KERNEL_HEAP_SIZE;


uint8_t user_stack[USER_STACK_SIZE] __attribute__((aligned(16)));


void* kmalloc(size_t size) {
    // Align size to 4 bytes
    size = (size + 3) & ~3;

    if (heap_next + size > heap_end) {
        printf("kmalloc failed: out of heap memory (requested %u bytes)\n", size);
        return 0; // Or handle error
    }

    void* ptr = (void*)heap_next;
    heap_next += size;

    return ptr;
}


void parse_memory_map(multiboot_info_t* mbi) {
    if (!(mbi->flags & (1 << 6))) {
        printf("No memory map from GRUB\n");
        return;
    }

    uint32_t mmap_end = mbi->mmap_addr + mbi->mmap_length;
    multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)mbi->mmap_addr;

    while ((uint32_t)mmap < mmap_end) {
        if (mmap->type == 1 && usable_region_count < MAX_MEMORY_REGIONS) {
            usable_regions[usable_region_count].base = mmap->addr;
            usable_regions[usable_region_count].length = mmap->len;
            usable_region_count++;

            printf("Usable memory map: base=0x%x, length=0x%x\n", (uint32_t)mmap->addr, (uint32_t)mmap->len);
        }
        mmap = (multiboot_memory_map_t*)((uint32_t)mmap + mmap->size + sizeof(uint32_t));
    }
}

void set_frame(uint32_t frame) {
    memory_bitmap[frame / 8] |= (1 << (frame % 8));
}

void clear_frame(uint32_t frame) {
    memory_bitmap[frame / 8] &= ~(1 << (frame % 8));
}

int test_frame(uint32_t frame) {
    return memory_bitmap[frame / 8] & (1 << (frame % 8));
}

uint32_t first_free_frame() {
    for (uint32_t i = 0; i < MAX_FRAMES / 8; i++) {
        if (memory_bitmap[i] != 0xFF) {
            for (int j = 0; j < 8; j++) {
                if (!(memory_bitmap[i] & (1 << j))) {
                    return i * 8 + j;
                }
            }
        }
    }
    return (uint32_t)-1; // No free frames
}

void init_memory() {
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        memory_bitmap[i] = 0;
    }
}

#define KERNEL_VIRTUAL_BASE 0xC0000000   // if you map kernel at high memory
#define KERNEL_PHYSICAL_BASE 0x00100000  // physical address where kernel is loaded



void init_paging() {
    // Clear page directory
    for (int i = 0; i < 1024; i++) page_directory[i] = 0;

    // Identity map first 16MB (0x00000000 to 0x00FFFFFF)
    static uint32_t identity_tables[4][1024] __attribute__((aligned(4096)));

    for (int table = 0; table < 4; table++) {
        for (int i = 0; i < 1024; i++) {
            identity_tables[table][i] = ((table * 0x400000) + (i * 0x1000)) | PAGE_PRESENT | PAGE_RW | PAGE_USER;
        }
        page_directory[table] = ((uint32_t)identity_tables[table]) | PAGE_PRESENT | PAGE_RW | PAGE_USER;
    }

    // Map kernel higher-half: 0xC0000000 - 0xC03FFFFF (maps 0x00100000 physical)
    static uint32_t kernel_table[1024] __attribute__((aligned(4096)));
    for (int i = 0; i < 1024; i++) {
        kernel_table[i] = (i * 0x1000 + 0x00100000) | PAGE_PRESENT | PAGE_RW;
    }
    page_directory[768] = ((uint32_t)kernel_table) | PAGE_PRESENT | PAGE_RW;

    //To find physical address of page directory
    uint32_t phys_addr_of_page_directory = (uint32_t)page_directory - KERNEL_VIRTUAL_BASE + KERNEL_PHYSICAL_BASE;
    page_directory[1023] = phys_addr_of_page_directory | PAGE_PRESENT | PAGE_RW | PAGE_USER;




    // Recursive mapping: last PDE maps to page directory itself
    //page_directory[1023] = ((uint32_t)page_directory) | PAGE_PRESENT | PAGE_RW;

    // Load page directory into CR3
    asm volatile ("mov %0, %%cr3" :: "r"(page_directory));

    // Enable paging by setting the PG bit in CR0
    uint32_t cr0;
    asm volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile ("mov %0, %%cr0" :: "r"(cr0));
}





// Helper to map a single 4KB page: assumes page directory & page tables exist
//extern uint32_t page_directory[1024]; // from memory.c
//new map_page
#define TEMP_MAP_VADDR 0xFFC00000  // Chosen virtual address for temp mapping
#define TEMP_MAP_INDEX (TEMP_MAP_VADDR >> 12) & 0x3FF
#define TEMP_MAP_PD_INDEX (TEMP_MAP_VADDR >> 22)

// Map a physical page to TEMP_MAP_VADDR and return pointer
void* temp_map_page(uint32_t phys_addr) {
    uint32_t pt_phys = page_directory[TEMP_MAP_PD_INDEX] & ~0xFFF;
    uint32_t* page_table = (uint32_t*)((pt_phys < 0x800000) ? (void*)pt_phys : temp_map_page(pt_phys));

    page_table[TEMP_MAP_INDEX] = phys_addr | PAGE_PRESENT | PAGE_RW;

    asm volatile("invlpg (%0)" :: "r"(TEMP_MAP_VADDR) : "memory");
    return (void*)TEMP_MAP_VADDR;
}

void map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    uint32_t pd_index = virtual_addr >> 22;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;

    uint32_t* page_table;

    if (!(page_directory[pd_index] & PAGE_PRESENT)) {
        uint32_t frame = first_free_frame();
        if (frame == (uint32_t)-1) {
            printf("map_page: No free frames for page table\n");
            return;
        }

        set_frame(frame);
        uint32_t pt_phys = frame * FRAME_SIZE;

        void* pt_virt = (pt_phys < 0x800000) ? (void*)pt_phys : temp_map_page(pt_phys);
        memset(pt_virt, 0, FRAME_SIZE);
        //page_directory[pd_index] = pt_phys | PAGE_PRESENT | PAGE_RW | (flags & PAGE_USER);
        uint32_t pd_flags = PAGE_PRESENT | PAGE_RW;
        if (flags & PAGE_USER) pd_flags |= PAGE_USER;
        page_directory[pd_index] = pt_phys | pd_flags;

        page_table = (uint32_t*)pt_virt;
        if (virtual_addr == 0xBFFFF000) {
    printf("Mapping VA 0x%x to PA 0x%x with flags 0x%x\n", virtual_addr, physical_addr, flags);
    printf("PD[%x] = 0x%x after mapping\n", pd_index, page_directory[pd_index]);
}

    } else {
        uint32_t pt_phys = page_directory[pd_index] & ~0xFFF;
        void* pt_virt = (pt_phys < 0x800000) ? (void*)pt_phys : temp_map_page(pt_phys);
        page_table = (uint32_t*)pt_virt;

        if (virtual_addr == 0xBFFFF000) {
    printf("Mapping VA 0x%x to PA 0x%x with flags 0x%x\n", virtual_addr, physical_addr, flags);
    printf("PD[%x] = 0x%x after mapping\n", pd_index, page_directory[pd_index]);
}


    }

    page_table[pt_index] = physical_addr | PAGE_PRESENT | PAGE_RW | (flags & PAGE_USER);
    //asm volatile("invlpg (%0)" :: "r"(virtual_addr) : "memory");

}


void map_elf_blob(void *addr, uint32_t length) {
    uint32_t start = (uint32_t)addr & 0xFFFFF000;
    uint32_t end   = ((uint32_t)addr + length + 0x1000) & 0xFFFFF000;
    for (uint32_t a = start; a < end; a += 0x1000){
        if (a <= 0x101bc0 && a + 0x1000 > 0x101bc0) {
            printf("0x101bc0 is being mapped at page: 0x%x\n", a);
        }
        map_page(a, a, PAGE_PRESENT | PAGE_RW | PAGE_USER);  // Identity map
    }
}


void init_kernel_heap_mapping() {
    // Map pages for the heap virtual range
    for (uint32_t addr = KERNEL_HEAP_START; addr < KERNEL_HEAP_START + KERNEL_HEAP_SIZE; addr += FRAME_SIZE) {
        uint32_t frame = first_free_frame();
        if (frame == (uint32_t)-1) {
            // no frames available, panic or return
            return;
        }
        set_frame(frame);
        uint32_t phys_addr = frame * FRAME_SIZE;

        map_page(addr, phys_addr, PAGE_PRESENT | PAGE_RW);
    }
}

#define KERNEL_START 0x00100000
#define KERNEL_END   0x00200000  // adjust based on your kernel size

extern uint32_t _start;
extern uint32_t __kernel_end;


void mark_usable_frames() {

    uint32_t kernel_start_addr = (uint32_t)&_start;
    uint32_t kernel_end_addr = (uint32_t)&__kernel_end;

    for (int i = 0; i < usable_region_count; i++) {
        uint32_t base = usable_regions[i].base;
        uint32_t length = usable_regions[i].length;
        uint32_t start_frame = base / FRAME_SIZE;
        uint32_t end_frame = (base + length) / FRAME_SIZE;

        for (uint32_t frame = start_frame; frame < end_frame; frame++) {

            uint32_t addr = frame * FRAME_SIZE;

            if (addr >= kernel_start_addr && addr < heap_end) continue;
            clear_frame(frame); // Mark as free
        }
    }
}

void alloc_user_stack(uint32_t user_stack_top, uint32_t size_in_pages) {
    for (uint32_t i = 0; i < size_in_pages; i++) {
        uint32_t virt_addr = user_stack_top - (i + 1) * FRAME_SIZE;

        uint32_t frame = first_free_frame();
        if (frame == (uint32_t)-1) {
            printf("alloc_user_stack: out of memory\n");
            return;
        }

        set_frame(frame);
        uint32_t phys_addr = frame * FRAME_SIZE;

        map_page(virt_addr, phys_addr, PAGE_PRESENT | PAGE_RW | PAGE_USER);

    }
}

