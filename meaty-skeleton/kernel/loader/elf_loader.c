#include <stdint.h>
#include <kernel/fs.h>
#include <kernel/memory.h>
#include <string.h>
#include <kernel/stdio.h>

#define PT_LOAD 1

typedef struct {
    uint8_t magic[4];
    uint8_t bit_format;
    uint8_t endian;
    uint8_t version;
    uint8_t abi;
    uint8_t pad[8];
    uint16_t type;
    uint16_t machine;
    uint32_t version2;
    uint32_t entry;
    uint32_t phoff;
    uint32_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
} __attribute__((packed)) Elf32_Ehdr;

typedef struct {
    uint32_t type;
    uint32_t offset;
    uint32_t vaddr;
    uint32_t paddr;
    uint32_t filesz;
    uint32_t memsz;
    uint32_t flags;
    uint32_t align;
} __attribute__((packed)) Elf32_Phdr;

uint32_t load_elf(const char* filename) {
    char buffer[MAX_FILE_SIZE];
    int size = ramfs_read(filename, buffer, sizeof(buffer));
    if (size <= 0) {
        printf("ELF Load Error: File not found or empty\n");
        return 0;
    }

    Elf32_Ehdr* ehdr = (Elf32_Ehdr*)buffer;
    if (!(ehdr->magic[0] == 0x7F && ehdr->magic[1] == 'E' && ehdr->magic[2] == 'L' && ehdr->magic[3] == 'F')) {
        printf("Not a valid ELF file\n");
        return 0;
    }

    Elf32_Phdr* phdr = (Elf32_Phdr*)(buffer + ehdr->phoff);

    for (int i = 0; i < ehdr->phnum; i++) {
        if (phdr[i].type != PT_LOAD) continue;

        uint32_t virt_addr = phdr[i].vaddr;
        uint32_t memsz = phdr[i].memsz;
        uint32_t filesz = phdr[i].filesz;
        uint32_t offset = phdr[i].offset;

        for (uint32_t addr = virt_addr; addr < virt_addr + memsz; addr += FRAME_SIZE) {
            uint32_t frame = first_free_frame();
            if (frame == (uint32_t)-1) return 0;
            set_frame(frame);
            map_page(addr, frame * FRAME_SIZE, PAGE_PRESENT | PAGE_RW | PAGE_USER);
            
        }

        // Zero out segment memory first
        memset((void*)virt_addr, 0, memsz);
        memcpy((void*)virt_addr, buffer + offset, filesz);
    }

    return ehdr->entry;
}


uint32_t load_elf_from_memory(void* elf_data, size_t size) {
    if (size < sizeof(Elf32_Ehdr)) {
        printf("ELF Load Error: Data too small\n");
        return 0;
    }

    Elf32_Ehdr* ehdr = (Elf32_Ehdr*)elf_data;

    if (!(ehdr->magic[0] == 0x7F && ehdr->magic[1] == 'E' && ehdr->magic[2] == 'L' && ehdr->magic[3] == 'F')) {
        printf("Not a valid ELF file\n");
        return 0;
    }

    if (ehdr->phoff + ehdr->phnum * sizeof(Elf32_Phdr) > size) {
        printf("ELF Load Error: Program header out of bounds\n");
        return 0;
    }

    Elf32_Phdr* phdr = (Elf32_Phdr*)((uint8_t*)elf_data + ehdr->phoff);

    for (int i = 0; i < ehdr->phnum; i++) {
        if (phdr[i].type != PT_LOAD) continue;

        uint32_t virt_addr = phdr[i].vaddr;
        uint32_t memsz = phdr[i].memsz;
        uint32_t filesz = phdr[i].filesz;
        uint32_t offset = phdr[i].offset;

        if (offset + filesz > size) {
            printf("ELF Load Error: Segment out of bounds\n");
            return 0;
        }

        // Map pages
        for (uint32_t addr = virt_addr; addr < virt_addr + memsz; addr += FRAME_SIZE) {
            uint32_t frame = first_free_frame();
            if (frame == (uint32_t)-1) {
                printf("ELF Load Error: Out of memory frames\n");
                return 0;
            }
            set_frame(frame);
            map_page(addr, frame * FRAME_SIZE, PAGE_PRESENT | PAGE_RW | PAGE_USER);            
        }

        // Zero out the segment memory first
        memset((void*)virt_addr, 0, memsz);
        // Copy segment data from elf_data
        memcpy((void*)virt_addr, (uint8_t*)elf_data + offset, filesz);
    }

    return ehdr->entry;
}