#include "io.h"

#include <stdbool.h>
#include <stdint.h>

#include "printf.h"
#include "sched.h"
#include "util.h"
#include "vgaterm.h"

#define PIC1_REMAP_BASE 32
#define PIC2_REMAP_BASE 40

#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1

#define PIC_EOI 0x20

#define ICW1_ICW4 0x01      /* ICW4 (not) needed */
#define ICW1_SINGLE 0x02    /* Single (cascade) mode */
#define ICW1_INTERVAL4 0x04 /* Call address interval 4 (8) */
#define ICW1_LEVEL 0x08     /* Level triggered (edge) mode */
#define ICW1_INIT 0x10      /* Initialization */

#define ICW4_8086 0x01       /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO 0x02       /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE 0x08  /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C /* Vuffered mode/master */
#define ICW4_SFNM 0x10       /* Special fully nested (not) */

#define IDT_MAX_DESCRIPTORS 256

struct isr_ctx {
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rax;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;

    /* Error code/interrupt number for exceptions */
    uint64_t info;

    /* Interrupt stack frame */
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

struct idt_entry {
    uint16_t offset_1;
    uint16_t selector;
    uint8_t ist;
    uint8_t attr;
    uint16_t offset_2;
    uint32_t offset_3;
    uint32_t reserved;
} __attribute__((packed));

struct idtr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

__attribute__((aligned(0x10))) static struct idt_entry idt[IDT_MAX_DESCRIPTORS];
static int vectors[IDT_MAX_DESCRIPTORS];
extern void *isr_stub_table[];

void *__interrupt_stack;

static inline void io_wait(void)
{
    outb(0x80, 0);
}

static void send_eoi(uint8_t irq)
{
    if (irq >= PIC2_REMAP_BASE)
        outb(PIC2_COMMAND, PIC_EOI);

    outb(PIC1_COMMAND, PIC_EOI);
}

void isr_main_entry(struct isr_ctx *ctx)
{
    uint32_t vec = (ctx->info >> 32) & 0xFFFF;
    uint32_t err = ctx->info & 0xFFFF;

    if (vec == 0xD)
        panic("General protection fault");

    if (vec == 0xE)
        panic("Page fault");

    bool switched;
    if (vec == 0x20)
        switched = sched_maybe_preempt();

    //    printf("Interrupt %d, error %d\n", vec, err);
    send_eoi(vec);

    if (switched)
        enter_usermode();
}

static void idt_set_descriptor(uint8_t vector, void *isr, uint8_t attr)
{
    struct idt_entry *descriptor = &idt[vector];

    descriptor->offset_1 = (uint64_t)isr & 0xFFFF;
    descriptor->offset_2 = ((uint64_t)isr >> 16) & 0xFFFF;
    descriptor->offset_3 = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
    descriptor->selector = 0x08;
    descriptor->attr = attr;
    descriptor->ist = 0;
    descriptor->reserved = 0;
}

void remap_pic(int pic1_offset, int pic2_offset)
{
    /* save masks */
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    /* start initialization sequence */
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    outb(PIC1_DATA, pic1_offset);
    io_wait();
    outb(PIC2_DATA, pic2_offset);
    io_wait();
    outb(PIC1_DATA, 4);
    io_wait();
    outb(PIC2_DATA, 2);
    io_wait();

    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    /* restore saved masks */
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

void idt_init()
{
    struct idtr idtr;
    idtr.base = (uint64_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(struct idt_entry) * IDT_MAX_DESCRIPTORS - 1;

    for (uint8_t vector = 0; vector < 48; vector++) {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
        vectors[vector] = 1;
    }

    remap_pic(PIC1_REMAP_BASE, PIC2_REMAP_BASE);

    asm volatile("lidt %0" : : "m"(idtr));
    //    asm volatile("sti");
}
