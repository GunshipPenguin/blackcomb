const sys = @import("sys.zig");

const dirent = extern struct { ino: u32, rec_len: u32 };

fn strlen(s: *const u8) usize {
    var c: usize = 0;
    while (@intToPtr(*u8, @ptrToInt(s) + c).* != '\x00') {
        c += 1;
    }

    return c;
}

export fn _start() void {
    const path = "/";
    var buf: [256]u8 = [_]u8{0} ** 256;

    var fd = sys.open(@ptrCast(*const u8, path));

    var sz = sys.getdents(fd, @ptrCast(*u8, &buf), buf.len);
    var de = @ptrCast(*align(1) dirent, &buf);

    while (sz > 0) {
        var name: *const u8 = @intToPtr(*const u8, @ptrToInt(de) + @sizeOf(dirent));

        _ = sys.write(sys.stdout, @ptrCast(*const void, name), strlen(name));
        _ = sys.write(sys.stdout, @ptrCast(*const void, "\n"), 1);

        sz -= @intCast(i32, de.rec_len);
        de = @intToPtr(*align(1) dirent, @ptrToInt(de) + de.rec_len);
    }

    _ = sys.close(@bitCast(i32, fd));
    _ = sys.exit(0);
}
