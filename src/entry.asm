extern kernel_main
extern __kernel_start
extern __kernel_end

%define KERNEL_TEXT_BASE 0xC0000000
%define PAGE_SIZE 4096

section .multiboot
align 64
header_start:
    dd 0xE85250D6                ; Magic number (multiboot 2)
    dd 0                         ; Architecture 0 (protected mode i386)
    dd header_end - header_start ; Header length
    dd 0x100000000 - (0xE85250D6 + 0 + (header_end - header_start)) ; Checksum
    dw 0
    dw 0
    dd 8
header_end:

; Small area for a bootstrap page table, just enough to map the kernel text and multiboot info
; and call kernel_main, we'll replace this shortly
section .bss
align PAGE_SIZE
boot_page_directory:
resb PAGE_SIZE
boot_page_table1:
resb PAGE_SIZE
boot_page_table2:
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

; Setup page table for kernel to map 0x00100000 to virtual address 0xC0100000
map_kernel_loop:
    cmp esi, __kernel_start
    jl kern_nextpage

    cmp esi, __kernel_end - KERNEL_TEXT_BASE
    jge map_mboot_info

    mov edx, esi
    or edx, 0x3
    mov [edi], edx

kern_nextpage:
    add esi, PAGE_SIZE
    add edi, 4
    loop map_kernel_loop

; Setup page table for multiboot information to map [ebx] to 0xA0000000
map_mboot_info:
    mov eax, DWORD [ebx]                         ; eax = size of multiboot information
    mov esi, ebx                                 ; esi = Current physical address
    mov edi, boot_page_table2 - KERNEL_TEXT_BASE ; edi = address of current PTE

    ; Page align the physical address down, ebx is only guaranteed to be 8-byte aligned as per
    ; the multiboot 2 spec
    and edi, 0xFFFFE000

    ; We need to map up to an extra 4096 bytes due to the rounding, so add 4096 to the size
    add eax, PAGE_SIZE

map_mboot_loop:
    ; eax = size remaining: Have we already mapped everything?
    cmp eax, 0
    jl mapping_done
    sub eax, PAGE_SIZE

    ; No? Ok, map another page
    mov edx, esi
    or edx, 0x3 ; edx = physical address | 0x3 (read/write)
    mov [edi], edx

mb_nextpage:
    add esi, PAGE_SIZE
    add edi, 4
    jmp map_mboot_loop

; Kernel / multiboot info mapping done, insert entries in page directories
mapping_done:
    ; Map VGA video memory to 0xC03FF000 via the last entry of the text page table
    mov DWORD [(boot_page_table1 - KERNEL_TEXT_BASE) + (1023 * 4)], 0xB8003

    ; Insert page directory entries

    ; Identity map the kernel text so we can execute the next instruction when we enable paging
    mov DWORD [boot_page_directory - KERNEL_TEXT_BASE], boot_page_table1 - KERNEL_TEXT_BASE + 0x3

    ; Map kernel text to 0xC0100000
    mov DWORD [boot_page_directory - KERNEL_TEXT_BASE + 768 * 4], boot_page_table1 - KERNEL_TEXT_BASE + 0x3

    ; Map multiboot information to 0xA0000000
    mov DWORD [boot_page_directory - KERNEL_TEXT_BASE + 640 * 4], boot_page_table2 - KERNEL_TEXT_BASE + 0x3

    mov ecx, boot_page_directory - KERNEL_TEXT_BASE
    mov cr3, ecx

    mov ecx, cr0
    or ecx, 0x80010000
    mov cr0, ecx

    lea ecx, [jump_to_main]
    jmp ecx

section .text
jump_to_main:
    ; Unmap the identity map, we don't need it anymore
    mov DWORD [boot_page_directory + 0], 0
    mov ecx, cr3
    mov cr3, ecx

    mov esp, stack_top

    push ebx
    call kernel_main
