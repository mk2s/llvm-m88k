#include "../ncd19c_console.h"

static void put_hex(unsigned v) {
  for (int i = 28; i >= 0; i -= 4)
    ncd_console_putc("0123456789abcdef"[(v >> i) & 15]);
}

volatile unsigned seed = 7;

int main(void) {
  unsigned v = seed * 6u + 0x1234u; /* 0x125e */
  ncd_console_puts("boot ok\nvalue=0x");
  put_hex(v);
  ncd_console_puts("\n");
  return 0;
}
