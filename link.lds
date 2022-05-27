ENTRY(_start)

SECTIONS
{
    . = 1M;
    . = ALIGN(8);

    .text : {
        *(.multiboot);
        *(.text);
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
    }
}
