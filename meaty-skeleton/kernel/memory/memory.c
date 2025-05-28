#include <stdint.h>
#include <kernel/memory.h>
#include <stdio.h>
#include <stddef.h>

uint8_t memory_bitmap[MAX_FRAMES / 8];
memory_region_t usable_regions[MAX_MEMORY_REGIONS];
int usable_region_count = 0;

//paging stuff
uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));

//heap stuff for multithreading
static uint32_t heap_next = KERNEL_HEAP_START;
static uint32_t heap_end = KERNEL_HEAP_START + KERNEL_HEAP_SIZE;

void* kmalloc(size_t size) {
    // Align size to 4 bytes
    size = (size + 3) & ~3;

    if (heap_next + size > heap_end) {
        // Heap overflow - no free memory available
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

            printf("Usable: base=0x%x, length=0x%x\n", (uint32_t)mmap->addr, (uint32_t)mmap->len);
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

void map_page(uint32_t virtual_addr, uint32_t physical_addr) {
    uint32_t pd_index = virtual_addr >> 22;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;

    // Get page table or allocate it
    uint32_t* page_table;
    if (!(page_directory[pd_index] & PAGE_PRESENT)) {
        // Allocate page table frame
        uint32_t frame = first_free_frame();
        if (frame == (uint32_t)-1) return; // No free frame

        set_frame(frame);
        uint32_t pt_phys = frame * FRAME_SIZE;

        // Map page table to virtual address (choose an identity map or a fixed region)
        // For simplicity, identity map page tables at some virtual address or store physical addr:
        page_directory[pd_index] = pt_phys | PAGE_PRESENT | PAGE_RW;
        page_table = (uint32_t*)(pt_phys); // You may need to identity map this address
    } else {
        page_table = (uint32_t*)(page_directory[pd_index] & ~0xFFF);
    }

    page_table[pt_index] = physical_addr | PAGE_PRESENT | PAGE_RW;
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

        map_page(addr, phys_addr);
    }
}

void mark_usable_frames() {
    for (int i = 0; i < usable_region_count; i++) {
        uint32_t base = usable_regions[i].base;
        uint32_t length = usable_regions[i].length;
        uint32_t start_frame = base / FRAME_SIZE;
        uint32_t end_frame = (base + length) / FRAME_SIZE;

        for (uint32_t frame = start_frame; frame < end_frame; frame++) {
            clear_frame(frame); // Mark as free
        }
    }
}