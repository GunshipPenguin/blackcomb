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
extern __kernel_start
extern __kernel_end

%define KERNEL_TEXT_BASE 0xC0000000
%define VGABUF 0xB8000
%define PAGE_SIZE 4096

section .bss
align PAGE_SIZE
boot_page_directory:
resb PAGE_SIZE
boot_page_table1:
resb PAGE_SIZE

align 16
stack_bottom:
resb 16384
stack_top:

section .text.boot
global _start
_start:
    mov edi, boot_page_table1 - KERNEL_TEXT_BASE
    mov esi, 0
    mov ecx, 1023

map_kernel_loop:
    cmp esi, __kernel_start
    jl next_page

    cmp esi, __kernel_end - KERNEL_TEXT_BASE
    jge done_mapping_kernel

    mov edx, esi
    or edx, 0x3
    mov [edi], edx

next_page:
    add esi, PAGE_SIZE
    add edi, 4
    loop map_kernel_loop

done_mapping_kernel:
    mov DWORD [(boot_page_table1 - KERNEL_TEXT_BASE) + (1023 * 4)], 0xB8003

    mov DWORD [boot_page_directory - KERNEL_TEXT_BASE], boot_page_table1 - KERNEL_TEXT_BASE + 0x3
    mov DWORD [boot_page_directory - KERNEL_TEXT_BASE + 768 * 4], boot_page_table1 - KERNEL_TEXT_BASE + 0x3

    mov ecx, boot_page_directory - KERNEL_TEXT_BASE
    mov cr3, ecx

    mov ecx, cr0
    or ecx, 0x80010000
    mov cr0, ecx

    lea ecx, jump_to_main
    jmp ecx

section .text
jump_to_main:
    mov DWORD [boot_page_directory + 0], 0
    mov ecx, cr3
    mov cr3, ecx

    mov esp, stack_top
    push ebx
    call kernel_main

    cli
    hlt

err:
    mov dword [VGABUF + 4], 0x4F4F4F52
    mov dword [VGABUF + 8], 0x4F3A4F52
    mov dword [VGABUF + 12], 0x4F204F20
    mov byte  [VGABUF + 14], al
    hlt
