const sys = @import("sys.zig");
const stdin = 1;

export fn _start() void {
    const msg = "Hello World!\n";
    _ = sys.write(sys.stdout, @ptrCast(*const void, msg), msg.len);
    _ = sys.exit(0);
}
