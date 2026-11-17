/* ffs() regression check for the M88kFFS pass: ffs(0) must be 0 and ffs(0x80000000) must be 32.
 * Prints one line per input; the last line is "FFS DONE". Expected output: tests/ffs-check.expected
 * (host: cc -DHOST -O2 tests/ffs-check.c). */
#ifdef HOST
#include <stdio.h>
static void out(const char *s) { fputs(s, stdout); }
#else
#include "../ncd19c_console.h"
static void out(const char *s) { ncd_console_puts(s); }
#endif
static void hex(unsigned v) {
  char b[9];
  for (int i = 7; i >= 0; --i, v >>= 4) b[i] = "0123456789abcdef"[v & 15];
  b[8] = 0;
  out(b);
}
__attribute__((noinline)) static int f(int x) { return __builtin_ffs(x); }
int main(void) {
  static const unsigned in[] = {0, 1, 2, 3, 4, 5, 0xa, 0x10000000, 0x20000000, 0x40000000,
                                0x60000000, 0x80000000u, 0x80000001u, 0xffffffffu, 0xfffffff0u};
  for (unsigned i = 0; i < sizeof in / sizeof *in; ++i) {
    volatile unsigned v = in[i];
    out("ffs ");
    hex(v);
    out(" ");
    hex(f((int)v));
    out("\n");
  }
  out("FFS DONE\n");
  return 0;
}
