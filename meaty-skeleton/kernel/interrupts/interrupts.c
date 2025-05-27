#include "kernel/stdio.h"
#include "kernel/interrupts.h"

struct idt_entry idt[256];
struct idt_ptr idtp;

extern void idt_load(); // Assembly function to load IDT
extern void isr14();

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

    idt_install();
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_load();  // Load IDT using lidt instruction
    asm volatile ("sti");
}

void page_fault_handler(uint32_t error_code, uint32_t fault_addr) {
    printf("Page fault!\n");
    printf("Fault addr: ");
    printf("%x", fault_addr);
    printf("\n");
    printf("Error code: ");
    printf("%x", error_code);
    printf("\n");

    while (1){ asm("hlt");} // Halt
}