extern kernel_main
extern __kernel_start_phys
extern __kernel_end_phys

%define PAGE_SIZE 4096
%define PAGE_PRESENT    (1 << 0)
%define PAGE_WRITE      (1 << 1)

%define CODE_SEG     0x0008
%define DATA_SEG     0x0010

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

section .data.boot
gdt:                          ; Boot GDT, we'll replace this later in the C code but need one to enter long mode
.null:
    dq 0x0000000000000000     ; Null Descriptor - should be present.

.code:
    dq 0x00209A0000000000     ; 64-bit code descriptor (exec/read).
    dq 0x0000920000000000     ; 64-bit data descriptor (read/write).

ALIGN 4
    dw 0                      ; Padding to make the "address of the GDT" field aligned on a 4-byte boundary

.pointer:
    dw $ - gdt - 1            ; 16-bit Size (Limit) of GDT.
    dd  gdt                   ; 32-bit Base Address of GDT. (CPU will zero extend to 64-bit)


; Page tables are set up by the bootstrapping ASM as follows:
; 0000000000000000-0000000000200000 0000000000200000 -rw     -- Identity map of First 2Mib
; ffffffff80000000-ffffffff80200000 0000000000200000 -rw     -- Higher half mapping of first 2Mib
;
; This is enough to enter the C code and initialize the MM, which remaps
; everything using C constructs.
align PAGE_SIZE
page_tables:
.p4:
    resb PAGE_SIZE
.p3_upper:
    resb PAGE_SIZE
.p3_lower:
    resb PAGE_SIZE
.p2_upper:
    resb PAGE_SIZE
.p2_lower:
    resb PAGE_SIZE
.p1:
    resb PAGE_SIZE

align 64
stack_bottom:
resb 16384
stack_top:

section .text.boot
bits 32
global _start

; Kernel entry point, this bootstrapping ASM code sets up a GDT, enables
; paging, and enters long mode, then jumps to kernel_main.
_start:
    ; P4 Entry - Lower
    lea eax, [page_tables.p3_lower]
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [page_tables.p4], eax

    ; P3 Entry - Lower
    lea eax, [page_tables.p2_lower]
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [page_tables.p3_lower], eax

    ; P2 Entry - Lower
    lea eax, [page_tables.p1]
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [page_tables.p2_lower], eax

    ; Higher half mapping @ 0xffffffff80000000
    ; Entry 511 in P4
    ; Entry 510 in P3
    ; Entry 0 in P2

    ; P4 Entry - Upper
    lea eax, [page_tables.p3_upper]
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [page_tables.p4 + (511 * 8)], eax

    ; P3 Entry - Upper
    lea eax, [page_tables.p2_upper]
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [page_tables.p3_upper + (510 * 8)], eax

    ; P2 Entry - Upper
    lea eax, [page_tables.p1]
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [page_tables.p2_upper], eax

    lea edi, [page_tables.p1]
    mov eax, PAGE_PRESENT | PAGE_WRITE

   ; P1 entries (used by both mappings)
.loop_pg_tbl:
    mov [edi], eax
    add eax, 0x1000
    add edi, 8
    cmp eax, 0x200000 ; If we did all 2MiB, end.
    jb .loop_pg_tbl

    ; Enter long mode, set the PAE and PGE bits
    mov eax, 10100000b
    mov cr4, eax

    ; Point CR3 at the PML4.
    lea edx, [page_tables.p4]
    mov cr3, edx

    ; Set LME bit in EFER MSR
    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x00000100
    wrmsr

    ; Activate long mode, enable paging and protection simultaneously
    mov ecx, cr0
    or ecx, 0x80000001
    mov cr0, ecx

    ; Load 64 bit GDT and longjmp to load CS
    lgdt [gdt.pointer]
    jmp 0x08:long_mode

bits 64
long_mode:
    ; Load data segments with 64 bit GDT entries
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Setup stack and jump to C kernel_main
    mov rsp, stack_top
    mov rdi, rbx
    call kernel_main
