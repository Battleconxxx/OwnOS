#include <kernel/thread.h>
#include <kernel/tty.h>
#include <kernel/memory.h>
#include <kernel/stdio.h>

#define STACK_SIZE 4096

static thread_t* current_thread = 0;
static thread_t* thread_list = 0;

void create_thread(void (*function)(void)) {
    thread_t* t = (thread_t*) kmalloc(sizeof(thread_t));
    t->stack_pointer = (uint32_t*) kmalloc(STACK_SIZE) + STACK_SIZE / sizeof(uint32_t);
    t->entry = function;
    t->next = 0;

    // Simulate stack frame for context switch
    *(--t->stack_pointer) = (uint32_t) function; // return address (eip)
    for (int i = 0; i < 7; i++)  // push ebp, edi, esi, ebx, etc.
        *(--t->stack_pointer) = 0;

    if (!thread_list) {
        thread_list = t;
        current_thread = t;
    } else {
        thread_t* temp = thread_list;
        while (temp->next) temp = temp->next;
        temp->next = t;
    }
}

void yield() {
    if (!current_thread || !current_thread->next) return;

    thread_t* prev = current_thread;
    current_thread = current_thread->next;

    asm volatile (
        "mov %%esp, %[prev_sp]\n"     // save old ESP
        "mov %[next_sp], %%esp\n"     // load new ESP
        "popa\n"                      // restore general-purpose registers
        "ret\n"                       // return to the new thread
        : [prev_sp] "=m"(prev->stack_pointer)
        : [next_sp] "m"(current_thread->stack_pointer)
    );
}

void init_threading() {
    printf("Threading initialized\n");
}
