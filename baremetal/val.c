/* Validation program: printf (ints, strings, floats), malloc/free, qsort,
   setjmp/longjmp and 64-bit divide. Build with -DHOST for the expected output. */
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static jmp_buf jb;
static void __attribute__((noinline)) jump(int v) { longjmp(jb, v); }

static int cmp(const void *a, const void *b)
{
	int x = *(const int *)a, y = *(const int *)b;
	return (x > y) - (x < y);
}

int main(void)
{
	printf("int %d %i %u %x %X %o %5d|%-5d|%05d %c\n", -42, 42, 4000000000u,
	       0xbeef, 0xBEEF, 8, 7, 7, 7, 'Z');
	printf("str [%s] [%10s] [%-6s] [%.3s]\n", "hello", "right", "left", "truncate");
	printf("ll %lld %llu %llx\n", -1234567890123LL, 18446744073709551615ULL,
	       0x0123456789abcdefULL);

	uint64_t *a = malloc(1000 * sizeof *a);
	uint64_t sum = 0;
	for (int i = 0; i < 1000; i++) {
		a[i] = 0x0123456789ABCDEFull * (uint64_t)i;
		sum += a[i];
	}
	volatile uint64_t d = 1000003ull;
	volatile int64_t sd = 7;
	int64_t ns = -(int64_t)sum;
	printf("sum %llu\n", (unsigned long long)sum);
	printf("udiv %llu urem %llu\n", (unsigned long long)(sum / d),
	       (unsigned long long)(sum % d));
	printf("sdiv %lld srem %lld\n", (long long)(ns / sd), (long long)(ns % sd));
	free(a);
	char *p = malloc(64);
	printf("heap reuse %s\n", p ? "ok" : "FAIL");
	free(p);

	int v[16];
	unsigned s = 12345;
	for (int i = 0; i < 16; i++) {
		s = s * 1103515245u + 12345u;
		v[i] = (int)((s >> 16) % 1000);
	}
	qsort(v, 16, sizeof v[0], cmp);
	printf("qsort");
	for (int i = 0; i < 16; i++)
		printf(" %d", v[i]);
	printf("\n");

	volatile int n = 0;
	int r = setjmp(jb);
	if (r < 3) {
		n += r;
		jump(r + 1);
	}
	printf("setjmp %d %d\n", r, n);

	volatile double x = 3.14159, y = 2.0;
	volatile float f = 1.5f;
	printf("float %.3f %.2f %.4f %g %e\n", x * y, x / y, (double)(f * f), 0.5,
	       -1e10);
	printf("fint %d %d\n", (int)(x * 100), (int)(-x * 100));
	puts("VAL DONE");
	return 0;
}
