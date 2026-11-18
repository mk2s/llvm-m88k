#include <stdio.h>
#include <string.h>
struct S1 { char c; };
struct S3 { char c[3]; };
struct S8 { int a, b; };
struct S12 { int a, b, c; };
struct SD { double d; };
struct SM { char c; double d; short s; };
struct SA { int a[10]; };
struct SF { float f; int i; };
union U { int i; float f; };
static struct S1 f1(struct S1 s, int k) { s.c += k; return s; }
static struct S3 f3(struct S3 s, struct S3 t) { struct S3 r; for (int i = 0; i < 3; i++) r.c[i] = s.c[i] + t.c[i]; return r; }
static struct S8 f8(int x, struct S8 s, int y) { s.a += x; s.b += y; return s; }
static struct S12 f12(struct S12 s, double d, struct S12 t, long long l) { s.a += t.a + (int)d; s.b += t.b + (int)l; s.c += t.c; return s; }
static struct SD fd(struct SD s, int i, struct SD t) { s.d = s.d * i + t.d; return s; }
static struct SM fm(struct SM s, struct SM t) { s.c += t.c; s.d += t.d; s.s += t.s; return s; }
static struct SA fa(struct SA s, int k, struct SA t) { struct SA r; for (int i = 0; i < 10; i++) r.a[i] = s.a[i] + t.a[i] * k; return r; }
static struct SF ff(float f, struct SF s, float g) { s.f += f + g; s.i += 1; return s; }
static union U fu(union U u, int k) { u.i += k; return u; }
static int many(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j, struct S12 s, int k) { return a + 2*b + 3*c + 4*d + 5*e + 6*f + 7*g + 8*h + 9*i + 10*j + s.a + s.b + s.c + 100*k; }
static double manyd(double a, int b, double c, int d, double e, int f, struct SD s, double g, int h) { return a + b * c + d * e + f + s.d + g * h; }
int main(void) {
  struct S1 a1 = {'a'}; a1 = f1(a1, 2); printf("%c\n", a1.c);
  struct S3 a3 = {{1, 2, 3}}, b3 = {{10, 20, 30}}; a3 = f3(a3, b3); printf("%d %d %d\n", a3.c[0], a3.c[1], a3.c[2]);
  struct S8 a8 = {1, 2}; a8 = f8(10, a8, 20); printf("%d %d\n", a8.a, a8.b);
  struct S12 a12 = {1, 2, 3}, b12 = {4, 5, 6}; a12 = f12(a12, 2.9, b12, 7000000000LL); printf("%d %d %d\n", a12.a, a12.b, a12.c);
  struct SD ad = {1.5}, bd = {0.25}; ad = fd(ad, 3, bd); printf("%g\n", ad.d);
  struct SM am = {1, 2.5, 3}, bm = {10, 0.5, 30}; am = fm(am, bm); printf("%d %g %d\n", am.c, am.d, am.s);
  struct SA aa, ba; for (int i = 0; i < 10; i++) { aa.a[i] = i; ba.a[i] = 100 + i; } aa = fa(aa, 2, ba);
  for (int i = 0; i < 10; i++) printf("%d ", aa.a[i]); printf("\n");
  struct SF af = {1.5f, 4}; af = ff(0.25f, af, 0.5f); printf("%g %d\n", (double)af.f, af.i);
  union U u; u.i = 5; u = fu(u, 3); printf("%d\n", u.i);
  printf("%d\n", many(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, a12, 3));
  struct SD sd = {0.5}; printf("%g\n", manyd(1.0, 2, 3.0, 4, 5.0, 6, sd, 7.0, 8));
  return 0;
}
