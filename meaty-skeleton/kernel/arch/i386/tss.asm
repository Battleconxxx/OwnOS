mov ax, 0x28   ; TSS selector = 5th entry = index 5 * 8 = 0x28
ltr ax
