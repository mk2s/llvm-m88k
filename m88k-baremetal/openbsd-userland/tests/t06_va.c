#include <stdarg.h>
#include <stdio.h>
static int sumi(int n, ...) { va_list ap; va_start(ap, n); int s = 0; while (n--) s += va_arg(ap, int); va_end(ap); return s; }
static double sumd(int n, ...) { va_list ap; va_start(ap, n); double s = 0; while (n--) s += va_arg(ap, double); va_end(ap); return s; }
static long long suml(int n, ...) { va_list ap; va_start(ap, n); long long s = 0; while (n--) s += va_arg(ap, long long); va_end(ap); return s; }
static void mixed(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  for (; *fmt; fmt++) switch (*fmt) {
    case 'i': printf("i%d ", va_arg(ap, int)); break;
    case 'l': printf("l%lld ", va_arg(ap, long long)); break;
    case 'd': printf("d%g ", va_arg(ap, double)); break;
    case 'p': printf("p%s ", va_arg(ap, char *)); break;
    case 'c': printf("c%c ", va_arg(ap, int)); break;
  }
  va_end(ap); printf("\n");
}
static int vwrap(char *buf, unsigned n, const char *fmt, ...) { va_list ap; va_start(ap, fmt); int r = vsnprintf(buf, n, fmt, ap); va_end(ap); return r; }
int main(void) {
  printf("%d %d %d\n", sumi(3, 1, 2, 3), sumi(6, 1, 2, 3, 4, 5, 6), sumi(0));
  printf("%g %g\n", sumd(2, 1.5, 2.5), sumd(3, 0.5, 0.25, 0.125));
  printf("%lld %lld\n", suml(2, 10000000000LL, -3LL), suml(3, 1LL, 2LL, 3LL));
  mixed("ilidpc", 1, 2LL, 3, 4.5, "five", 'x');
  mixed("dd", 1.0, 2.0);
  char b[100]; int r = vwrap(b, sizeof b, "%c%c%c %x", 'a', 'b', 'c', 255); printf("%d [%s]\n", r, b);
  r = vwrap(b, sizeof b, "%d %d", 1, 2); printf("%d [%s]\n", r, b);
  return 0;
}
