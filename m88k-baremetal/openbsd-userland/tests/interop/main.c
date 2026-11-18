#include <stdio.h>
struct S3 { char c[3]; };
struct S12 { int a, b, c; };
struct SD { double d; };
struct SM { char c; double d; short s; };
struct SA { int a[10]; };
struct S3 lib_s3(struct S3, struct S3);
struct S12 lib_s12(int, struct S12, double, struct S12, long long);
struct SD lib_sd(struct SD, int, struct SD);
struct SM lib_sm(struct SM, struct SM);
struct SA lib_sa(struct SA, int, struct SA);
int lib_many(int, int, int, int, int, int, int, int, int, int, struct S12, int);
long long lib_ll(int, long long, int, long long, double);
int lib_callback(int (*)(int, struct S12, double), int);
static int cb(int n, struct S12 s, double d) { return n * 1000 + s.a + s.b * 10 + s.c * 100 + (int)(d * 2); }
int main(void) {
  struct S3 a3 = {{1, 2, 3}}, b3 = {{10, 20, 30}}; a3 = lib_s3(a3, b3); printf("%d %d %d\n", a3.c[0], a3.c[1], a3.c[2]);
  struct S12 a12 = {1, 2, 3}, b12 = {4, 5, 6}; a12 = lib_s12(100, a12, 2.9, b12, 7000000000LL); printf("%d %d %d\n", a12.a, a12.b, a12.c);
  struct SD ad = {1.5}, bd = {0.25}; ad = lib_sd(ad, 3, bd); printf("%g\n", ad.d);
  struct SM am = {1, 2.5, 3}, bm = {10, 0.5, 30}; am = lib_sm(am, bm); printf("%d %g %d\n", am.c, am.d, am.s);
  struct SA aa, ba; for (int i = 0; i < 10; i++) { aa.a[i] = i; ba.a[i] = 100 + i; } aa = lib_sa(aa, 2, ba);
  for (int i = 0; i < 10; i++) printf("%d ", aa.a[i]); printf("\n");
  printf("%d\n", lib_many(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, a12, 3));
  printf("%lld\n", lib_ll(3, 5000000000LL, 4, -7LL, 2.5));
  printf("%d\n", lib_callback(cb, 7));
  return 0;
}
