/* KNOWN ISSUE (llvm/lib/Target/M88k/KNOWN_ISSUES.md): struct passed through "...". */
#include <stdarg.h>
#include <stdio.h>
struct S { int a, b, c; };
struct D { double d; int i; };
static int get(int n, ...) { va_list ap; va_start(ap, n); struct S s = va_arg(ap, struct S); int x = va_arg(ap, int); va_end(ap); return s.a * 100 + s.b * 10 + s.c + x * 1000 + n; }
static double getd(int n, ...) { va_list ap; va_start(ap, n); struct D d = va_arg(ap, struct D); va_end(ap); return d.d + d.i + n; }
int main(void) {
  struct S s = {1, 2, 3}; struct D d = {1.5, 7};
  printf("%d %g\n", get(5, s, 9), getd(1, d));
  return 0;
}
