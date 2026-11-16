/* Runtime test for the compiler-rt builtins and the code the M88k backend
 * generates around them: 64 bit mul/div/rem (__muldi3, __divdi3, __udivdi3,
 * __moddi3, __umoddi3), 64 bit shifts, int <-> FP conversions (__floatdidf,
 * __fixdfdi, __fixunsdfdi, __floatundidf, ...), fcmp, bit counting, bswap,
 * copysign and overflow checks.
 *
 * Every result is printed as hex. Build for the host with -DHOST to produce
 * the expected output (tests/rt-builtins.expected):
 *   cc -DHOST -O2 -o /tmp/rt-host tests/rt-builtins.c && /tmp/rt-host
 * NaN and infinity operands are not used: MAME does not model them, and a real
 * MC88100 traps on them.
 */
#include <stdint.h>
#ifdef HOST
#include <stdio.h>
static void out(const char *s) { fputs(s, stdout); }
#else
#include "../ncd19c_console.h"
static void out(const char *s) { ncd_console_puts(s); }
#endif

static unsigned nlines;

static void put_hex(uint64_t v, int digits) {
  char buf[17];
  for (int i = digits - 1; i >= 0; --i, v >>= 4)
    buf[i] = "0123456789abcdef"[v & 15];
  buf[digits] = 0;
  out(buf);
}

static void line(const char *tag, unsigned i, uint64_t v, int digits) {
  out(tag);
  out(" ");
  put_hex(i, 2);
  out(" ");
  put_hex(v, digits);
  out("\n");
  ++nlines;
}

static uint64_t dbits(double d) {
  union { double d; uint64_t u; } x = {d};
  return x.u;
}

static uint32_t fbits(float f) {
  union { float f; uint32_t u; } x = {f};
  return x.u;
}

/* volatile, so the operations are done at run time. */
static volatile int64_t A[] = {
    0, 1, -1, 7, -7, 1000003, -123456789012345ll, 0x0123456789abcdefll,
    INT64_MAX, INT64_MIN + 1, 0x00000000ffffffffll, -0x100000000ll};
static volatile int64_t B[] = {
    3, -3, 1, 0x100000001ll, -1000003, 65536, 0x7fffffffll, -0x80000000ll,
    9, -2, 0xfedcba987ll, 12345};
static volatile unsigned S[] = {0, 1, 7, 31, 32, 33, 47, 63, 4, 16, 40, 62};
static volatile double D[] = {
    0.0, 1.0, -1.0, 0.5, -2.75, 123456789.875, -9.0071992547409e15,
    4.611686018427388e18, 1e-300, 3.0e10, -1.5e18, 6.02214076e23};
static volatile float F[] = {0.0f, 1.0f, -1.0f, 0.5f, -2.75f, 1.0e9f,
                             -3.0e12f, 1.5e18f, 1e-30f, 3.0e10f, -7.0f, 2.5f};
static volatile uint32_t U[] = {0, 1, 0x7fffffff, 0x80000000, 0x80000001,
                                0xffffffff, 12345, 0xdeadbeef};

#define N(x) (sizeof(x) / sizeof((x)[0]))

int main(void) {
  out("rt-builtins start\n");
  for (unsigned i = 0; i < N(A); ++i) {
    int64_t a = A[i], b = B[i];
    uint64_t ua = (uint64_t)a, ub = (uint64_t)b;
    unsigned s = S[i];
    line("mul", i, ua * ub, 16); /* unsigned: no UB on overflow */
    line("sdiv", i, (uint64_t)(a / b), 16);
    line("srem", i, (uint64_t)(a % b), 16);
    line("udiv", i, ua / ub, 16);
    line("urem", i, ua % ub, 16);
    /* Remainders without the matching division, so they are not rewritten
       as a - (a / b) * b and really call __moddi3/__umoddi3. */
    line("srem2", i, (uint64_t)(a % B[(i + 1) % N(B)]), 16);
    line("urem2", i, ua % (uint64_t)B[(i + 2) % N(B)], 16);
    line("shl", i, ua << s, 16);
    line("ashr", i, (uint64_t)(a >> s), 16);
    line("lshr", i, ua >> s, 16);
    line("i2d", i, dbits((double)a), 16);
    line("u2d", i, dbits((double)ua), 16);
    line("i2f", i, fbits((float)a), 8);
    line("u2f", i, fbits((float)ua), 8);
    line("clz", i, (unsigned)__builtin_clzll(ua | 1), 2);
    line("ctz", i, (unsigned)__builtin_ctzll(ua | 0x8000000000000000ull), 2);
    line("pop", i, (unsigned)__builtin_popcountll(ua), 2);
    line("bsw64", i, __builtin_bswap64(ua), 16);
    line("bsw32", i, __builtin_bswap32((uint32_t)ua), 8);
    int64_t m64;
    int o64 = __builtin_mul_overflow(a, b, &m64);
    int32_t m32;
    int o32 = __builtin_mul_overflow((int32_t)a, (int32_t)b, &m32);
    uint32_t mu32;
    int ou32 = __builtin_mul_overflow((uint32_t)a, (uint32_t)b, &mu32);
    line("mulo", i, (unsigned)(o64 | o32 << 1 | ou32 << 2), 2);
    line("sx8", i, (uint64_t)(int64_t)(int8_t)ua, 16);
    line("sx16", i, (uint32_t)(int32_t)(int16_t)ua, 8);
    line("clz0", i, ua ? (unsigned)__builtin_clzll(ua) : 64u, 2);
    uint32_t d32;
    uint64_t d64;
    int bo32 = __builtin_sub_overflow((uint32_t)ua, (uint32_t)ub, &d32);
    int bo64 = __builtin_sub_overflow(ua, ub, &d64);
    int co64 = __builtin_add_overflow(ua, ub, &d64);
    line("ovf", i, (unsigned)(bo32 | bo64 << 1 | co64 << 2), 2);
  }
  for (unsigned i = 0; i < N(D); ++i) {
    double d = D[i], e = D[(i + 5) % N(D)];
    float f = F[i], g = F[(i + 3) % N(F)];
    /* Only convert values that are in range of the destination type. */
    if (d > -9.2e18 && d < 9.2e18)
      line("d2i", i, (uint64_t)(int64_t)d, 16);
    if (d >= 0 && d < 1.8e19)
      line("d2u", i, (uint64_t)d, 16);
    if (f > -9.2e18f && f < 9.2e18f)
      line("f2i", i, (uint64_t)(int64_t)f, 16);
    if (f >= 0 && f < 1.8e19f)
      line("f2u", i, (uint64_t)f, 16);
    if (d >= 0 && d < 4.2e9)
      line("d2u32", i, (uint32_t)d, 8);
    unsigned cd = (d < e) | (d <= e) << 1 | (d > e) << 2 | (d >= e) << 3 |
                  (d == e) << 4 | (d != e) << 5 | (d == d) << 6 |
                  (!(d < e)) << 7;
    unsigned cf = (f < g) | (f <= g) << 1 | (f > g) << 2 | (f >= g) << 3 |
                  (f == g) << 4 | (f != g) << 5 | (f == f) << 6 |
                  (!(f >= g)) << 7;
    line("fcmpd", i, cd, 2);
    line("fcmpf", i, cf, 2);
    line("csgn", i, dbits(__builtin_copysign(d, e)), 16);
    line("fabs", i, fbits(__builtin_fabsf(f)), 8);
    line("dmix", i, dbits(d * e + (double)f / 3.0 - e), 16);
  }
  for (unsigned i = 0; i < N(U); ++i) {
    line("u32d", i, dbits((double)U[i]), 16);
    line("u32f", i, fbits((float)U[i]), 8);
  }
  out("lines ");
  put_hex(nlines, 4);
  out("\nRT DONE\n");
  return 0;
}
