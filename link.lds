ENTRY(_start)

PAGE_SIZE = 0x1000;

PHYS = 0x100000;
KERNEL_TEXT_BASE = 0xffffffff80000000;

SECTIONS
{
    . = PHYS;
    __kernel_start_phys = .;

    .multiboot : {
        *(.multiboot);
    }

    .text.boot : {
        *(.text.boot);
    }

    .data.boot : {
        *(.data.boot);
    }

    . += KERNEL_TEXT_BASE;

    .kernelstack ALIGN(PAGE_SIZE) : AT(ADDR(.kernelstack) - KERNEL_TEXT_BASE) {
        *(.kernelstack);
    }

    .text ALIGN(PAGE_SIZE) : AT(ADDR(.text) - KERNEL_TEXT_BASE) {
        *(.text);
    }

    .rodata ALIGN(PAGE_SIZE) : AT(ADDR(.rodata) - KERNEL_TEXT_BASE) {
        *(.rodata)
    }

    .data ALIGN(PAGE_SIZE) : AT(ADDR(.data) - KERNEL_TEXT_BASE) {
        *(.data)
    }

    .bss ALIGN(PAGE_SIZE) : AT(ADDR(.bss) - KERNEL_TEXT_BASE) {
        *(.bss)
    }

    __kernel_end_phys = . - KERNEL_TEXT_BASE;
}
