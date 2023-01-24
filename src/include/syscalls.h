#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

void syscall_enable();
void initial_enter_usermode(uint64_t rip, uint64_t rsp, uint64_t cr3);

#endif /* __SYSCALLS_H__ */
