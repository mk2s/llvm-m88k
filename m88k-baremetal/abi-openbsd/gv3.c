int stk(int a,int b,int c,int d,int e,int f,int g,int h,int i,int j);
int stk2(int a,int b,int c,int d,int e,int f,int g,int h,int i,int j) { return i + j; }
int callstk(void) { return stk(1,2,3,4,5,6,7,8,9,10); }
long long ll(int a, long long b, int c) { return b + c; }
long long llcall(void) { return ll(1, 2, 3); }
struct T { int a, b; };
int sd(struct T t, double d, int c) { return t.b + c + (int)d; }
