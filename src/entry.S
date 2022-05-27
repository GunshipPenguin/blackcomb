section .multiboot_header
align 8
header_start:
    dd 0xE85250D6                ; Magic number (multiboot 2)
    dd 0                         ; Architecture 0 (protected mode i386)
    dd header_end - header_start ; Header length
    dd 0x100000000 - (0xE85250D6 + 0 + (header_end - header_start)) ; Checksum
    dw 0
    dw 0
    dd 8
header_end:
extern kernel_main

%define KERNEL_TEXT_BASE 0xFFFFFFFF80000000
%define VGABUF 0xB8000

section .bss
align 16
stack_bottom:
resb 16384
stack_top:

%macro outb 2
mov al, %2
    mov dx, %1
    out dx, al
%endmacro

section .text
global _start
_start:
    mov esp, stack_bottom

    call kernel_main
    mov dword [VGABUF + 4], 0x4F4F4F52
    mov dword [VGABUF + 8], 0x4F3A4F52
    mov dword [VGABUF + 12], 0x4F204F20
    mov byte  [VGABUF + 14], al
    call kernel_main
    hlt

    ; Set the cursor to blink
    outb 0x3D4, 0x09
    outb 0x3D5, 0x0F
    outb 0x3D4, 0x0B
    outb 0x3D5, 0x0F
    outb 0x3D4, 0x0A
    outb 0x3D5, 0x0E

    call kernel_main
