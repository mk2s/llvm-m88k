/* debug-demo: a small program to try the debugger on.
 * Prints Fibonacci numbers on the screen (the serial port belongs to the debugger).
 * Good places for breakpoints: fib(), the line that updates `stats`, print_line(). */

void ncd_rom_out1char(int c);

struct stats {
  unsigned rounds;   /* completed passes over 0..LIMIT */
  unsigned calls;    /* fib() calls so far */
  unsigned largest;  /* largest value seen */
};

#define LIMIT 24

struct stats stats;
unsigned values[LIMIT + 1];

static void put_char(char c) {
  ncd_rom_out1char((unsigned char)c);
}

static void put_str(const char *s) {
  while (*s)
    put_char(*s++);
}

static void put_uint(unsigned v) {
  char buf[12];
  int n = 0;
  do {
    buf[n++] = (char)('0' + v % 10);
    v /= 10;
  } while (v);
  while (n)
    put_char(buf[--n]);
}

unsigned fib(unsigned n) {
  stats.calls++;
  if (n < 2)
    return n;
  unsigned a = 0, b = 1;
  for (unsigned i = 2; i <= n; i++) {
    unsigned next = a + b;
    a = b;
    b = next;
  }
  return b;
}

static void print_line(unsigned n, unsigned value) {
  put_str("  fib(");
  put_uint(n);
  put_str(") = ");
  put_uint(value);
  put_char('\n');
}

int main(void) {
  put_str("\n  debug-demo running\n");
  for (;;) {
    for (unsigned n = 0; n <= LIMIT; n++) {
      unsigned value = fib(n);
      values[n] = value;
      if (value > stats.largest)
        stats.largest = value;
      print_line(n, value);
    }
    stats.rounds++;
  }
}
