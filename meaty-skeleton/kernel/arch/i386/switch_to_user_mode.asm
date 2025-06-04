global switch_to_user_mode
extern user_mode_entry


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
    push 0xBFFFF000     ; User ESP (current ESP becomes user stack)
    pushf               ; EFLAGS
    push 0x1B           ; User CS
    push dword user_mode_entry ; EIP = entry point in user mode

    ; Far return into user mode
    iret
