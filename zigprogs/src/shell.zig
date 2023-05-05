const sys = @import("sys.zig");
const stdin = 1;

export fn _start() void {
    const ps1 = "$ ";
    var buf = [_]u16{0} ** 32;
    while (true) {
        _ = sys.write(sys.stdout, @ptrCast(*const void, ps1), ps1.len);
        _ = sys.read(sys.stdin, @ptrCast(*void, &buf), buf.len - 1);
        _ = sys.write(sys.stdout, @ptrCast(*void, &buf), buf.len - 1);
    }
}
