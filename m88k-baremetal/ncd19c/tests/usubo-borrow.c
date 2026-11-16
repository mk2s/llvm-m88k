/* Regression test for the usub.with.overflow borrow used as a value (0 or 1,
 * not -1) and for add x,-C / subtract-immediate selection.
 * Expected output from the host:
 *   cc -DHOST -O2 -o /tmp/ub-host tests/usubo-borrow.c && /tmp/ub-host > tests/usubo-borrow.expected
 */
#include <stdint.h>
#ifdef HOST
#include <stdio.h>
static void out(const char *s) { fputs(s, stdout); }
#else
#include "../ncd19c_console.h"
static void out(const char *s) { ncd_console_puts(s); }
#endif
static void put_hex(uint32_t v) {
  char buf[9];
  for (int i = 7; i >= 0; --i, v >>= 4) buf[i] = "0123456789abcdef"[v & 15];
  buf[8] = 0;
  out(buf);
}
static void line(const char *n, unsigned i, uint32_t v) {
  out(n); out(" "); put_hex(i); out(" "); put_hex(v); out("\n");
}
#define NOINL __attribute__((noinline))
/* add.ll f8: b + borrow(0 - a) */
NOINL static uint32_t f8(uint32_t a, uint32_t b) {
  uint32_t r;
  return b + __builtin_usub_overflow(0u, a, &r);
}
NOINL static uint32_t borrow(uint32_t a, uint32_t b) {
  uint32_t r;
  return __builtin_usub_overflow(a, b, &r);
}
NOINL static uint32_t borrow_res(uint32_t a, uint32_t b) {
  uint32_t r;
  (void)__builtin_usub_overflow(a, b, &r);
  return r;
}
NOINL static uint32_t borrow_sum(uint32_t a, uint32_t b, uint32_t c) {
  uint32_t r;
  return c + __builtin_usub_overflow(a, b, &r) + r;
}
NOINL static uint32_t borrow_neg(uint32_t a, uint32_t b) { /* 0 - borrow */
  uint32_t r;
  return 0u - __builtin_usub_overflow(a, b, &r);
}
NOINL static uint32_t borrow_sel(uint32_t a, uint32_t b) {
  uint32_t r;
  return __builtin_usub_overflow(a, b, &r) ? 0x1234u : 0x5678u;
}
NOINL static uint32_t subi(uint32_t a) { return a - 5u; }
NOINL static uint32_t subi_big(uint32_t a) { return a - 0xffffu; }
NOINL static uint32_t addn(uint32_t a) { return a + (uint32_t)-1; }
NOINL static uint32_t addn7(uint32_t a) { return a + (uint32_t)-7; }
NOINL static uint32_t addn_big(uint32_t a) { return a + (uint32_t)-65535; }
NOINL static uint32_t addn_huge(uint32_t a) { return a + (uint32_t)-65536; }
NOINL static uint32_t rsub(uint32_t a) { return 100u - a; }
NOINL static uint32_t imm_borrow(uint32_t a) {
  uint32_t r;
  return __builtin_usub_overflow(a, 10u, &r);
}
int main(void) {
  static volatile uint32_t V[] = {0, 1, 2, 5, 9, 10, 11, 100, 0xffff, 0x10000,
                                  0x7fffffff, 0x80000000u, 0xfffffffeu, 0xffffffffu};
  unsigned n = sizeof(V) / sizeof(V[0]);
  for (unsigned i = 0; i < n; ++i) {
    uint32_t a = V[i];
    line("f8", i, f8(a, 0x100u));
    line("f8b", i, f8(a, a));
    line("subi", i, subi(a));
    line("subib", i, subi_big(a));
    line("addn", i, addn(a));
    line("addn7", i, addn7(a));
    line("addnb", i, addn_big(a));
    line("addnh", i, addn_huge(a));
    line("rsub", i, rsub(a));
    line("ibor", i, imm_borrow(a));
    for (unsigned j = 0; j < n; ++j) {
      uint32_t b = V[j];
      line("bor", i * 16 + j, borrow(a, b));
      line("bres", i * 16 + j, borrow_res(a, b));
      line("bsum", i * 16 + j, borrow_sum(a, b, 7u));
      line("bneg", i * 16 + j, borrow_neg(a, b));
      line("bsel", i * 16 + j, borrow_sel(a, b));
    }
  }
  out("UB DONE\n");
  return 0;
}
