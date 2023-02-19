global syscall_enable
syscall_enable:
    ; Store kernel CS base in STAR [47:32] and user CS base in STAR [63:48]
    mov ecx, 0xC0000081 ; STAR
    rdmsr
    mov edx, 0x00180008
    wrmsr

    ; Load &syscall_entry in LSTAR
    mov ecx, 0xC0000082 ; LSTAR
    rdmsr
    mov rdi, syscall_entry
    mov eax, edi
    shr rdi, 32
    mov edx, edi
    wrmsr

    ; Enable syscall insn
    mov ecx, 0xC0000080 ; EFER
    rdmsr
    or eax, 1
    wrmsr

    mov ecx, 0xC0000084 ; FMASK
    rdmsr
    or eax, (1 << 9) | (1 << 10) ; Disable interrups, clear direction flag

    ret

; void enter_usermode(void *addr)
global enter_usermode
enter_usermode:
    mov rcx, rdi  ; arg1 = address to jump to
    mov r11, 0x202
    o64 sysret

syscall_entry:
    nop
    hlt
