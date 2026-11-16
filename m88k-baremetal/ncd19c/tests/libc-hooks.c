/* Checks the picolibc port hooks: setjmp/longjmp, malloc on sbrk, and
   printf varargs (ints, 64-bit, doubles, stack-passed arguments). */
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static jmp_buf jb;
static volatile int depth;

static void __attribute__((noinline)) jump(int v)
{
	depth++;
	longjmp(jb, v);
}

int main(void)
{
	volatile int n = 0;
	int r = setjmp(jb);
	if (r < 3) {
		n += r;
		jump(r + 1);
	}
	printf("setjmp %d %d %d\n", r, n, depth);
	if (setjmp(jb) == 0)
		jump(0);
	printf("longjmp0 ok\n");

	char *p = malloc(1000), *q;
	strcpy(p, "heap");
	q = malloc(50000);
	printf("malloc %s %d\n", p, q != NULL && q > p);
	free(q);
	free(p);

	printf("ll %lld %llu %llx\n", -1234567890123LL, 18446744073709551615ULL,
	       0x0123456789abcdefULL);
	printf("args %d %d %d %d %d %d %d %lld %d %s\n", 1, 2, 3, 4, 5, 6, 7,
	       8LL, 9, "ten");
	printf("dbl %.3f %g %e\n", 3.14159, 0.5, -1e10);
	return 0;
}
