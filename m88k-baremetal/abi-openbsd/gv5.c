#include <stdarg.h>
struct S { int a, b, c; };
int ov(int a,int b,int c,int d,int e,int f,int g,int h, struct S s, int i, double dd, int j) { return s.c + i + j + (int)dd; }
int ovcall(struct S *p) { return ov(1,2,3,4,5,6,7,8,*p,9,1.0,10); }
int sum(int n, ...) { va_list ap; va_start(ap, n); int t = 0; while (n--) t += va_arg(ap, int); va_end(ap); return t; }
int sumcall(void) { return sum(10, 1,2,3,4,5,6,7,8,9,10); }
