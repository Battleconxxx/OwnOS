#include "kernel/gdt.h"
#include "kernel/tss.h"
#include "kernel/memory.h"

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct gdt_entry gdt[6]; // 0: null, 1: kernel code, 2: kernel data, 3: user code, 4: user data, 5: TSS

static struct gdt_ptr gp;

extern void gdt_flush(uint32_t);

extern uint8_t kernel_stack[KERNEL_STACK_SIZE];


void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = base & 0xFFFF;
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = limit & 0xFFFF;
    gdt[num].granularity = (limit >> 16) & 0x0F;

    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access = access;
}

void gdt_install(void) {
    gp.limit = sizeof(struct gdt_entry) * 6 - 1;
    gp.base = (uint32_t)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);                // Null segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User code (ring 3, exec/read)
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User data (ring 3, read/write)
    //gdt_set_gate(5, (uint32_t)&tss_entry, sizeof(tss_entry), 0x89, 0x40);

    write_tss(5, 0x10, (uint32_t)(kernel_stack + sizeof(kernel_stack)));
   

    gdt_flush((uint32_t)&gp);
    tss_flush(); // Assembly function to load TR
}
