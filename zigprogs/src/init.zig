const sys = @import("sys.zig");

export fn _start() void {
    const msg = "init is forking /shell\n";
    const shellPath = "/shell";

    _ = sys.write(sys.stdout, @ptrCast(*const void, msg), msg.len);

    var ret = sys.fork();
    if (ret == 0) {
        _ = sys.exec(@ptrCast(*const u8, shellPath));
    }

    while (true) {}
}
