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
gdt:
.null:
    dq 0x0000000000000000             ; Null Descriptor - should be present.

.code:
    dq 0x00209A0000000000             ; 64-bit code descriptor (exec/read).
    dq 0x0000920000000000             ; 64-bit data descriptor (read/write).

ALIGN 4
    dw 0                              ; Padding to make the "address of the GDT" field aligned on a 4-byte boundary

.pointer:
    dw $ - gdt - 1                    ; 16-bit Size (Limit) of GDT.
    dd  gdt                           ; 32-bit Base Address of GDT. (CPU will zero extend to 64-bit)

section .data.boot
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

section .text.boot
bits 32
global _start
_start:
    ; PML4 Entry - Lower
    lea eax, [page_tables.p3_lower]
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [page_tables.p4], eax

    ; PDPT Entry - Lower
    lea eax, [page_tables.p2_lower]
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [page_tables.p3_lower], eax

    ; Page Directory Entry - Lower
    lea eax, [page_tables.p1]
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [page_tables.p2_lower], eax

    ; 0xffffffff80000000:
    ; Entry 511 in PML4
    ; Entry 508 in PDPT
    ; Entry 0 in Page Directory

    ; PML4 Entry - Upper
    lea eax, [page_tables.p3_upper]
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [page_tables.p4 + (511 * 8)], eax

    ; PDPT Entry - Upper
    lea eax, [page_tables.p2_upper]
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [page_tables.p3_upper + (510 * 8)], eax

    ; Page Directory Entry - Upper
    lea eax, [page_tables.p1]
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [page_tables.p2_upper], eax

    lea edi, [page_tables.p1]
    mov eax, PAGE_PRESENT | PAGE_WRITE

   ; Build the Page Table.
.loop_pg_tbl:
    mov [edi], eax
    add eax, 0x1000
    add edi, 8
    cmp eax, 0x200000                 ; If we did all 2MiB, end.
    jb .loop_pg_tbl

    ; Enter long mode.
    mov eax, 10100000b                ; Set the PAE and PGE bit.
    mov cr4, eax

    lea edx, [page_tables.p4]            ; Point CR3 at the PML4.
    mov cr3, edx

    mov ecx, 0xC0000080               ; Read from the EFER MSR.
    rdmsr

    or eax, 0x00000100                ; Set the LME bit.
    wrmsr

    mov ebx, cr0                      ; Activate long mode -
    or ebx, 0x80000001                ; - by enabling paging and protection simultaneously.
    mov cr0, ebx

    lgdt [gdt.pointer]                ; Load GDT.Pointer defined below.

    jmp 0x08:LongMode             ; Load CS with 64 bit segment and flush the instruction cache

bits 64
LongMode:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Blank out the screen to a blue color.
    mov edi, 0xB8000
    mov rcx, 500                      ; Since we are clearing uint64_t over here, we put the count as Count/4.
    mov rax, 0x1F201F201F201F20       ; Set the value to set the screen to: Blue background, white foreground, blank spaces.
    rep stosq                         ; Clear the entire screen.

    ; Display "Hello World!"
    mov edi, 0x00b8000

    mov rax, 0x1F6C1F6C1F651F48
    mov [edi],rax

    mov rax, 0x1F6F1F571F201F6F
    mov [edi + 8], rax

    mov rax, 0x1F211F641F6C1F72
    mov [edi + 16], rax

    jmp kernel_main
