#ifndef __GDT_H__
#define __GDT_H__

#include <stdint.h>

struct tss {
    uint32_t resv1;

    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t rsp3;

    uint64_t resv2;

    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;

    uint64_t resv3;
    uint32_t resv4;
    uint16_t iopb;
} __attribute__((packed));

extern struct tss __tss;
void gdt_init();

#endif /* __GDT_H__ */
