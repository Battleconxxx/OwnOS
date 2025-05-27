#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

#define MAX_MEMORY_REGIONS 32

typedef struct {
    uint64_t base;
    uint64_t length;
} memory_region_t;

typedef struct {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed)) multiboot_memory_map_t;

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
} multiboot_info_t;

extern memory_region_t usable_regions[];
extern int usable_region_count;

void parse_memory_map(multiboot_info_t* mbi);

#endif