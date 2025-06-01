#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdint.h>

void exception_handler(uint8_t int_no, uint32_t error_code);
void page_fault_handler(uint32_t faulting_address, uint32_t error_code);

#endif