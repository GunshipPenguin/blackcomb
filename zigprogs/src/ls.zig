const sys = @import("sys.zig");

export fn _start() void {
    const path = "/";
    var fd = sys.open(@ptrCast(*const u8, path));
    _ = sys.close(@bitCast(i32, fd));
    _ = sys.exit(0);
}
