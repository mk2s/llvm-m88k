#include <math.h>
#include <stdio.h>
int main(void) {
  volatile double x = 1.5, y = -2.25; volatile float f = 0.1f;
  printf("%.9g %.9g %.9g %.9g\n", x + y, x - y, x * y, x / y);
  printf("%.9g %.9g %.9g\n", (double)f, (double)(f * 3.0f), (double)(f / 3.0f));
  printf("%.9g %.9g %.9g %.9g\n", sqrt(2.0), sin(1.0), cos(1.0), atan2(1.0, 2.0));
  printf("%.9g %.9g %.9g %.9g\n", pow(2.0, 0.5), exp(1.0), log(10.0), fmod(10.5, 3.0));
  printf("%.9g %.9g %.9g %.9g\n", floor(-1.5), ceil(-1.5), fabs(y), trunc(2.7));
  static const double c[] = {0.0, 1.0, -1.0, 0.5, 123456.789, -987654.321, 2147483000.0, -2147483000.0, 1e9, 4294967000.0};
  for (unsigned i = 0; i < sizeof c / sizeof *c; i++) {
    volatile double d = c[i]; volatile float g = (float)c[i];
    printf("%.9g -> int %d uint %u ll %lld | f %d\n", (double)d, (int)(d > 2147483647.0 || d < -2147483648.0 ? 0 : d),
           (unsigned)(d < 0 || d > 4294967295.0 ? 0 : d), (long long)d, (int)(g > 2147483000.0f || g < -2147483000.0f ? 0 : g));
  }
  volatile int i = -123456789; volatile unsigned u = 4000000000u; volatile long long ll = -1234567890123LL; volatile unsigned long long ull = 12345678901234567ULL;
  printf("%.9g %.9g %.9g %.9g\n", (double)i, (double)u, (double)ll, (double)ull);
  printf("%.9g %.9g %.9g %.9g\n", (double)(float)i, (double)(float)u, (double)(float)ll, (double)(float)ull);
  return 0;
}
