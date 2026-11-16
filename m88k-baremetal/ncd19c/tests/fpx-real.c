/* Real-hardware check of the SFU1 exception handler (ncd19c_fpx.[Sc]). Denormal, infinity and NaN
 * operands, division by zero and integer-conversion overflow make a real MC88100 take precise
 * exceptions (vector 114); the handler completes the instruction in software. MAME never raises
 * them, so in MAME this program only checks that the code runs and prints the expected text.
 * Netboot it on the real unit as 0A000207 and compare with tests/fpx-real.expected. The test
 * is "not verified in MAME". Expected output from the host:
 *   cc -DHOST -O0 -o /tmp/fpx-real-host tests/fpx-real.c && /tmp/fpx-real-host
 */
#include <stdint.h>
#include <stdio.h>
static void pd(const char *name, double d)
{
	union { double d; uint64_t u; } x = {d};
	printf("%s %08x%08x\n", name, (unsigned)(x.u >> 32), (unsigned)x.u);
}
static void pf(const char *name, float f)
{
	union { float f; uint32_t u; } x = {f};
	printf("%s %08x\n", name, (unsigned)x.u);
}
static double mk(uint64_t u)
{
	union { double d; uint64_t u; } x;
	x.u = u;
	return x.d;
}
int main(void)
{
	volatile double den1 = mk(1), den2 = mk(2), minn = mk(0x0010000000000000ull);
	volatile double zero = 0.0, one = 1.0, big = 1.5e9, two = 2.0;
	volatile float sden = 1e-40f, sone = 1.0f;
	double inf, nan;
	pd("denorm add", den1 + den2);
	pd("denorm mul", den2 * two);
	pd("denorm half", minn * 0.5);
	pd("denorm sub", minn - den1);
	pd("one plus denorm", one + den1);
	pf("sdenorm add", sden + sden);
	pf("sdenorm one", sone - sden);
	inf = one / zero;
	pd("1/0", inf);
	pd("-1/0", -one / zero);
	pd("inf*2", inf * two);
	pd("inf-inf class", (inf - inf) == (inf - inf) ? 1.0 : 0.0);
	nan = zero / zero;
	printf("nan cmp eq=%d ne=%d lt=%d gt=%d le=%d ge=%d self=%d\n", nan == one, nan != one, nan < one,
	       nan > one, nan <= one, nan >= one, nan == nan);
	printf("inf cmp gt=%d eq=%d lt=%d\n", inf > big, inf == inf, -inf < big);
	printf("int %d %d\n", (int)big, (int)(-big));
	printf("denorm cmp lt=%d eq=%d\n", den1 < den2, den1 == den1);
	printf("printf %.3f %d\n", 2.5 * 1.5, 42);
	printf("FPX REAL DONE\n");
	return 0;
}
