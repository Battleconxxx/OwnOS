#include <stdint.h>
#include <kernel/stdio.h> // for printf


void page_fault_handler(uint32_t error_code, uint32_t fault_addr) {
    printf("Page fault at 0x%x, error code: 0x%x\n", fault_addr, error_code);
    while (1) { asm("hlt"); }
}
