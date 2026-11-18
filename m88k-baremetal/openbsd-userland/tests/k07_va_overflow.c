/* KNOWN ISSUE (llvm/lib/Target/M88k/KNOWN_ISSUES.md): a Clang-compiled variadic wrapper whose register words run out
   (named + variadic words > 8, counting double alignment) passes its va_list to libc's vsnprintf. */
#include <stdarg.h>
#include <stdio.h>
static int vwrap(char *buf, unsigned n, const char *fmt, ...) { va_list ap; va_start(ap, fmt); int r = vsnprintf(buf, n, fmt, ap); va_end(ap); return r; }
static int sumi(int n, ...) { va_list ap; va_start(ap, n); int s = 0; while (n--) s += va_arg(ap, int); va_end(ap); return s; }
int main(void) {
  char b[200];
  printf("%d [", vwrap(b, sizeof b, "%d %s %.2f %lld", 5, "abc", 2.5, 99LL)); printf("%s]\n", b);
  vwrap(b, sizeof b, "%d %d %d %d %d %d %d %d %d %d", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10); puts(b);
  vwrap(b, sizeof b, "%g %g %g %g %g", 1.0, 2.0, 3.0, 4.0, 5.0); puts(b);
  printf("%d %d\n", sumi(10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10), sumi(12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12));
  return 0;
}
