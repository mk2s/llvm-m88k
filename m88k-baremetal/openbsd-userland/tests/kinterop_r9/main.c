#include <stdio.h>
struct SD { double d; };
double lib_manyd(double, int, double, int, double, int, struct SD, double, int);
int lib_bf(double, int, double, int, double, int);
int main(void) {
  struct SD sd = {0.5};
  printf("%g\n", lib_manyd(1.0, 2, 3.0, 4, 5.0, 6, sd, 7.0, 8));
  printf("%d\n", lib_bf(1.0, 2, 3.0, 4, 5.0, 6));
  return 0;
}
