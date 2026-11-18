/* compiled by one compiler; main.c by the other.
   Arrays inside struct parameters are not modified in place: OpenBSD GCC 3.3.6 returns the
   unmodified copy when a loop updates a by-value struct parameter (all -O levels). */
struct S3 { char c[3]; };
struct S12 { int a, b, c; };
struct SD { double d; };
struct SM { char c; double d; short s; };
struct SA { int a[10]; };
struct S3 lib_s3(struct S3 s, struct S3 t) { struct S3 r; for (int i = 0; i < 3; i++) r.c[i] = s.c[i] + t.c[i]; return r; }
struct S12 lib_s12(int x, struct S12 s, double d, struct S12 t, long long l) { s.a += t.a + (int)d + x; s.b += t.b + (int)l; s.c += t.c; return s; }
struct SD lib_sd(struct SD s, int i, struct SD t) { s.d = s.d * i + t.d; return s; }
struct SM lib_sm(struct SM s, struct SM t) { s.c += t.c; s.d += t.d; s.s += t.s; return s; }
struct SA lib_sa(struct SA s, int k, struct SA t) { struct SA r; for (int i = 0; i < 10; i++) r.a[i] = s.a[i] + t.a[i] * k; return r; }
int lib_many(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j, struct S12 s, int k) { return a + 2*b + 3*c + 4*d + 5*e + 6*f + 7*g + 8*h + 9*i + 10*j + s.a + s.b + s.c + 100*k; }
long long lib_ll(int a, long long b, int c, long long d, double e) { return a + b * c + d + (long long)e; }
int lib_callback(int (*cb)(int, struct S12, double), int n) { struct S12 s = {n, n + 1, n + 2}; return cb(n, s, n * 0.5) + 1; }
