#ifndef THREAD_H
#define THREAD_H

#include <stdint.h>

typedef struct thread {
    uint32_t* stack_pointer;
    void (*entry)(void);
    struct thread* next;
} thread_t;

void init_threading();
void create_thread(void (*function)(void));
void yield();

#endif
