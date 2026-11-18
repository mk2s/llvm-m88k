extern fn ncd_console_puts(s: [*:0]const u8) void;
extern fn ncd_console_putc(c: u8) void;

fn putHex(v: u32) void {
    var i: i32 = 28;
    while (i >= 0) : (i -= 4) {
        ncd_console_putc("0123456789abcdef"[(v >> @intCast(i)) & 15]);
    }
}

fn fib(n: u32) u32 {
    var a: u32 = 0;
    var b: u32 = 1;
    var i: u32 = 0;
    while (i < n) : (i += 1) {
        const t = a +% b;
        a = b;
        b = t;
    }
    return a;
}

export fn zig_main() c_int {
    ncd_console_puts("zig ok\nfib(20)=0x");
    putHex(fib(20));
    ncd_console_puts("\n");
    const big: u64 = 0x123456789 * 1000;
    putHex(@truncate(big / 7));
    ncd_console_puts("\n");
    return 0;
}
