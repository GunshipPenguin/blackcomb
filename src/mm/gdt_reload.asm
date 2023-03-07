global flush_gdt

; rdi = uint32 code
; rsi = uint32 data
; rdx = uint32 tss
bits 64
flush_gdt:
    push rbp
    mov rbp, rsp

    push qword rsi
    push qword rbp

    pushf
    pop rax
    ; Mask TF, NT & RF, all of which have effects on iret we don't want
    and rax, 0b1111111111111111111111111111111111111111111111101011111011111111
    push rax
    popf

    pushf
    push qword rdi
    push qword longjmp_target
    iretq

longjmp_target:
    mov ds, rsi
    mov es, rsi
    mov ss, rsi
    mov gs, rsi
    mov fs, rsi

    ltr dx

    leave
    ret
