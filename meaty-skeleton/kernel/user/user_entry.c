#include <kernel/user_entry.h>
#include <stdio.h>


void user_mode_entry() {
    // This is now running in ring 3
    printf("Now in user mode");
    while (1) {
        // You can test syscalls, faults, or just halt here
        asm volatile ("hlt");
    }
}
