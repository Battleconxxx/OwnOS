[bits 32]

global isr0
extern divide_by_zero_handler

isr0:
    pusha                 ; Save all registers
    push dword 0          ; Push dummy error code
    push dword 0          ; Push interrupt number (0)
    call divide_by_zero_handler
    add esp, 8            ; Clean up stack (error code + int number)
    popa                  ; Restore registers
    iret

global isr14
extern page_fault_handler

isr14:
    cli
    pusha
    mov eax, cr2
    push eax        ; push faulting address
    push dword [esp + 36] ; push error code
    call page_fault_handler
    add esp, 8
    popa
    iret

global isr32
extern timer_handler
isr32:
    pusha
    call timer_handler
    popa
    iret
