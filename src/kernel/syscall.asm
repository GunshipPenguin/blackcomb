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

extern current
extern do_syscall
syscall_entry:
    ; Construct struct regs on stack
    push rcx ; syscall sets rcx to the userspace rip

    push rax
    push rbx
    push rcx
    push rdx
    push rdi
    push rsi
    push rbp
    push rsp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    call do_syscall

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsp
    pop rbp
    pop rsi
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    pop rcx ; Pop rip into rcx, sysret will set rip = rcx

    o64 sysret
