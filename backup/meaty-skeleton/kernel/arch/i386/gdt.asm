global gdt_flush

section .text

gdt_flush:
    mov eax, [esp + 4]
    lgdt [eax]

    mov ax, 0x10      ; Data segment selector (2nd entry, 0x10)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:.flush_done  ; Code segment selector (1st entry, 0x08)

.flush_done:
    ret
