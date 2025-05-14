[org 0x7c00]

mov bp, 0x7c00
mov sp, bp

mov bx, Teststring
call Printfunction


JMP $

%include "print.asm"

times 510-($-$$) db 0

dw 0xAA55