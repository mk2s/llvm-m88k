/* SFU1 exception handler, C part. See ncd19c_fpx.S for the entry/exit sequence.
 *
 * The MC88100 FPU traps instead of handling reserved operands (denormals, NaN, infinity),
 * division by zero, integer-conversion overflow, and result underflow/overflow/inexact. This code
 * finishes the faulted instruction in software and puts the result in the saved register frame.
 * Manual section numbers refer to the MC88100 User's Manual, 2nd ed. (1990).
 *
 * All arithmetic goes through the compiler-rt soft-float routines called by name, so this file never
 * executes an FP instruction. It runs with the FPU enabled (the stub restarts it), because
 * compiler-rt and ordinary C use mul/div, which are FPU instructions on the MC88100.
 *
 * Limits: user exception handlers (FPCR.EFINV/EFDVZ/EFUNF/EFOVF/EFINX, 6.8.4-6.8.5) are not
 * dispatched; every exception gets the default IEEE 754 action. Emulated arithmetic rounds to
 * nearest even (compiler-rt); FPCR.RM is honoured only for int and the imprecise underflow/overflow
 * results. The inexact accrued flag is not tracked for emulated precise operations.
 */
#include "ncd19c_console.h"
#include "ncd19c_fpx.h"

extern double __adddf3(double, double);
extern double __subdf3(double, double);
extern double __muldf3(double, double);
extern double __divdf3(double, double);
extern double __extendsfdf2(float);
extern float __truncdfsf2(double);
extern double __floatsidf(int);
extern int __eqdf2(double, double);
extern int __ltdf2(double, double);
void _exit(int);
void ncd_fpx_reset_psr(void);

volatile unsigned ncd_fpx_count[2];

union d64 { double d; uint64_t u; };
union f32 { float f; uint32_t u; };

/* noinline: keeps the optimizer from seeing a double at the call sites, where it would turn integer
   NaN tests on the bits into an FP compare (fcmp ord) that traps again on a reserved operand. */
#define OPAQUE __attribute__((noinline))
static OPAQUE double tod(uint64_t u) { union d64 x; x.u = u; return x.d; }
static OPAQUE uint64_t fromd(double d) { union d64 x; x.d = d; return x.u; }

#define DEXP(b) (((b) >> 52) & 0x7ff)
#define DFRAC(b) ((b) & 0xfffffffffffffull)
static int is_nan(uint64_t b) { return DEXP(b) == 0x7ff && DFRAC(b) != 0; }
static int is_inf(uint64_t b) { return DEXP(b) == 0x7ff && DFRAC(b) == 0; }
static int is_zero(uint64_t b) { return (b << 1) == 0; }

static OPAQUE uint64_t f32_to_f64(uint32_t b)
{
	union f32 x;
	x.u = b;
	return fromd(__extendsfdf2(x.f));
}

static OPAQUE uint32_t f64_to_f32(uint64_t b)
{
	union f32 x;
	x.f = __truncdfsf2(tod(b));
	return x.u;
}

/* FPHSn/FPLSn -> IEEE double bits (6.8.1.4-6.8.1.7). A double is split over the two registers.
   A single has the sign in bit 31, its 8-bit exponent sign-extended in bits 30-20, the high 20
   mantissa bits in 19-0, and the low 3 mantissa bits in the top of the low register. */
static uint64_t unpack(uint32_t h, uint32_t l, unsigned dbl)
{
	if (dbl)
		return ((uint64_t)h << 32) | l;
	return f32_to_f64((h & 0x80000000u) | (((h >> 20) & 0xffu) << 23) | ((h & 0xfffffu) << 3) |
			  (l >> 29));
}

/* Signaling NaN test on the raw operand registers (the conversion of a single to a double quiets it). */
static int raw_snan(uint32_t h, uint32_t l, unsigned dbl)
{
	unsigned e = dbl ? (h >> 20) & 0x7ff : (h >> 20) & 0xff;
	uint32_t frac = (h & 0xfffffu) | (dbl ? l : l >> 29);
	return e == (dbl ? 0x7ffu : 0xffu) && frac != 0 && !((h >> 19) & 1);
}

static void put_result(struct ncd_fpx_frame *f, unsigned d, unsigned dbl, uint64_t bits)
{
	if (d == 0)
		return;
	if (dbl) {
		f->r[d] = (uint32_t)(bits >> 32);
		f->r[(d + 1) & 31] = (uint32_t)bits;
	} else {
		f->r[d] = f64_to_f32(bits);
	}
	f->r[0] = 0;
}

/* fcmp result string (manual: fcmp). Bit layout as in mc88100_device::fcmp (MAME m88000.cpp). */
#define FCMP_NC 0x001u
#define FCMP_CP 0x002u
#define FCMP_EQ 0x004u
#define FCMP_NE 0x008u
#define FCMP_GT 0x010u
#define FCMP_LE 0x020u
#define FCMP_LT 0x040u
#define FCMP_GE 0x080u
#define FCMP_OU 0x100u
#define FCMP_IB 0x200u
#define FCMP_IN 0x400u
#define FCMP_OB 0x800u

static uint32_t fcmp_bits(uint64_t a, uint64_t b)
{
	uint32_t res;
	if (is_nan(a) || is_nan(b))
		return FCMP_NC | FCMP_NE; /* unordered */
	res = FCMP_GE | FCMP_LE | FCMP_EQ | FCMP_CP;
	if (__eqdf2(tod(a), tod(b)) != 0) {
		res = FCMP_NE | FCMP_CP;
		if (__ltdf2(tod(a), tod(b)) < 0)
			res |= FCMP_LT | FCMP_LE;
		else
			res |= FCMP_GE | FCMP_GT;
	}
	if (!(b >> 63)) { /* range checks are all zero when rS2 is negative */
		if ((a << 1) != 0 && (res & FCMP_NE)) {
			if ((a >> 63) || (res & FCMP_GT))
				res |= FCMP_OU | FCMP_OB;
			else if (res & FCMP_LT)
				res |= FCMP_IN | FCMP_IB;
		} else {
			res |= FCMP_OB | FCMP_IB;
		}
	}
	return res;
}

/* Double bits -> int32 with rounding mode rm (FPCR.RM encoding: 0 nearest, 1 zero, 2 -inf, 3 +inf).
   Out-of-range values, NaN and infinity set *inv and give the largest-magnitude integer of the
   right sign (as OpenBSD does; a NaN is treated as positive). */
static uint32_t cvt_int(uint64_t a, unsigned rm, unsigned *flags)
{
	unsigned sign = a >> 63, e = DEXP(a), inc = 0;
	uint64_t m = DFRAC(a), mag, rem = 0, half = 0;
	int shift;
	if (e == 0x7ff)
		goto invalid;
	if (e)
		m |= 1ull << 52;
	else
		e = 1;
	shift = 1075 - (int)e;
	if (shift <= 0)
		goto invalid;
	if (shift >= 55) { /* below 2^-2: only rounding up can give a non-zero result */
		mag = 0;
		rem = m != 0;
		half = 2; /* never reached: rem < half */
	} else {
		mag = m >> shift;
		rem = m & ((1ull << shift) - 1);
		half = 1ull << (shift - 1);
	}
	switch (rm) {
	case 0:
		inc = rem > half || (rem == half && (mag & 1));
		break;
	case 1:
		inc = 0;
		break;
	case 2:
		inc = sign && rem;
		break;
	default:
		inc = !sign && rem;
		break;
	}
	mag += inc;
	if (mag > (sign ? 0x80000000ull : 0x7fffffffull))
		goto invalid;
	if (rem)
		*flags |= FPSR_AFINX;
	return sign ? (uint32_t)(0 - (uint32_t)mag) : (uint32_t)mag;
invalid:
	*flags |= FPSR_AFINV;
	return sign ? 0x80000000u : 0x7fffffffu;
}

static void fatal(const char *why, const struct ncd_fpx_frame *f);

static void precise(struct ncd_fpx_frame *f)
{
	uint32_t pt = f->fppt;
	unsigned op = (pt >> 11) & 31, t1 = (pt >> 9) & 1, t2 = (pt >> 7) & 1, td = (pt >> 5) & 1;
	unsigned d = pt & 31, flags = 0;
	uint64_t a = unpack(f->fphs1, f->fpls1, t1), b = unpack(f->fphs2, f->fpls2, t2), r;
	if (f->fpecr & (FPECR_FUNIMP | FPECR_FPRV))
		fatal("unimplemented FP opcode or privilege violation", f);
	if (op != 0x04 && raw_snan(f->fphs2, f->fpls2, t2))
		flags |= FPSR_AFINV;
	if ((op == 0x00 || op == 0x05 || op == 0x06 || op == 0x07 || op == 0x0e) &&
	    raw_snan(f->fphs1, f->fpls1, t1))
		flags |= FPSR_AFINV;
	switch (op) {
	case 0x00: /* fmul */
	case 0x05: /* fadd */
	case 0x06: /* fsub */
	case 0x0e: /* fdiv */
		if (op == 0x00)
			r = fromd(__muldf3(tod(a), tod(b)));
		else if (op == 0x05)
			r = fromd(__adddf3(tod(a), tod(b)));
		else if (op == 0x06)
			r = fromd(__subdf3(tod(a), tod(b)));
		else
			r = fromd(__divdf3(tod(a), tod(b)));
		if (is_nan(r) && !is_nan(a) && !is_nan(b))
			flags |= FPSR_AFINV;
		else if (op == 0x0e && is_zero(b) && !is_zero(a) && !is_inf(a) && !is_nan(a))
			flags |= FPSR_AFDVZ;
		put_result(f, d, td, r);
		break;
	case 0x04: /* flt: integer in FPLS2 */
		put_result(f, d, td, fromd(__floatsidf((int)f->fpls2)));
		break;
	case 0x07: /* fcmp */
		if (d)
			f->r[d] = fcmp_bits(a, b);
		break;
	case 0x09: /* int: current rounding mode */
	case 0x0a: /* nint: nearest */
	case 0x0b: /* trnc: toward zero */
		r = cvt_int(b, op == 0x09 ? (f->fpcr >> 14) & 3 : op == 0x0a ? 0 : 1, &flags);
		if (d)
			f->r[d] = (uint32_t)r;
		break;
	default:
		fatal("unimplemented FP opcode", f);
	}
	f->fpsr |= flags;
}

/* Imprecise exceptions: FPRH/FPRL hold the partial result (21+32 mantissa bits including the hidden
   bit, guard/round/sticky/addone, rounding mode), FPIT the destination and RESEXP, the unbiased
   exponent minus one (6.8.1.9-6.8.1.11, 6.8.5). */
static void imprecise(struct ncd_fpx_frame *f)
{
	uint32_t it = f->fpit, h = f->fprh;
	unsigned dbl = (it >> 10) & 1, d = it & 31, sign = h >> 31, rm = (h >> 29) & 3;
	unsigned g = (h >> 28) & 1, rd = (h >> 27) & 1, s = (h >> 26) & 1, addone = (h >> 25) & 1;
	unsigned flags = 0;
	int unb = ((int32_t)it >> 20) + 1, p, emin, bias;
	uint64_t m = h & 0x1fffffu, hidden, bits;
	if (dbl) {
		m = (m << 32) | f->fprl;
		p = 53;
		emin = -1022;
		bias = 1023;
	} else {
		m = (m << 3) | (f->fprl >> 29);
		p = 24;
		emin = -126;
		bias = 127;
	}
	hidden = 1ull << (p - 1);
	if ((f->fpecr & FPECR_FUNF) && emin - unb > 0) {
		/* Default underflow action: undo the rounding, denormalize, round once (6.8.5.1). */
		int n;
		uint64_t v;
		unsigned inexact, inc;
		if (addone) {
			m -= 1;
			if (m < hidden) { /* the carry had renormalized the mantissa */
				m = (hidden << 1) - 1;
				unb--;
			}
		}
		n = emin - unb;
		v = (m << 3) | ((uint64_t)g << 2) | ((uint64_t)rd << 1) | s;
		if (n >= 60) {
			v = v != 0;
		} else if (n > 0) {
			uint64_t lost = v & ((1ull << n) - 1);
			v = (v >> n) | (lost != 0);
		}
		inexact = (v & 7) != 0;
		switch (rm) {
		case 0:
			inc = ((v >> 2) & 1) && ((v & 3) || ((v >> 3) & 1));
			break;
		case 1:
			inc = 0;
			break;
		case 2:
			inc = sign && inexact;
			break;
		default:
			inc = !sign && inexact;
			break;
		}
		m = (v >> 3) + inc; /* the denormal fraction; 2^(p-1) is the smallest normal */
		bits = m;
		if (inexact)
			flags |= FPSR_AFUNF | FPSR_AFINX;
	} else if (f->fpecr & FPECR_FOVF) {
		/* Default overflow action: infinity or the largest finite number (6.8.5.2). */
		int to_inf = rm == 0 ? 1 : rm == 1 ? 0 : rm == 2 ? (int)sign : !sign;
		if (dbl)
			bits = to_inf ? 0x7ff0000000000000ull : 0x7fefffffffffffffull;
		else
			bits = to_inf ? 0x7f800000u : 0x7f7fffffu;
		flags |= FPSR_AFOVF | FPSR_AFINX;
	} else {
		/* Inexact (6.8.5.3): rebuild the rounded result from the exponent and mantissa. */
		uint64_t biased = (uint64_t)(unb + bias);
		bits = dbl ? (biased << 52) | (m & (hidden - 1)) : (biased << 23) | (m & (hidden - 1));
		flags |= FPSR_AFINX;
	}
	if (dbl) {
		bits |= (uint64_t)sign << 63;
		if (d) {
			f->r[d] = (uint32_t)(bits >> 32);
			f->r[(d + 1) & 31] = (uint32_t)bits;
		}
	} else {
		bits |= (uint64_t)sign << 31;
		if (d)
			f->r[d] = (uint32_t)bits;
	}
	f->r[0] = 0;
	f->fpsr |= flags;
}

void ncd_fpx_handle(struct ncd_fpx_frame *f, unsigned vector)
{
	if (vector != NCD_FPX_VEC_PRECISE && vector != NCD_FPX_VEC_IMPRECISE)
		fatal("bad vector", f);
	ncd_fpx_count[vector - NCD_FPX_VEC_PRECISE]++;
	if (vector == NCD_FPX_VEC_PRECISE) {
		if (f->fpecr & 0xf8)
			precise(f);
		else if (f->epsr & 8)
			fatal("FP instruction with the FPU disabled", f);
		/* else: software tb0 to the vector, nothing to do */
	} else if (f->fpecr & 7) {
		imprecise(f);
	}
	f->fpecr = 0;
}

static void put_hex(unsigned v)
{
	int i;
	for (i = 28; i >= 0; i -= 4)
		ncd_console_putc("0123456789abcdef"[(v >> i) & 15]);
}

static void fatal(const char *why, const struct ncd_fpx_frame *f)
{
	ncd_console_puts("\nFPX FATAL: ");
	ncd_console_puts(why);
	if (f) {
		ncd_console_puts(" FPECR=");
		put_hex(f->fpecr);
		ncd_console_puts(" FPPT=");
		put_hex(f->fppt);
		ncd_console_puts(" FPIT=");
		put_hex(f->fpit);
		ncd_console_puts(" SXIP=");
		put_hex(f->sxip);
	}
	ncd_console_putc('\n');
	ncd_fpx_reset_psr();
	_exit(1);
}

void ncd_fpx_fatal_nested(void)
{
	fatal("exception during FPU restart", 0);
}

/* Vector table patching. Each entry is 8 bytes: br <stub> and a nop. */
static uint32_t saved[4];
static int installed;

static uint32_t br_to(uint32_t from, uint32_t to)
{
	return 0xc0000000u | (((to - from) >> 2) & 0x03ffffffu);
}

void ncd_fpx_install(void)
{
	volatile uint32_t *v;
	uint32_t psr;
	if (installed)
		return;
	psr = ncd_fpx_rd_psr();
	ncd_fpx_wr_psr(psr | 2); /* PSR.IND: interrupts off while the table is written */
	v = (volatile uint32_t *)(ncd_fpx_rd_vbr() + NCD_FPX_VEC_PRECISE * 8);
	saved[0] = v[0];
	saved[1] = v[1];
	saved[2] = v[2];
	saved[3] = v[3];
	v[0] = br_to((uint32_t)&v[0], (uint32_t)ncd_fpx_stub114);
	v[1] = 0xf4005800u; /* or %r0,%r0,%r0 */
	v[2] = br_to((uint32_t)&v[2], (uint32_t)ncd_fpx_stub115);
	v[3] = 0xf4005800u;
	installed = 1;
	ncd_fpx_wr_psr(psr);
}

void ncd_fpx_uninstall(void)
{
	volatile uint32_t *v;
	uint32_t psr;
	if (!installed)
		return;
	psr = ncd_fpx_rd_psr();
	ncd_fpx_wr_psr(psr | 2);
	v = (volatile uint32_t *)(ncd_fpx_rd_vbr() + NCD_FPX_VEC_PRECISE * 8);
	v[0] = saved[0];
	v[1] = saved[1];
	v[2] = saved[2];
	v[3] = saved[3];
	installed = 0;
	ncd_fpx_wr_psr(psr);
}
