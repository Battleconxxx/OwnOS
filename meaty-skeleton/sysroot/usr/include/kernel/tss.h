#ifndef TSS_H
#define TSS_H

#include <stdint.h>

struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0;    // Stack pointer to load when switching to ring 0
    uint32_t ss0;     // Stack segment to load when switching to ring 0
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

static struct tss_entry tss_entry;

void write_tss(int num, uint16_t ss0, uint32_t esp0);
void tss_flush();

#endif
