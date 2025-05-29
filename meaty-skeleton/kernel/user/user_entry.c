#include <kernel/user.h>

void user_mode_entry() {
    // This is now running in ring 3
    while (1) {
        // You can test syscalls, faults, or just halt here
        asm volatile ("hlt");
    }
}
