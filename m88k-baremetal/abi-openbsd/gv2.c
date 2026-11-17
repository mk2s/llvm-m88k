#include <stdarg.h>
int get_int(va_list ap) { return va_arg(ap, int); }
double get_dbl(va_list ap) { return va_arg(ap, double); }
int start1(int n, ...) { va_list ap; va_start(ap, n); int r = va_arg(ap, int); va_end(ap); return r; }
int start0(int n, int m, ...) { va_list ap; va_start(ap, m); int r = va_arg(ap, int); va_end(ap); return r; }
struct S { int a, b, c; };
struct S ret(struct S *p) { return *p; }
int use(struct S s, int x);
int call(struct S *p, int x) { return use(*p, x); }
int callee(struct S s, int x) { return s.b + x; }
