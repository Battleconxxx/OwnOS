#include "kernel/stdio.h"
#include "kernel/interrupts.h"
#include "kernel/thread.h"
#include <kernel/tty.h>

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43
#define PIT_FREQUENCY 1193182
#define PIT_IRQ 0
#define PIT_DEFAULT_HZ 100  // 100 Hz timer interrupt frequency


struct idt_entry idt[256];
struct idt_ptr idtp;

extern void idt_load(); // Assembly function to load IDT

extern void isr0();
extern void isr8();
extern void isr13();
extern void isr14();
extern void isr32();
extern void isr80();
extern void default_isr();

// extern void yield(void);

static thread_t* current_thread = 0;
// static thread_t* thread_list = 0;

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

// void pic_remap() {
//     outb(0x20, 0x11);
//     outb(0xA0, 0x11);
//     outb(0x21, 0x20);
//     outb(0xA1, 0x28);
//     outb(0x21, 0x04);
//     outb(0xA1, 0x02);
//     outb(0x21, 0x01);
//     outb(0xA1, 0x01);
//     // Mask timer IRQ0 (bit0=1)
//     outb(0x21, 0x01);
//     // Leave slave PIC unmasked (or mask as needed)
//     outb(0xA1, 0x0);
// }


void idt_install() {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint32_t)&idt;

    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, (uint32_t)default_isr, 0x08, 0x8E);
    }    
}

void init_interrupts() {

    pic_remap();
    idt_install();
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E); // Divide-by-zero
    idt_set_gate(8, (uint32_t)isr8, 0x08, 0x8E); // Double Fault
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E); // Page fault
    idt_set_gate(32, (uint32_t)isr32, 0x08, 0x8E); // Timer
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E); // gpf
    idt_set_gate(0x80, (uint32_t)isr80, 0x08, 0xEE);  // Interrupt gate, DPL=3 (0xEE)

    //pit_init(PIT_DEFAULT_HZ);
    idt_load();
    asm volatile ("sti");
}

// uint32_t read_cr2() {
//     uint32_t cr2;
//     asm volatile ("mov %%cr2, %0" : "=r"(cr2));
//     return cr2;
// }

void page_fault_handler(uint32_t fault_addr, uint32_t error_code) {
    asm volatile("cli");
    uint32_t cr2;
    asm volatile("mov %%cr2, %0" : "=r"(cr2));
    // Avoid printf to prevent faults
    // Print to a fixed memory location or serial port
    volatile uint32_t *debug = (volatile uint32_t *)0xC0001000; // Adjust to mapped address
    debug[0] = cr2;
    debug[1] = error_code;
    debug[2] = 0xDEADBEEF;
    for(;;);
}


#include <kernel/tty.h>
#include <kernel/interrupts.h>

void timer_handler(uint32_t error_code, uint32_t interrupt_number) {
    asm volatile("cli"); // Disable interrupts
    // Acknowledge the interrupt (send EOI to PIC)
    outb(0x20, 0x20); // EOI to PIC1 (master)

    // Debug output (optional, use with caution to avoid recursion)
    printf("Timer interrupt (IRQ0) fired, tick %u\n", interrupt_number);

    // Optionally, increment a system tick counter
    static uint32_t tick = 0;
    tick++;
    volatile uint32_t *debug = (volatile uint32_t *)0xC0001000; // Mapped address
    debug[3] = tick; // Store tick count

    // Re-enable interrupts (handled by iret in isr32)
}

void divide_by_zero_handler(uint32_t error_code, uint32_t int_no) {
    printf("Divide by zero error (int %d), error code: 0x%x\n", int_no, error_code);
    for(;;){}
}

//multithreading stuff

void pit_init(uint32_t frequency) {
    uint32_t divisor = PIT_FREQUENCY / frequency;
    outb(PIT_COMMAND, 0x36);          // Channel 0, mode 3 (square wave)
    outb(PIT_CHANNEL0, divisor & 0xFF);       // Low byte
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF); // High byte
}

void gpf_handler(registers_t *regs) {
    printf("\nGPF: int_no=%u err_code=0x%x\n", regs->int_no, regs->err_code);
    printf("EIP=0x%x CS=0x%x EFLAGS=0x%x\n", regs->eip, regs->cs, regs->eflags);
    for(;;);
}


void fault_handler(uint32_t int_no, uint32_t error_code) {
    printf("Unhandled exception %d, error: 0x%x\n", int_no, error_code);
    for(;;){}
}

void double_fault_handler(uint32_t error_code, uint32_t int_no) {
    // Double Fault usually has error_code = 0 (CPU pushes 0)
    printf("Double Fault Exception (int %d), error code: 0x%x\n", int_no, error_code);
    // Halt the system since this is critical
    for(;;){}
}

#define MAX_INTERRUPTS 256
static isr_handler_t interrupt_handlers[MAX_INTERRUPTS];

void register_interrupt_handler(uint8_t n, isr_handler_t handler) {
    interrupt_handlers[n] = handler;
}
