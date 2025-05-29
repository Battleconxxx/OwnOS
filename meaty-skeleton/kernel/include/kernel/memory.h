#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <kernel/multiboot.h>
#include <stddef.h>

#define FRAME_SIZE 4096
#define TOTAL_MEMORY (4ULL * 1024 * 1024 * 1024)
#define MAX_FRAMES (TOTAL_MEMORY / FRAME_SIZE)
#define BITMAP_SIZE (MAX_FRAMES / 8)

//heap for multithreading
#define KERNEL_HEAP_START 0xC0000000  // Typical kernel heap start (high mem)
// #define KERNEL_HEAP_SIZE  (4 * 1024 * 1024)  // 4 MB heap
#define KERNEL_HEAP_SIZE  (4 * 1024 * 1024)  // 4 MB heap



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

#define KERNEL_STACK_SIZE 4096
#define USER_STACK_SIZE 4096

extern uint8_t kernel_stack[KERNEL_STACK_SIZE];
#define KERNEL_STACK_TOP ((uint32_t)&kernel_stack[KERNEL_STACK_SIZE])


extern uint8_t user_stack[USER_STACK_SIZE];
extern void* user_stack_top;

void parse_memory_map(multiboot_info_t* mbi);
void init_memory();
void set_frame(uint32_t frame);
void clear_frame(uint32_t frame);
int test_frame(uint32_t frame);
uint32_t first_free_frame();
void init_paging();
void* kmalloc(size_t size);
void init_kernel_heap_mapping();
void mark_usable_frames();
void map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);
void map_elf_blob(void *addr, uint32_t length);


#endif