#include "gdt.h"

#include <stdint.h>

#include "defs.h"
#include "kmalloc.h"
#include "util.h"

void flush_gdt(uint32_t code, uint32_t data);

struct gdtr {
    uint16_t len;
    uint32_t addr;
} __attribute__((packed));

static uint64_t gdt[5];

static void
encode_gdt_entry(uint8_t *target, uint32_t base, uint32_t limit, uint8_t access_byte, uint8_t flags)
{
    if (limit > 0xFFFFF)
        panic("GDT cannot encode limits larger than 0xFFFFF");

    /* limit */
    target[0] = limit & 0xFF;
    target[1] = (limit >> 8) & 0xFF;
    target[6] = (limit >> 16) & 0x0F;

    /* base */
    target[2] = base & 0xFF;
    target[3] = (base >> 8) & 0xFF;
    target[4] = (base >> 16) & 0xFF;
    target[7] = (base >> 24) & 0xFF;

    /* access byte */
    target[5] = access_byte;

    /* flags */
    target[6] |= (flags << 4);
}

static void load_gdt()
{
    struct gdtr gdtr;
    gdtr.len = sizeof(gdt);
    gdtr.addr = (uintptr_t)&gdt;
    asm("lgdt %0" : : "m"(gdtr));

    flush_gdt(0x8, 0x10);
}

void gdt_init()
{
    /* null descriptor */
    encode_gdt_entry((uint8_t *)gdt, 0, 0, 0, 0);

    /* kernel CS */
    encode_gdt_entry((uint8_t *)(gdt + 1), 0, 0xFFFFF, 0x9A, 0xC);

    /* kernel DS */
    encode_gdt_entry((uint8_t *)(gdt + 2), 0, 0xFFFFF, 0x92, 0xC);

    /* user CS */
    encode_gdt_entry((uint8_t *)(gdt + 3), 0, 0xFFFFF, 0xFA, 0xC);

    /* user DS */
    encode_gdt_entry((uint8_t *)(gdt + 4), 0, 0xFFFFF, 0xF2, 0xC);

    /* TSS */
    // encode_gdt_entry(gdt + 5, 0, 0, 0, 0);

    load_gdt();
}
