// #include <string.h>
#include <stdint.h>
#include <stddef.h>
// void* memset(void* bufptr, int value, size_t size) {
// 	unsigned char* buf = (unsigned char*) bufptr;
// 	for (size_t i = 0; i < size; i++)
// 		buf[i] = (unsigned char) value;
// 	return bufptr;
// }

void *memset(void *ptr, int value, size_t num) {
    uint8_t *p = (uint8_t *)ptr;
    for (size_t i = 0; i < num; i++) {
        p[i] = (uint8_t)value;
    }
    return ptr;
}
