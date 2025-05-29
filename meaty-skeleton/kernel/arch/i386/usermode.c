#include <kernel/user.h>

#define USER_CODE_SEG 0x1B
#define USER_DATA_SEG 0x23

void switch_to_user_mode() {
    asm volatile (
        "cli;"
        "mov $0x23, %%ax;"
        "mov %%ax, %%ds;"
        "mov %%ax, %%es;"
        "mov %%ax, %%fs;"
        "mov %%ax, %%gs;"

        "mov %0, %%eax;"    // User stack
        "pushl $0x23;"      // ss
        "pushl %%eax;"      // esp
        "pushf;"            // eflags
        "pushl $0x1B;"      // cs
        "pushl $1f;"        // eip
        "iret;"
        "1:\n"
        "mov $0x20, %%eax;" // system call test
        :
        : "r"(user_stack_top)
        : "eax"
    );
}