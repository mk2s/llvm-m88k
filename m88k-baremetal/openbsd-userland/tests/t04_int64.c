#include <stdio.h>
typedef long long ll; typedef unsigned long long ull;
int main(void) {
  static const ll v[] = {0, 1, -1, 2, 3, 255, 65536, 0x7fffffffLL, 0x80000000LL, -0x80000000LL, 0x123456789abcdefLL,
                         -0x123456789abcdefLL, 0x7fffffffffffffffLL, -0x7fffffffffffffffLL - 1, 1000000007LL, -999999937LL};
  ull h = 1469598103934665603ULL;
  for (unsigned i = 0; i < sizeof v / sizeof *v; i++)
    for (unsigned j = 0; j < sizeof v / sizeof *v; j++) {
      volatile ll a = v[i], b = v[j]; ull ua = (ull)a, ub = (ull)b;
      ll r[8]; unsigned n = 0;
      r[n++] = a + b; r[n++] = a - b; r[n++] = a * b;
      if (b != 0 && !(a == (-0x7fffffffffffffffLL - 1) && b == -1)) { r[n++] = a / b; r[n++] = a % b; }
      if (ub != 0) { r[n++] = (ll)(ua / ub); r[n++] = (ll)(ua % ub); }
      r[n++] = (ll)(ua << (j & 63)) ^ (a >> (j & 63)) ^ (ll)(ua >> (i & 63));
      for (unsigned k = 0; k < n; k++) { h ^= (ull)r[k]; h *= 1099511628211ULL; }
      if (i == j) printf("%2u: %lld %lld %llu\n", i, a + b, a * b, ua >> 3);
    }
  printf("hash %llx\n", h);
  return 0;
}
