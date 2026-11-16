/* Probe: which FP-to-integer conversions trap on a real MC88100? Build with the SFU1 handler:
 *   build.sh --fpx trnc-probe
 * Raw trnc/int/nint (.ss and .sd) are run through inline asm over a sweep of values, so the
 * compiler's lowering is not involved. A probe stub is placed in front of the ncd19c_fpx entry stub
 * (vector 114): it records FPECR, FPHS2/FPLS2, FPPT, FPSR, FPCR, SXIP and SSBR as the hardware
 * delivers them, counts the trap, then continues into ncd_fpx_stub114, whose handler completes the
 * conversion in software. "r=" is therefore the software result of a trapped instruction (the
 * destination is pre-set to deadbeef; the hardware itself never writes it for a precise trap).
 *
 * Line: <insn> <operand bits hex> <decimal> <T|-> [fe=<FPECR> pt=.. h2=.. l2=.. sx=.. sb=.. sr=.. cr=..] r=<result>
 * No host-expected file exists: whether an instruction traps is the question being measured
 * (MAME never traps, so every MAME line has "-").
 */
#include <stdint.h>
#include "../ncd19c_console.h"
#include "../ncd19c_fpx.h"

/* rec[]: 0 FPECR, 1 FPHS2, 2 FPLS2, 3 FPPT, 4 FPSR, 5 FPCR, 6 SXIP, 7 SSBR, 8 trap count */
volatile uint32_t probe_rec[10];
void probe_stub(void);
__asm__(
	".text\n.p2align 3\n.globl probe_stub\nprobe_stub:\n"
	"	stcr	%r31, %SR3\n"
	"	stcr	%r30, %SR2\n"
	"	or.u	%r31, %r0, %hi16(probe_rec)\n"
	"	or	%r31, %r31, %lo16(probe_rec)\n"
	"	fldcr	%r30, %fcr0\n	st	%r30, %r31, 0\n"
	"	fldcr	%r30, %fcr3\n	st	%r30, %r31, 4\n"
	"	fldcr	%r30, %fcr4\n	st	%r30, %r31, 8\n"
	"	fldcr	%r30, %fcr5\n	st	%r30, %r31, 12\n"
	"	fldcr	%r30, %fcr62\n	st	%r30, %r31, 16\n"
	"	fldcr	%r30, %fcr63\n	st	%r30, %r31, 20\n"
	"	ldcr	%r30, %SXIP\n	st	%r30, %r31, 24\n"
	"	ldcr	%r30, %SSBR\n	st	%r30, %r31, 28\n"
	"	ld	%r30, %r31, 32\n	addu	%r30, %r30, 1\n	st	%r30, %r31, 32\n"
	"	ldcr	%r30, %SR2\n"
	"	ldcr	%r31, %SR3\n"
	"	br	ncd_fpx_stub114\n");

static void out(const char *s) { ncd_console_puts(s); }
static void hex(uint32_t v, int n)
{
	char b[9];
	for (int i = n - 1; i >= 0; --i, v >>= 4)
		b[i] = "0123456789abcdef"[v & 15];
	b[n] = 0;
	out(b);
}
/* Decimal of an IEEE double with at most one fractional bit (.0 or .5), integer arithmetic only. */
static void dec(uint64_t b)
{
	char buf[24];
	int n = 0, e = (int)((b >> 52) & 0x7ff) - 1023, half = 0;
	uint64_t m = (b & 0xfffffffffffffull) | (1ull << 52), ip;
	if (b << 1 == 0) {
		out("0");
		return;
	}
	if (b >> 63)
		out("-");
	if (e >= 52) {
		ip = m << (e - 52);
	} else if (e >= 0) {
		ip = m >> (52 - e);
		half = (m >> (51 - e)) & 1;
	} else {
		ip = 0;
		half = e == -1;
	}
	do {
		buf[n++] = '0' + (char)(ip % 10);
		ip /= 10;
	} while (ip);
	while (n)
		{ char c[2] = {buf[--n], 0}; out(c); }
	if (half)
		out(".5");
}
static uint64_t f2d(uint32_t f)
{
	uint32_t e = (f >> 23) & 0xff;
	if ((f << 1) == 0)
		return (uint64_t)f << 32;
	return ((uint64_t)(f >> 31) << 63) | ((uint64_t)(e - 127 + 1023) << 52) | ((uint64_t)(f & 0x7fffff) << 29);
}
/* double bits of an exact value: sign, integer magnitude, plus optional 0.5 */
static uint64_t mk(int neg, uint64_t mag2) /* mag2 = 2*|x| */
{
	int e = 63 - __builtin_clzll(mag2);
	uint64_t frac = (mag2 << (63 - e)) << 1 >> 12;
	return ((uint64_t)neg << 63) | ((uint64_t)(e - 1 + 1023) << 52) | frac;
}
#define RUND(NAME, MN)                                                                        \
	static uint32_t NAME(uint64_t bits)                                                   \
	{                                                                                     \
		union { uint64_t u; double d; } x = {bits};                                   \
		uint32_t r = 0xdeadbeef;                                                      \
		__asm__ volatile(MN " %0, %1" : "+r"(r) : "r"(x.d) : "memory");              \
		return r;                                                                     \
	}
#define RUNS(NAME, MN)                                                                        \
	static uint32_t NAME(uint32_t bits)                                                   \
	{                                                                                     \
		union { uint32_t u; float f; } x = {bits};                                    \
		uint32_t r = 0xdeadbeef;                                                      \
		__asm__ volatile(MN " %0, %1" : "+r"(r) : "r"(x.f) : "memory");              \
		return r;                                                                     \
	}
RUND(trnc_sd, "trnc.sd")
RUND(int_sd, "int.sd")
RUND(nint_sd, "nint.sd")
RUNS(trnc_ss, "trnc.ss")
RUNS(int_ss, "int.ss")
RUNS(nint_ss, "nint.ss")

static void report(const char *insn, uint64_t shown, uint32_t cnt0, uint32_t res)
{
	int trapped = probe_rec[8] != cnt0;
	out(insn);
	out(" ");
	hex((uint32_t)(shown >> 32), 8);
	hex((uint32_t)shown, 8);
	out(" ");
	dec(shown);
	out(trapped ? " T" : " -");
	if (trapped) {
		out(" fe=");
		hex(probe_rec[0], 2);
		out(" pt=");
		hex(probe_rec[3], 4);
		out(" h2=");
		hex(probe_rec[1], 8);
		out(" l2=");
		hex(probe_rec[2], 8);
		out(" sx=");
		hex(probe_rec[6], 8);
		out(" sb=");
		hex(probe_rec[7], 8);
		out(" sr=");
		hex(probe_rec[4], 2);
		out(" cr=");
		hex(probe_rec[5], 8);
	}
	out(" r=");
	hex(res, 8);
	out("\n");
}

static uint64_t vals[200];
static uint32_t fseen[200];
static int nv;
static void add(uint64_t b) { vals[nv++] = b; }
static void add_mag(uint64_t mag2)
{
	add(mk(0, mag2));
	add(mk(1, mag2));
}
int main(void)
{
	uint32_t psr, *v;
	ncd_fpx_install();
	/* chain the probe stub in front of the fpx stub */
	psr = ncd_fpx_rd_psr();
	ncd_fpx_wr_psr(psr | 2);
	v = (uint32_t *)(ncd_fpx_rd_vbr() + NCD_FPX_VEC_PRECISE * 8);
	v[0] = 0xc0000000u | ((((uint32_t)probe_stub - (uint32_t)&v[0]) >> 2) & 0x03ffffffu);
	ncd_fpx_wr_psr(psr);

	for (int k = 20; k <= 31; ++k) {
		uint64_t p = 1ull << k;
		add_mag(2 * (p - 1));
		add_mag(2 * p);
		add_mag(2 * p + 1);
	}
	add_mag(2 * (1073741824ull - 1) + 1); /* 2^30 - 0.5 */
	add_mag(2 * 2147483649ull);	      /* 2^31 + 1 (the -2^31-1 case) */
	add(0);
	add_mag(1);		      /* 0.5 */
	add_mag(2 * 4294967296ull);   /* 2^32 */
	add_mag(2 * 10000000000ull);  /* 1e10 */
	add_mag(2);		      /* 1 */
	add_mag(3);		      /* 1.5 */

	out("TRNC PROBE\n");
	for (int i = 0; i < nv; ++i) {
		uint64_t b = vals[i];
		uint32_t c = probe_rec[8], r;
		r = trnc_sd(b);
		report("trnc.sd", b, c, r);
		c = probe_rec[8];
		r = int_sd(b);
		report("int.sd ", b, c, r);
		c = probe_rec[8];
		r = nint_sd(b);
		report("nint.sd", b, c, r);
	}
	/* single precision: values exactly representable as float, each once */
	int nf = 0;
	for (int i = 0; i < nv; ++i) {
		uint64_t b = vals[i];
		uint32_t e = (b >> 52) & 0x7ff, f;
		int ok = 1;
		if (b << 1 == 0) {
			f = 0;
		} else {
			if ((b & 0x1fffffffull) != 0)
				ok = 0;
			f = (uint32_t)(b >> 63) << 31 | ((e - 1023 + 127) << 23) | (uint32_t)((b >> 29) & 0x7fffff);
		}
		for (int j = 0; j < nf; ++j)
			if (fseen[j] == f)
				ok = 0;
		if (!ok)
			continue;
		fseen[nf++] = f;
		uint32_t c = probe_rec[8], r;
		r = trnc_ss(f);
		report("trnc.ss", f2d(f), c, r);
		c = probe_rec[8];
		r = int_ss(f);
		report("int.ss ", f2d(f), c, r);
		c = probe_rec[8];
		r = nint_ss(f);
		report("nint.ss", f2d(f), c, r);
	}
	out("DONE\n");
	ncd_fpx_uninstall();
	return 0;
}
