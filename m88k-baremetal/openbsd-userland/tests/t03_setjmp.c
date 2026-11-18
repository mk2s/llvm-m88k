#include <setjmp.h>
#include <stdio.h>
static jmp_buf jb;
static int depth(int n) { if (n == 0) longjmp(jb, 42); return depth(n - 1) + 1; }
int main(void) {
  volatile int count = 0;
  int r = setjmp(jb);
  if (r == 0) { count++; depth(50); printf("not reached\n"); }
  printf("r=%d count=%d\n", r, (int)count);
  jmp_buf b2; volatile int k = 0;
  if (setjmp(b2) < 3) { k++; longjmp(b2, k + 1); }
  printf("k=%d\n", (int)k);
  return 0;
}
