#include <stdint.h>
#include <kernel/memory.h>
#include <stdio.h>

uint8_t memory_bitmap[MAX_FRAMES / 8];
memory_region_t usable_regions[MAX_MEMORY_REGIONS];
int usable_region_count = 0;

//paging stuff
uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));

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

//paging stuff
// void init_paging() {
//     for (int i = 0; i < 1024; i++) {
//         // Identity map first 4MB (0x00000000–0x003FFFFF)
//         first_page_table[i] = (i * 0x1000) | PAGE_PRESENT | PAGE_RW;
//     }

//     // Point first page directory entry to the page table
//     page_directory[0] = ((uint32_t)first_page_table) | PAGE_PRESENT | PAGE_RW;

//     // Zero out the rest of the directory
//     for (int i = 1; i < 1024; i++) {
//         page_directory[i] = 0;
//     }

//     // Load page directory into CR3
//     asm volatile ("mov %0, %%cr3" :: "r"(&page_directory));

//     // Enable paging: set the PG (bit 31) and PE (bit 0) in CR0
//     uint32_t cr0;
//     asm volatile ("mov %%cr0, %0" : "=r"(cr0));
//     cr0 |= 0x80000000;  // Set PG bit
//     asm volatile ("mov %0, %%cr0" :: "r"(cr0));
// }

void init_paging() {
    for (int i = 0; i < 1024; i++) {
        first_page_table[i] = (i * 0x1000) | PAGE_PRESENT | PAGE_RW;
    }
    for (int i = 1; i < 1024; i++) {
        page_directory[i] = 0;
    }
    page_directory[0] = ((uint32_t)first_page_table) | PAGE_PRESENT | PAGE_RW;

    asm volatile ("mov %0, %%cr3" :: "r"(page_directory));

    uint32_t cr0;
    asm volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;  // enable paging
    asm volatile ("mov %0, %%cr0" :: "r"(cr0));
}
