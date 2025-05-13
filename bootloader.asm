[org 0x7c00]

mov bp, 0x7c00
mov sp, bp

mov bx, Teststring
call Printfunction


JMP $
Printfunction:
    mov ah, 0x0e
    .Loop:
        mov al, [bx]
        int 0x10
        inc bx
        cmp [bx], byte 0
        je .Exit
        jmp .Loop
    .Exit:
        ret


Teststring:
    db 'This is a test string',0

times 510-($-$$) db 0

dw 0xAA55