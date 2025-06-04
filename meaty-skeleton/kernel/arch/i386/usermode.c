#include <kernel/user_entry.h>
#include <kernel/memory.h>
#include <stdint.h>

#define USER_CODE_SEG 0x1B
#define USER_DATA_SEG 0x23

void switch_to_user_mode(uint32_t entry_point, uint32_t user_stack_top) {
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
        "pushl %1;"         // user entry point
        "iret;"
        :
        : "r"(user_stack_top), "r"(entry_point)
        : "eax"
    );
}
