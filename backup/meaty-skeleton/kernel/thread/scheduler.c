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
    thread_t* next = current_thread->next;

    asm volatile (
        "pusha\n"                    // Save all registers
        "mov %%esp, %[prev_sp]\n"    // Save stack pointer
        "mov %[next_sp], %%esp\n"    // Load new stack pointer
        "popa\n"                     // Restore all registers
        "ret\n"                      // Return to next thread
        : [prev_sp] "=m"(prev->stack_pointer)
        : [next_sp] "m"(next->stack_pointer)
    );

    current_thread = next;
}

void init_threading() {
    // Create a thread_t for the currently running "main" thread
    thread_t* main_thread = (thread_t*) kmalloc(sizeof(thread_t));

    uint32_t esp;
    asm volatile ("mov %%esp, %0" : "=r"(esp));  // get current stack pointer
    main_thread->stack_pointer = (uint32_t*) esp;
    main_thread->entry = 0;  // Not applicable to main thread
    main_thread->next = 0;

    thread_list = current_thread = main_thread;

    printf("Threading initialized\n");
}
