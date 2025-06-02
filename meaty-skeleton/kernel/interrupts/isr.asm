[bits 32]

global isr0
extern divide_by_zero_handler

isr0:
    pusha
    push dword 0           ; dummy error code (no error code for #DE)
    push dword 0           ; interrupt number 0
    call divide_by_zero_handler
    add esp, 8
    popa
    iret


global isr8
extern double_fault_handler

isr8:
    cli
    pusha
    push dword 0           ; double fault has error code 0 pushed by CPU
    push dword 8           ; interrupt number 8
    call double_fault_handler
    add esp, 8
    popa
    iret


global isr13
extern gpf_handler

isr13:
    cli
    pusha
    mov eax, [esp + 36]    ; error code pushed by CPU
    push eax               ; error code
    push dword 13          ; interrupt number 13
    call gpf_handler
    add esp, 8
    popa
    iret


global isr14
extern page_fault_handler

isr14:
    cli
    pusha
    mov eax, cr2           ; faulting address
    push eax               ; faulting address
    mov eax, [esp + 36]    ; error code
    push eax               ; error code
    call page_fault_handler
    add esp, 8
    popa
    iret


global isr32
extern timer_handler

isr32:
    cli
    pusha
    mov eax, esp           ; pointer to saved registers on stack
    push eax
    call timer_handler
    pop eax
    mov esp, eax           ; switch to new stack pointer returned by timer_handler
    popa                   ; restore registers from new stack
    sti
    iret
