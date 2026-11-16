/* (uint32_t) of double and float. The backend must convert only in-range
 * values with trnc, since a real MC88100 raises an SFU1 precise exception
 * otherwise. Float inputs are chosen to be exactly representable.
 * Expected output (tests/fp2u32.expected) comes from the host:
 *   cc -DHOST -O2 -o /tmp/fp2u32 tests/fp2u32.c && /tmp/fp2u32
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
static volatile double D[] = {0, 1, 0.5, 2147483647.0, 2147483648.0,
                              2147483648.5, 3e9, 4294967295.0};
static volatile float F[] = {0, 1, 0.5f, 2147483520.0f, 2147483648.0f,
                             2147483904.0f, 3e9f, 4294967040.0f};
/* In [2^30, 2^31) the hardware trnc traps although the result fits; the default SFU1 handler
   completes it. Signed conversions too. */
static volatile double DS[] = {1073741823.0, 1073741824.0, 1500000000.5, 2147483647.0,
                               -1073741824.0, -1500000000.5, -2147483647.0, -2147483648.0};
static volatile float FS[] = {536870912.0f, 1073741824.0f, 1610612736.0f, 2147483520.0f,
                              -1073741824.0f, -1610612736.0f, -2147483520.0f, -2147483648.0f};
int main(void) {
  for (unsigned i = 0; i < 8; ++i)
    line("d2u32", i, (uint32_t)D[i], 8);
  for (unsigned i = 0; i < 8; ++i)
    line("f2u32", i, (uint32_t)F[i], 8);
  for (unsigned i = 0; i < 8; ++i)
    line("d2s32", i, (uint32_t)(int32_t)DS[i], 8);
  for (unsigned i = 0; i < 8; ++i)
    line("f2s32", i, (uint32_t)(int32_t)FS[i], 8);
  for (unsigned i = 0; i < 8; ++i)
    line("d2u32b", i, (uint32_t)(DS[i] < 0 ? -DS[i] : DS[i]), 8);
  out("DONE\n");
  return 0;
}
