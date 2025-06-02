#include <kernel/user_entry.h>
#include <stdio.h>

// usermode.c
int write(const char* msg) {
    int syscall_num = 1;  // assume syscall #1 is write
    int ret;
    asm volatile (
        "int $0x80"
        : "=a" (ret)
        : "a" (syscall_num), "b" (msg)
    );
    return ret;
}


void user_mode_entry() {
    
    while (1) {
        write("in usermode now");
        // You can test syscalls, faults, or just halt here
        asm volatile ("hlt");
    }
}
