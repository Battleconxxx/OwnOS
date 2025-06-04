// testprog.c
#define VGA_MEMORY ((volatile unsigned short*)0xB8000)

int main() {
    VGA_MEMORY[0] = (0x0F << 8) | 'A';  // white foreground, black background, character 'A'
    while (1) {
        asm volatile("hlt");
    }
    return 0;
}
