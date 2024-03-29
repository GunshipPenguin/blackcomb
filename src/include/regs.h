#ifndef __REGS_H__
#define __REGS_H__

#include <stdint.h>
#include "printf.h"

struct regs {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;

    uint64_t rbp;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;

    uint64_t rip; /* Loaded into rcx for sysret */
    uint64_t rsp;
} __attribute__((packed));

#endif /* __REGS_H__ */
