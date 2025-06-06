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

typedef struct registers {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;


typedef void (*isr_handler_t)(registers_t*);

extern struct idt_ptr idtp;

void idt_set_gate(int num, uint32_t base, uint16_t sel, uint8_t flags);
void init_interrupts();
void page_fault_handler(uint32_t error_code, uint32_t fault_addr);
//uint32_t* timer_handler(uint32_t* current_esp);
void timer_handler(uint32_t error_code, uint32_t interrupt_number);
void divide_by_zero_handler(uint32_t error_code, uint32_t int_no);
void pit_init(uint32_t frequency);
void fault_handler(uint32_t int_no, uint32_t error_code);
void double_fault_handler(uint32_t error_code, uint32_t int_no);
void gpf_handler(registers_t *regs);

void register_interrupt_handler(uint8_t n, isr_handler_t handler);

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

#endif
