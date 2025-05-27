#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <kernel/multiboot.h>

#define FRAME_SIZE 4096
#define TOTAL_MEMORY (4ULL * 1024 * 1024 * 1024)
#define MAX_FRAMES (TOTAL_MEMORY / FRAME_SIZE)
#define BITMAP_SIZE (MAX_FRAMES / 8)



#define MAX_MEMORY_REGIONS 32

//paging stuff
#define PAGE_PRESENT 0x1
#define PAGE_RW      0x2
#define PAGE_USER    0x4

typedef struct {
    uint64_t base;
    uint64_t length;
} memory_region_t;


extern memory_region_t usable_regions[];
extern int usable_region_count;

void parse_memory_map(multiboot_info_t* mbi);
void init_memory();
void set_frame(uint32_t frame);
void clear_frame(uint32_t frame);
int test_frame(uint32_t frame);
uint32_t first_free_frame();
void init_paging();

#endif