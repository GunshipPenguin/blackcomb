global flush_gdt

; reload_gdt(uint32_t code, uint32_t data)
flush_gdt:
    mov eax, [esp + 8] ; Data segment selector

    mov ds, eax
    mov es, eax
    mov ss, eax
    mov gs, eax
    mov fs, eax

    push WORD [esp + 4] ; Code segment selector
    push longjmp_target
    jmp far [esp + 6]

longjmp_target:
    sub esp, 6
    ret
