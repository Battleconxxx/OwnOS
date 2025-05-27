#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>

struct idt_entry {
    uint16_t base_low;
    uint16_t sel;
    uint8_t always0;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

extern struct idt_ptr idtp;

void idt_set_gate(int num, uint32_t base, uint16_t sel, uint8_t flags);
void init_interrupts();
void page_fault_handler(uint32_t error_code, uint32_t fault_addr);

#endif
