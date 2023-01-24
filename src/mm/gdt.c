#include "gdt.h"

#include <stdint.h>

#include "defs.h"
#include "kmalloc.h"
#include "string.h"
#include "util.h"

void flush_gdt(uint32_t code, uint32_t data, uint32_t tss);

struct gdtr {
    uint16_t len;
    uint64_t addr;
} __attribute__((packed));

struct tss __tss;

/* Five 8-byte descriptors and one 16-byte one (TSS) */
static uint64_t gdt[7];

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

void encode_tss_gdt_entry(
    uint8_t *target, uint64_t base, uint32_t limit, uint8_t access_byte, uint8_t flags)
{
    /* All fields are the same except base, which is extended to 64 bits */
    encode_gdt_entry(target, 0, limit, access_byte, flags);

    target[2] = base & 0xFF;
    target[3] = (base >> 8) & 0xFF;
    target[4] = (base >> 16) & 0xFF;
    target[7] = (base >> 24) & 0xFF;
    target[8] = (base >> 32) & 0xFF;
    target[9] = (base >> 40) & 0xFF;
    target[10] = (base >> 48) & 0xFF;
    target[11] = (base >> 56) & 0xFF;

    /* Top 4 bytes are reserved */
    target[12] = 0;
    target[13] = 0;
    target[14] = 0;
    target[15] = 0;
}

static void load_gdt()
{
    struct gdtr gdtr;
    gdtr.len = sizeof(gdt);
    gdtr.addr = (uint64_t)&gdt;
    asm("lgdt %0" : : "m"(gdtr));

    flush_gdt(0x8, 0x10, 0x28);
}

void gdt_init()
{
    /* null descriptor */
    encode_gdt_entry((uint8_t *)gdt, 0, 0, 0, 0);

    /* kernel CS */
    encode_gdt_entry((uint8_t *)(gdt + 1), 0, 0xFFFFF, 0x9A, 0xA);

    /* kernel DS */
    encode_gdt_entry((uint8_t *)(gdt + 2), 0, 0xFFFFF, 0x92, 0xC);

    /* user DS */
    encode_gdt_entry((uint8_t *)(gdt + 3), 0, 0xFFFFF, 0xF2, 0xC);

    /* user CS */
    encode_gdt_entry((uint8_t *)(gdt + 4), 0, 0xFFFFF, 0xFA, 0xA);

    /* TSS */
    memset(&__tss, 0, sizeof(__tss));
    encode_tss_gdt_entry((uint8_t *)(gdt + 5), (uint64_t)&__tss, sizeof(__tss), 0x89, 0);

    load_gdt();
}
