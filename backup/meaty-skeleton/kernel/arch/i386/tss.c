#include <stdint.h>
#include <kernel/gdt.h>
#include <kernel/tss.h>
#include <string.h>


static struct tss_entry tss;

extern void tss_flush(); // implemented in ASM, loads TR register

void write_tss(int num, uint16_t ss0, uint32_t esp0){

    uint32_t base = (uint32_t)&tss;
    uint32_t limit = sizeof(tss) - 1;

    //gdt_set_gate(num, base, limit, 0x89, 0x40); // Present, Ring 0, type 0x9 = 32-bit TSS (available)
    gdt_set_gate(num, base, limit, 0x89, 0x40);

    memset(&tss, 0, sizeof(tss));
    tss.ss0 = ss0;
    tss.esp0 = esp0;
    tss.cs = 0x1B; // user mode code segment (GDT index 3 | 0x3)
    tss.ss = tss.ds = tss.es = tss.fs = tss.gs = 0x23; // user data segment (GDT index 4 | 0x3)
    tss.iomap_base = sizeof(tss);
}