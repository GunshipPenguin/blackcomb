const sys = @import("sys.zig");

export fn _start() void {
    const ps1 = "$ ";
    var buf = [_]u8{0} ** 32;

    while (true) {
        _ = sys.write(sys.stdout, @ptrCast(*const void, ps1), ps1.len);

        @memset(&buf, 0, buf.len);
        _ = sys.read(sys.stdin, @ptrCast(*void, &buf), buf.len - 1);

        if (sys.fork() == 0) {
            _ = sys.exec(@ptrCast(*const u8, &buf));
        }

        var wstatus: u32 = 0;
        _ = sys.wait(&wstatus);
    }
}
