#include <stdint.h>
#include <kernel/stdio.h> // for printf


void page_fault_handler(uint32_t error_code, uint32_t fault_addr) {
    printf("Page fault!\n");
    printf("Fault addr: ");
    printf("%x", fault_addr);
    printf("\n");
    printf("Error code: ");
    printf("%x", error_code);
    printf("\n");

    while (1); // Halt
}
