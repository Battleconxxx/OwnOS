#include "kernel/stdio.h"
#include "kernel/interrupts.h"

struct idt_entry idt[256];
struct idt_ptr idtp;

extern void idt_load(); // Assembly function to load IDT
extern void isr14();
extern void isr0();
extern void isr32();

void idt_set_gate(int num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
    idt[num].base_high = (base >> 16) & 0xFFFF;
}

void pic_remap() {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0);
    outb(0xA1, 0x0);
}

void idt_install() {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint32_t)&idt;

    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);  // Clear entries
    }

    
}

void init_interrupts() {

    pic_remap();
    idt_install();
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E); // Divide-by-zero
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E); // Page fault
    idt_set_gate(32, (uint32_t)isr32, 0x08, 0x8E); // Timer
    idt_load();
    asm volatile ("sti");
}

void page_fault_handler(uint32_t error_code, uint32_t fault_addr) {
    printf("Page fault at 0x%x, error code: 0x%x\n", fault_addr, error_code);
    while (1) { asm("hlt"); }
}

void timer_handler() {
    printf("Timer tick\n");
    outb(0x20, 0x20); // EOI
}

void divide_by_zero_handler(uint32_t error_code, uint32_t int_no) {
    printf("Divide by zero error (int %d), error code: 0x%x\n", int_no, error_code);
    while (1) { asm("hlt"); }
}