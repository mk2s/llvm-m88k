struct S { int a, b, c; };
struct T { int a, b; };
int use(struct S s, int x);
int call(struct S *p, int x) { return use(*p, x); }
int callee(struct S s, int x) { return s.b + x; }
int sd(struct T t, double d, int c) { return t.b + c + (int)d; }
int sdcall(struct T *t) { return sd(*t, 2.0, 3); }
struct S ret(struct S *p) { return *p; }
