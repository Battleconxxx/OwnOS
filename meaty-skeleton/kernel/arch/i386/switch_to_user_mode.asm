global switch_to_user_mode
extern user_mode_entry
extern user_stack_top

section .text
switch_to_user_mode:
    ; Set up data segment registers
    mov ax, 0x23        ; User data selector (GDT index 4 | 0x3)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Push the stack for IRET
    ; Stack layout: SS, ESP, EFLAGS, CS, EIP
    push 0x23           ; User SS
    mov eax, [user_stack_top]            ; User ESP (current ESP becomes user stack)
    push eax
    pushf               ; EFLAGS
    push 0x1B           ; User CS
    push dword user_mode_entry ; EIP = entry point in user mode

    ; Far return into user mode
    iret
