extern isr_main_entry

isr_common:
    push rdi
    push rsi
    push rdx
    push rcx
    push rax
    push r8
    push r9
    push r10
    push r11

	mov rdi, rsp
    call isr_main_entry

    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rax
    pop r8
    pop r9
    pop r10
    pop r11

    add esp, 8 ; IRQ info field
    iretq

%macro isr_err_stub 1
isr_stub_%+%1:
    ; Store interrupt number in high 4 bytes of error code
    mov dword [rsp + 4], %1
    jmp isr_common
%endmacro

%macro isr_no_err_stub 1
isr_stub_%+%1:
    push qword 0 ; Consistency with error version
    jmp isr_common
%endmacro

%macro isr_irq_stub 1
isr_stub_%+%1:
	push qword %1
	jmp isr_common
%endmacro

isr_no_err_stub 0
isr_no_err_stub 1
isr_no_err_stub 2
isr_no_err_stub 3
isr_no_err_stub 4
isr_no_err_stub 5
isr_no_err_stub 6
isr_no_err_stub 7
isr_err_stub    8
isr_no_err_stub 9
isr_err_stub    10
isr_err_stub    11
isr_err_stub    12
isr_err_stub    13
isr_err_stub    14
isr_no_err_stub 15
isr_no_err_stub 16
isr_err_stub    17
isr_no_err_stub 18
isr_no_err_stub 19
isr_no_err_stub 20
isr_no_err_stub 21
isr_no_err_stub 22
isr_no_err_stub 23
isr_no_err_stub 24
isr_no_err_stub 25
isr_no_err_stub 26
isr_no_err_stub 27
isr_no_err_stub 28
isr_no_err_stub 29
isr_err_stub    30
isr_no_err_stub 31
isr_irq_stub    32
isr_irq_stub    33
isr_irq_stub    34
isr_irq_stub    35
isr_irq_stub    36
isr_irq_stub    37
isr_irq_stub    38
isr_irq_stub    39
isr_irq_stub    40

global isr_stub_table
isr_stub_table:
%assign i 0
%rep    40
    dq isr_stub_%+i
%assign i i+1
%endrep
