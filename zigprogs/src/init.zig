const sys = @import("sys.zig");

export fn _start() void {
    const msg = "[INIT] forking/executing /sh\n";
    const shPath = "/sh";

    _ = sys.write(sys.stdout, @ptrCast(*const void, msg), msg.len);

    var ret = sys.fork();
    if (ret == 0) {
        _ = sys.exec(@ptrCast(*const u8, shPath));
    }

    while (true) {}
}
