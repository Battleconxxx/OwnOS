#ifndef ELF_LOADER_H
#define ELF_LOADER_H

#include <stdint.h>

uint32_t load_elf(const char* filename);
uint32_t load_elf_from_memory(void* elf_data, size_t size);

#endif
