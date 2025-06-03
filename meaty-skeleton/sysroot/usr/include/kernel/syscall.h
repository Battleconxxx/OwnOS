#ifndef _SYSCALL_H
#define _SYSCALL_H

#pragma once
#include <stdint.h>
#include <kernel/interrupts.h>


uint32_t syscall_dispatcher(uint32_t syscall_number, uint32_t arg1, uint32_t arg2, uint32_t arg3);
void syscall_isr_handler(registers_t *regs);
void init_syscalls();

#endif
