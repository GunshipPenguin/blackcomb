pub const stdin = 0;
pub const stdout = 1;

const sys_read = 0;
const sys_write = 1;
const sys_fork = 2;
const sys_exec = 3;

pub fn syscall0(number: usize) usize {
    return asm volatile ("syscall"
        : [ret] "={rax}" (-> usize),
        : [number] "{rax}" (number),
        : "rcx", "r11"
    );
}

pub fn syscall1(number: usize, arg1: usize) usize {
    return asm volatile ("syscall"
        : [ret] "={rax}" (-> usize),
        : [number] "{rax}" (number),
          [arg1] "{rdi}" (arg1),
        : "rcx", "r11"
    );
}

pub fn syscall2(number: usize, arg1: usize, arg2: usize) usize {
    return asm volatile ("syscall"
        : [ret] "={rax}" (-> usize),
        : [number] "{rax}" (number),
          [arg1] "{rdi}" (arg1),
          [arg2] "{rsi}" (arg2),
        : "rcx", "r11"
    );
}

pub fn syscall3(number: usize, arg1: usize, arg2: usize, arg3: usize) usize {
    return asm volatile ("syscall"
        : [ret] "={rax}" (-> usize),
        : [number] "{rax}" (number),
          [arg1] "{rdi}" (arg1),
          [arg2] "{rsi}" (arg2),
          [arg3] "{rdx}" (arg3),
        : "rcx", "r11"
    );
}

pub fn read(fd: i64, buf: *void, count: usize) usize {
    return syscall3(sys_read, @bitCast(usize, fd), @ptrToInt(buf), count);
}

pub fn write(fd: i64, buf: *const void, count: usize) usize {
    return syscall3(sys_write, @bitCast(usize, fd), @ptrToInt(buf), count);
}

pub fn fork() i64 {
    return @bitCast(i64, syscall0(sys_fork));
}

pub fn exec(path: *const u8) i64 {
    return @bitCast(i64, syscall1(sys_exec, @ptrToInt(path)));
}
