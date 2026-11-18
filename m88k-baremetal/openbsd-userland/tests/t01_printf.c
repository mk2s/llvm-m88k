#include <stdio.h>
#include <string.h>
int main(void) {
  volatile int i = -12345; volatile unsigned u = 4000000000u;
  volatile long long ll = -123456789012345LL; volatile unsigned long long ull = 18446744073709551615ULL;
  volatile double d = 3.14159265358979; volatile float f = 2.5f;
  printf("int %d %i %5d|%-5d|%05d %x %X %o %c %s\n", (int)i, (int)i, 42, 42, 42, 255, 255, 8, 'z', "str");
  printf("unsigned %u %lu %hu %hhu\n", (unsigned)u, (unsigned long)u, (unsigned short)65537, (unsigned char)257);
  printf("llong %lld %llu %llx\n", (long long)ll, (unsigned long long)ull, (unsigned long long)ull);
  printf("double %f %.3f %e %g %10.4f|%-10.2f|\n", (double)d, (double)d, (double)d, (double)d, (double)d, (double)d);
  printf("float %f %g\n", (double)f, (double)f);
  printf("ptr-null %p %s\n", (void *)0, "x");
  printf("%5.2s|%-6s|%6s|\n", "abcdef", "ab", "cd");
  printf("%d%%\n", 100);
  char buf[64]; snprintf(buf, sizeof buf, "%d-%s-%.2f", 7, "q", 1.005); puts(buf);
  return 0;
}
