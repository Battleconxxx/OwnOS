#include <kernel/interrupts.h>
#include <kernel/syscall.h>
#include <kernel/syscall_numbers.h>
#include <kernel/tty.h>
#include <kernel/memory.h>

// This is the ISR handler for interrupt 0x80
// void syscall_isr_handler(registers_t *regs) {
//     uint32_t ret = syscall_dispatcher(regs->eax, regs->ebx, regs->ecx, regs->edx);
//     regs->eax = ret; // return value in eax
// }

void syscall_isr_handler(registers_t *regs) {
    asm volatile (
        "mov $kernel_stack_top, %esp\n"
        "mov $kernel_stack_top, %ebp\n"
    );
    // Now you're on the kernel stack
}


// This is your syscall dispatcher logic
uint32_t syscall_dispatcher(uint32_t syscall_number, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    switch (syscall_number) {
        case SYSCALL_WRITE:
            terminal_writestring((const char*)arg1); // arg1 is pointer to string
            return 0;
        default:
            return (uint32_t)-1;
    }
}

// Register the syscall ISR
void init_syscalls() {
    register_interrupt_handler(0x80, syscall_isr_handler);
}