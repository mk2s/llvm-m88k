#include <stdarg.h>
struct S { int a, b, c; };
struct T { int a, b; };
int get(int n, ...) { va_list ap; va_start(ap, n); struct S s = va_arg(ap, struct S); va_end(ap); return s.c; }
int put(struct S *p) { return get(1, *p); }
int recv(struct S s, struct T t, int x) { return s.c + t.b + x; }
int send(struct S *p, struct T *q) { return recv(*p, *q, 7); }
int stk(int a,int b,int c,int d,int e,int f,int g,int h,int i,int j) { return i + j; }
int callstk(void) { return stk(1,2,3,4,5,6,7,8,9,10); }
struct S ret(struct S *p) { return *p; }
