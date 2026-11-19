#include <stdio.h>
extern int zfoo_value; extern int *zfoo_ptr; extern int zfoo_add(int);
int main(void) {
	printf("zfoo %d %d %d\n", zfoo_add(2), zfoo_ptr == &zfoo_value, *zfoo_ptr);
	return zfoo_add(1) == 6 ? 0 : 1;
}
