#include <stdint.h>
void user_mode_entry() {
    
    int x = 1234;
    x++;
    volatile uint32_t *debug = (volatile uint32_t *)0xBFFF0000;
    debug[0] = 0x12345678;
    //printf("Welcome to user mode");
    for(;;){}
}
