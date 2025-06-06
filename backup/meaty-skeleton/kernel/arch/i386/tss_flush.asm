global tss_flush
section .text

tss_flush:
    mov ax, 0x28     ; TSS selector = 5th entry * 8 = 0x28
    ltr ax
    ret
