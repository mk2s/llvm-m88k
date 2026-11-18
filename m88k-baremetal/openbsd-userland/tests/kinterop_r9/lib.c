/* KNOWN ISSUE (llvm/lib/Target/M88k/KNOWN_ISSUES.md): when a 64-bit argument no longer fits in a register
   pair, OpenBSD GCC passes it on the stack and still gives the next 32-bit argument r9; Clang does not. */
struct SD { double d; };
double lib_manyd(double a, int b, double c, int d, double e, int f, struct SD s, double g, int h) { return a + b * c + d * e + f + s.d + g * h; }
int lib_bf(double a, int b, double c, int d, double e, int f) { return f + (int)e; }
