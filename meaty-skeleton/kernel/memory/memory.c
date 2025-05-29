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

uint8_t kernel_stack[KERNEL_STACK_SIZE] __attribute__((aligned(16)));
uint8_t user_stack[USER_STACK_SIZE] __attribute__((aligned(16)));
void* user_stack_top = &user_stack[USER_STACK_SIZE];

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

void init_paging() {

    // Clear page directory
    for (int i = 0; i < 1024; i++) page_directory[i] = 0;

    for (int i = 0; i < 1024; i++) {
        first_page_table[i] = (i * 0x1000) | PAGE_PRESENT | PAGE_RW;
    }
    page_directory[0] = ((uint32_t)first_page_table) | PAGE_PRESENT | PAGE_RW;


    //next 4mb
    // Second page table: map 0x00400000 - 0x007FFFFF (next 4MB)
    static uint32_t second_page_table[1024] __attribute__((aligned(4096)));
    for (int i = 0; i < 1024; i++) {
        second_page_table[i] = (i * 0x1000 + 0x00400000) | PAGE_PRESENT | PAGE_RW;
    }
    page_directory[1] = ((uint32_t)second_page_table) | PAGE_PRESENT | PAGE_RW;

    static uint32_t kernel_table[1024] __attribute__((aligned(4096)));

    for (int i = 0; i < 1024; i++)
        kernel_table[i] = (i * 0x1000 + 0x00100000) | PAGE_PRESENT | PAGE_RW;
    page_directory[768] = ((uint32_t)kernel_table) | PAGE_PRESENT | PAGE_RW;

    asm volatile ("mov %0, %%cr3" :: "r"(page_directory));

    uint32_t cr0;
    asm volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;  // enable paging
    asm volatile ("mov %0, %%cr0" :: "r"(cr0));
}


// Helper to map a single 4KB page: assumes page directory & page tables exist
extern uint32_t page_directory[1024]; // from memory.c

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


        // For now assume the page table is identity mapped (this should be mapped properly later)
        void* pt_virt = (pt_phys < 0x800000) ? (void*)pt_phys : NULL;

        // Assuming identity map below 8MB, so if pt_phys < 8MB, use direct cast
        if (!pt_virt) {
            printf("map_page: pt_phys > 8MB, can't access page table\n");
            return;
        }

        // Zero the page table frame
        memset(pt_virt, 0, FRAME_SIZE);

        // Since first 4MB is identity mapped, phys == virt here
        page_directory[pd_index] = pt_phys | PAGE_PRESENT | PAGE_RW | (flags & PAGE_USER);

        page_table = (uint32_t*)pt_virt;
    } else {
        // Page table already present, get its virtual address
        uint32_t pt_phys = page_directory[pd_index] & ~0xFFF;

        void* pt_virt = (pt_phys < 0x800000) ? (void*)pt_phys : NULL;
        if (!pt_virt) {
            printf("map_page: pt_phys > 8MB, can't access page table\n");
            return;
        }

        page_table = (uint32_t*)pt_virt;
    }

    //doubtful line
    page_table[pt_index] = physical_addr | PAGE_PRESENT | PAGE_RW | (flags & PAGE_USER);
}

void map_elf_blob(void *addr, uint32_t length) {
    uint32_t start = (uint32_t)addr & 0xFFFFF000;
    uint32_t end   = ((uint32_t)addr + length + 0x1000) & 0xFFFFF000;

    for (uint32_t a = start; a < end; a += 0x1000)
        map_page(a, a, PAGE_PRESENT | PAGE_RW);  // Identity map
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


void mark_usable_frames() {
    for (int i = 0; i < usable_region_count; i++) {
        uint32_t base = usable_regions[i].base;
        uint32_t length = usable_regions[i].length;
        uint32_t start_frame = base / FRAME_SIZE;
        uint32_t end_frame = (base + length) / FRAME_SIZE;

        for (uint32_t frame = start_frame; frame < end_frame; frame++) {

            uint32_t addr = frame * FRAME_SIZE;

            if (addr >= KERNEL_START && addr < heap_end) continue;
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

        // Map with user-accessible flag
        uint32_t pd_index = virt_addr >> 22;
        uint32_t pt_index = (virt_addr >> 12) & 0x3FF;

        uint32_t* page_table;

        if (!(page_directory[pd_index] & PAGE_PRESENT)) {
            uint32_t pt_frame = first_free_frame();
            if (pt_frame == (uint32_t)-1) {
                printf("alloc_user_stack: No free frame for page table\n");
                return;
            }
            set_frame(pt_frame);
            uint32_t pt_phys = pt_frame * FRAME_SIZE;

            void* pt_virt = (pt_phys < 0x800000) ? (void*)pt_phys : NULL;
            if (!pt_virt) {
                printf("alloc_user_stack: pt_phys > 8MB, cannot map\n");
                return;
            }

            memset(pt_virt, 0, FRAME_SIZE);
            page_directory[pd_index] = pt_phys | PAGE_PRESENT | PAGE_RW | PAGE_USER;
            page_table = (uint32_t*)pt_virt;
        } else {
            uint32_t pt_phys = page_directory[pd_index] & ~0xFFF;
            void* pt_virt = (pt_phys < 0x400000) ? (void*)pt_phys : NULL;
            if (!pt_virt) {
                printf("alloc_user_stack: existing pt > 4MB\n");
                return;
            }
            page_table = (uint32_t*)pt_virt;
        }

        page_table[pt_index] = phys_addr | PAGE_PRESENT | PAGE_RW | PAGE_USER;
    }
}

