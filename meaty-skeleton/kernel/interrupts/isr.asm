[bits 32]
global isr14
extern page_fault_handler

isr14:
    cli
    push eax
    mov eax, cr2
    push eax        ; push faulting address
    push dword [esp + 8] ; push error code
    call page_fault_handler
    add esp, 8
    pop eax
    sti
    iret
