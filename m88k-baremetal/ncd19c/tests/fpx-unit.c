/* Unit test for ncd_fpx_handle (ncd19c_fpx.c): synthetic precise/imprecise exception frames built
 * from the MC88100 manual's encodings (FPPT 6.8.1.8, FPIT 6.8.1.9, FPRH/FPRL 6.8.1.10-11, operand
 * registers 6.8.1.4-7), handler called directly, destination registers compared with values the host
 * computes from the same inputs. Build for the host with -DHOST to get tests/fpx-unit.expected:
 *   cc -DHOST -O0 -o /tmp/fpx-unit-host tests/fpx-unit.c && /tmp/fpx-unit-host
 * Output per case: name, dest register words, FPSR, FPECR, number of other registers changed.
 * Flags (FPSR) and the fcmp/NaN results follow the manual and are written out as constants.
 * MAME never raises these exceptions; this checks the emulation logic only.
 */
#include <stdint.h>
#ifdef HOST
#include <stdio.h>
static void out(const char *s) { fputs(s, stdout); }
#else
#include "../ncd19c_console.h"
#include "../ncd19c_fpx.h"
static void out(const char *s) { ncd_console_puts(s); }
#endif
static void hex(uint32_t v)
{
	char b[9];
	for (int i = 7; i >= 0; --i, v >>= 4)
		b[i] = "0123456789abcdef"[v & 15];
	b[8] = 0;
	out(b);
}
static void emit(const char *name, uint32_t x, uint32_t y, uint32_t fpsr, uint32_t fpecr, unsigned bad)
{
	out(name);
	out(" ");
	hex(x);
	out(" ");
	hex(y);
	out(" ");
	hex(fpsr);
	out(" ");
	hex(fpecr);
	out(" ");
	hex(bad);
	out("\n");
}
#define SENT(n) (0xa5a50000u + (n))
static uint64_t dbits(double d) { union { double d; uint64_t u; } x = {d}; return x.u; }
static double bitsd(uint64_t u) { union { double d; uint64_t u; } x; x.u = u; return x.d; }
static uint32_t sbits(float f) { union { float f; uint32_t u; } x = {f}; return x.u; }
static float bitss(uint32_t u) { union { float f; uint32_t u; } x; x.u = u; return x.f; }

/* FPPT (6.8.1.8): bits 15-5 of the instruction (opcode, T1, T2, TD), destination in 4-0. */
#define PT(op, t1, t2, td, d) (((op) << 11) | ((t1) << 9) | ((t2) << 7) | ((td) << 5) | (d))
/* FPIT (6.8.1.9): RESEXP (unbiased exponent - 1) in 31-20, opcode 15-11, DESTSIZ 10, destination 4-0. */
#define IT(resexp, op, dbl, d) ((((uint32_t)(resexp)&0xfffu) << 20) | ((op) << 11) | ((dbl) << 10) | (d))
/* FPRH (6.8.1.10): sign 31, RNDMODE 30-29, guard 28, round 27, sticky 26, addone 25, mantissa hi 20-0. */
#define RH(sign, rm, g, r, s, ao, hi) \
	(((uint32_t)(sign) << 31) | ((rm) << 29) | ((g) << 28) | ((r) << 27) | ((s) << 26) | ((ao) << 25) | (hi))

#define FPSR_INV 0x10
#define FPSR_DVZ 0x08
#define FPSR_UNF 0x04
#define FPSR_OVF 0x02
#define FPSR_INX 0x01

/* host reference values */
static uint64_t ref_fadd_den(void) { volatile double a = bitsd(3), b = bitsd(0x000fffffffffffffull); return dbits(a + b); }
static uint64_t ref_fsub_sden(void) { volatile float a = 1.0f, b = bitss(1); return sbits(a - b); }
static uint64_t ref_fdiv_dvz(void) { volatile double a = 1.0, b = 0.0; return dbits(a / b); }
static uint64_t ref_fdiv_dvzn(void) { volatile double a = -3.0, b = 0.0; return dbits(a / b); }
static uint64_t ref_fmul_inf(void) { volatile double a = bitsd(0x7ff0000000000000ull), b = 2.0; return dbits(a * b); }
static uint64_t ref_fmul_sinf(void) { volatile float a = bitss(0xff800000u), b = 0.5f; return sbits(a * b); }
static uint64_t ref_fadd_sd(void) { volatile float a = bitss(0x00000007u); volatile double b = bitsd(0x0000000000000010ull); return dbits((double)a + b); }
static uint64_t ref_flt(void) { volatile int i = -5; return dbits((double)i); }
static uint64_t ref_fcmp_nan(void) { return 0x9; } /* not comparable, not equal (manual: fcmp) */
/* fcmp, written from the manual: nc/cp, eq/ne, gt/le/lt/ge and the range bits ou/ib/in/ob. */
static uint64_t ref_fcmp(double a, double b)
{
	uint32_t r = 0x2 | 0x4 | 0x20 | 0x80; /* cp eq le ge */
	if (a != b) {
		r = 0x2 | 0x8; /* cp ne */
		r |= a < b ? 0x40 | 0x20 : 0x80 | 0x10;
	}
	if (!(dbits(b) >> 63)) {
		if (a < 0 || a > b)
			r |= 0x100 | 0x800;
		else if (a == 0 || a == b)
			r |= 0x800 | 0x200;
		else
			r |= 0x400 | 0x200;
	}
	return r;
}
static uint64_t ref_fcmp_in(void) { volatile double a = 1.0, b = 2.0; return ref_fcmp(a, b); }
static uint64_t ref_fcmp_ou(void) { volatile double a = -1.0, b = 2.0; return ref_fcmp(a, b); }
static uint64_t ref_fcmp_gt(void) { volatile double a = 5.0, b = 2.0; return ref_fcmp(a, b); }
static uint64_t ref_fcmp_negb(void) { volatile double a = 1.0, b = -2.0; return ref_fcmp(a, b); }
static uint64_t sat(double d, int trunc)
{
	/* int/nint/trnc: out-of-range gives the largest integer of the sign (manual 6.8.4.2, OpenBSD) */
	if (d >= 2147483648.0)
		return 0x7fffffffu;
	if (d < -2147483648.0)
		return 0x80000000u;
	if (trunc)
		return (uint32_t)(int32_t)d;
	return (uint32_t)(int32_t)(d < 0 ? d - 0.5 : d + 0.5);
}
static uint64_t ref_trnc_ovf(void) { volatile double d = 1e10; return sat(d, 1); }
static uint64_t ref_trnc_ovfn(void) { volatile double d = -1e10; return sat(d, 1); }
static uint64_t ref_trnc_ok(void) { volatile double d = 1.5e9; return sat(d, 1); }
static uint64_t ref_nint_ok(void) { volatile double d = 1073741823.75; return sat(d, 0); }
static uint64_t ref_int_sovf(void) { volatile float f = 3.0e9f; return sat(f, 0); }
static uint64_t ref_int_ok(void) { volatile double d = -2147483648.0; return sat(d, 1); }
static uint64_t ref_fmul_unf(void) { volatile double x = 0x1p-1000, y = 0x1p-30; return dbits(x * y); }
static uint64_t ref_fmul_unf_inx(void) { volatile double x = 0x1.0000000000001p-1000, y = 0x1p-30; return dbits(x * y); }
static uint64_t ref_fmul_sunf(void) { volatile float x = 0x1.8p-100f, y = 0x1p-27f; return sbits(x * y); }
static uint64_t ref_fmul_ovf(void) { volatile double x = 1e200, y = 1e200; return dbits(x * y); }
static uint64_t ref_fmul_ovf_rz(void) { return 0x7fefffffffffffffull; } /* RM = toward zero */
static uint64_t ref_fmul_ovfn_ri(void) { return 0xffefffffffffffffull; } /* -overflow, RM = +inf */
static uint64_t ref_inexact(void) { volatile double x = 3.0, y = 7.0; return dbits(x / y); }

/* Single operand in the FPHSn/FPLSn format: sign-extended exponent (6.8.1.4). */
static void sh(uint32_t b, uint32_t *h, uint32_t *l)
{
	uint32_t e = (b >> 23) & 0xff;
	if (e & 0x80)
		e |= 0x700;
	*h = (b & 0x80000000u) | (e << 20) | ((b >> 3) & 0xfffffu);
	*l = (b & 7) << 29;
}

struct tc {
	const char *name;
	unsigned vec;
	uint32_t pt;		/* FPPT or FPIT */
	uint64_t s1, s2;	/* operands, as IEEE bits; s2 of flt is the integer */
	unsigned s1s, s2s;	/* operand is single */
	uint32_t fprh, fprl, fpecr;
	unsigned d, nwords;
	uint64_t (*ref)(void);
	uint32_t fpsr;
};
#define D(x) dbits(x)
#define P(name, pt, s1, s1s, s2, s2s, ecr, d, nw, ref, fpsr) \
	{name, 114, pt, s1, s2, s1s, s2s, 0, 0, ecr, d, nw, ref, fpsr}
#define I(name, it, rh, rl, ecr, d, nw, ref, fpsr) {name, 115, it, 0, 0, 0, 0, rh, rl, ecr, d, nw, ref, fpsr}
static const struct tc cases[] = {
    P("fadd.ddd.denorm", PT(0x05, 1, 1, 1, 10), 3, 0, 0x000fffffffffffffull, 0, 0x10, 10, 2, ref_fadd_den, 0),
    P("fsub.sss.denorm", PT(0x06, 0, 0, 0, 7), 0x3f800000u, 1, 1, 1, 0x10, 7, 1, ref_fsub_sden, 0),
    P("fadd.sdd.denorm", PT(0x05, 0, 1, 1, 20), 7, 1, 0x10, 0, 0x10, 20, 2, ref_fadd_sd, 0),
    P("fdiv.ddd.dvz", PT(0x0e, 1, 1, 1, 12), 0x3ff0000000000000ull, 0, 0, 0, 0x08, 12, 2, ref_fdiv_dvz, FPSR_DVZ),
    P("fdiv.ddd.dvzneg", PT(0x0e, 1, 1, 1, 12), 0xc008000000000000ull, 0, 0, 0, 0x08, 12, 2, ref_fdiv_dvzn, FPSR_DVZ),
    P("fmul.ddd.inf", PT(0x00, 1, 1, 1, 14), 0x7ff0000000000000ull, 0, 0x4000000000000000ull, 0, 0x10, 14, 2, ref_fmul_inf, 0),
    P("fmul.sss.inf", PT(0x00, 0, 0, 0, 3), 0xff800000u, 1, 0x3f000000u, 1, 0x10, 3, 1, ref_fmul_sinf, 0),
    P("fcmp.sdd.nan", PT(0x07, 0, 1, 0, 5), 0x7fc00000u, 1, 0x3ff0000000000000ull, 0, 0x10, 5, 1, ref_fcmp_nan, 0),
    P("fcmp.sdd.snan", PT(0x07, 0, 1, 0, 5), 0x7f800001u, 1, 0x3ff0000000000000ull, 0, 0x10, 5, 1, ref_fcmp_nan, FPSR_INV),
    P("fcmp.ddd.in", PT(0x07, 1, 1, 0, 6), 0x3ff0000000000000ull, 0, 0x4000000000000000ull, 0, 0x10, 6, 1, ref_fcmp_in, 0),
    P("fcmp.ddd.ou", PT(0x07, 1, 1, 0, 6), 0xbff0000000000000ull, 0, 0x4000000000000000ull, 0, 0x10, 6, 1, ref_fcmp_ou, 0),
    P("fcmp.ddd.gt", PT(0x07, 1, 1, 0, 6), 0x4014000000000000ull, 0, 0x4000000000000000ull, 0, 0x10, 6, 1, ref_fcmp_gt, 0),
    P("fcmp.ddd.negb", PT(0x07, 1, 1, 0, 6), 0x3ff0000000000000ull, 0, 0xc000000000000000ull, 0, 0x10, 6, 1, ref_fcmp_negb, 0),
    P("trnc.sd.ovf", PT(0x0b, 0, 1, 0, 8), 0, 0, 0x4202a05f20000000ull, 0, 0x80, 8, 1, ref_trnc_ovf, FPSR_INV),
    P("trnc.sd.ovfneg", PT(0x0b, 0, 1, 0, 8), 0, 0, 0xc202a05f20000000ull, 0, 0x80, 8, 1, ref_trnc_ovfn, FPSR_INV),
    P("trnc.sd.ok", PT(0x0b, 0, 1, 0, 8), 0, 0, 0x41d65a0bc0000000ull, 0, 0x80, 8, 1, ref_trnc_ok, 0),
    P("nint.sd.ok", PT(0x0a, 0, 1, 0, 9), 0, 0, 0x41cfffffffe00000ull, 0, 0x80, 9, 1, ref_nint_ok, FPSR_INX),
    P("int.ss.ovf", PT(0x09, 0, 0, 0, 11), 0, 0, 0x4f32d05eu, 1, 0x80, 11, 1, ref_int_sovf, FPSR_INV),
    P("int.sd.min", PT(0x09, 0, 1, 0, 11), 0, 0, 0xc1e0000000000000ull, 0, 0x80, 11, 1, ref_int_ok, 0),
    P("flt.d", PT(0x04, 0, 0, 1, 4), 0, 0, (uint64_t)(uint32_t)-5, 0, 0x10, 4, 2, ref_flt, 0),
    /* FPIT/FPRH/FPRL are 6.8.1.9-11. Result 2^-1030 = 1.0 x 2^-1030: RESEXP = -1031, mantissa 1.0. */
    I("fmul.d.unf", IT(-1031, 0x00, 1, 12), RH(0, 0, 0, 0, 0, 0, 0x100000), 0, 0x04, 12, 2, ref_fmul_unf, 0),
    I("fmul.d.unf.inx", IT(-1031, 0x00, 1, 12), RH(0, 0, 0, 0, 0, 0, 0x100000), 1, 0x04, 12, 2, ref_fmul_unf_inx, FPSR_UNF | FPSR_INX),
    I("fmul.s.unf", IT(-128, 0x00, 0, 13), RH(0, 0, 0, 0, 0, 0, 0x180000), 0, 0x04, 13, 1, ref_fmul_sunf, 0),
    I("fmul.d.ovf", IT(1024, 0x00, 1, 16), RH(0, 0, 0, 0, 0, 0, 0x100000), 0, 0x02, 16, 2, ref_fmul_ovf, FPSR_OVF | FPSR_INX),
    I("fmul.d.ovf.rz", IT(1024, 0x00, 1, 16), RH(0, 1, 0, 0, 0, 0, 0x100000), 0, 0x02, 16, 2, ref_fmul_ovf_rz, FPSR_OVF | FPSR_INX),
    I("fmul.d.ovfneg.rp", IT(1024, 0x00, 1, 16), RH(1, 3, 0, 0, 0, 0, 0x100000), 0, 0x02, 16, 2, ref_fmul_ovfn_ri, FPSR_OVF | FPSR_INX),
    /* inexact only: 3/7 = 1.714285..x2^-2, RESEXP = -3; mantissa taken from the rounded result */
    I("fdiv.d.inx", IT(-3, 0x0e, 1, 18), RH(0, 0, 0, 0, 0, 0, 0x1b6db6), 0xdb6db6dbu, 0x01, 18, 2, ref_inexact, FPSR_INX),
};
#define N (sizeof(cases) / sizeof(cases[0]))

#ifdef HOST
int main(void)
{
	out("fpx-unit start\n");
	for (unsigned i = 0; i < N; ++i) {
		const struct tc *c = &cases[i];
		uint64_t v = c->ref();
		if (c->nwords == 2)
			emit(c->name, (uint32_t)(v >> 32), (uint32_t)v, c->fpsr, 0, 0);
		else
			emit(c->name, (uint32_t)v, SENT(c->d + 1), c->fpsr, 0, 0);
	}
	out("FPX UNIT DONE\n");
	return 0;
}
#else
static struct ncd_fpx_frame fr;
int main(void)
{
	out("fpx-unit start\n");
	for (unsigned i = 0; i < N; ++i) {
		const struct tc *c = &cases[i];
		for (unsigned n = 0; n < 32; ++n)
			fr.r[n] = SENT(n);
		fr.fpsr = fr.fpcr = 0;
		fr.fpecr = c->fpecr;
		fr.vector = c->vec;
		if (c->vec == 114) {
			fr.fppt = c->pt;
			if (c->s1s)
				sh((uint32_t)c->s1, &fr.fphs1, &fr.fpls1);
			else {
				fr.fphs1 = (uint32_t)(c->s1 >> 32);
				fr.fpls1 = (uint32_t)c->s1;
			}
			if (((c->pt >> 11) & 31) == 4) { /* flt: integer operand in FPLS2 */
				fr.fpls2 = (uint32_t)c->s2;
			} else if (c->s2s) {
				sh((uint32_t)c->s2, &fr.fphs2, &fr.fpls2);
			} else {
				fr.fphs2 = (uint32_t)(c->s2 >> 32);
				fr.fpls2 = (uint32_t)c->s2;
			}
		} else {
			fr.fpit = c->pt;
			fr.fprh = c->fprh;
			fr.fprl = c->fprl;
		}
		ncd_fpx_handle(&fr, c->vec);
		unsigned bad = 0;
		for (unsigned n = 1; n < 32; ++n)
			if (fr.r[n] != SENT(n) && n != c->d && !(c->nwords == 2 && n == ((c->d + 1) & 31)))
				++bad;
		emit(c->name, fr.r[c->d], fr.r[(c->d + 1) & 31], fr.fpsr, fr.fpecr, bad);
	}
	out("FPX UNIT DONE\n");
	return 0;
}
#endif
