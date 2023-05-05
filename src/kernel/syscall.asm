%include "asmdefs.asm"

extern do_syscall
extern __current_task_stack

; void switch_to_asm(uint64_t new_rsp, uint64_t *put_old_rsp)
extern switch_to_asm
switch_to_asm:
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov [rsi], rsp
    mov rsp, rdi

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp

    ret

global syscall_enable
syscall_enable:
    ; Store kernel CS base in STAR [47:32] and user CS base in STAR [63:48]
    mov ecx, 0xC0000081 ; STAR
    rdmsr
    mov edx, 0x00100008
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
    wrmsr

    ret

global pop_regs_and_sysret
pop_regs_and_sysret:
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rsi
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; rip and rsp are all that remains
    pop rcx ; pop rip into rcx, restored by sysret
    pop rsp

    mov r11, 0x1b
    mov ds, r11
    mov r11, 0x202
    o64 sysret

rsp_scratch:
dq 0

global syscall_entry
syscall_entry:
    ; Save rsp in static scratch space
    mov [rsp_scratch], rsp

    ; Switch to kernel stack
    mov rbp, 0
    mov rsp, [__current_task_stack]

    ; Construct struct regs on scratch stack
    push qword [rsp_scratch]
    push rcx ; (rip) syscall sets rcx to the userspace rip
    push rax
    push rbx
    push rcx
    push rdx
    push rdi
    push rsi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp
    call do_syscall

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rsi
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    pop rcx ; Pop rip into rcx, sysret will set rip = rcx
    pop rsp ; Switch back to userspace stack

    mov r11, 0x202 ; Re-enable interrupts on return to usermode
    o64 sysret
