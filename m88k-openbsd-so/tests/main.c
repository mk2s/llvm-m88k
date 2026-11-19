#include <stdio.h>
extern int foo_counter, foo_init_ran;
extern int *foo_ptr;
extern int foo_add(int), foo_puts(void), foo_call_exe(int), foo_apply(int (*)(int), int);
extern int *foo_static_addr(void);
extern int foo_ext_add(int);
int exe_callback(int a) { return a * 2; }
static int triple(int a) { return a * 3; }
static int fails;
#define CHECK(c) do { if (c) printf("ok: %s\n", #c); else { printf("FAIL: %s\n", #c); fails++; } } while (0)
int main(void) {
	int (*fp)(int) = foo_add;
	foo_puts();
	CHECK(foo_init_ran == 1234);
	CHECK(foo_counter == 41);
	CHECK(foo_ptr == &foo_counter && *foo_ptr == 41);
	CHECK(foo_add(1) == 42);
	CHECK(fp(2) == 43);
	CHECK(foo_call_exe(10) == 21);
	CHECK(foo_apply(triple, 5) == 15);
	CHECK(foo_apply(foo_add, 9) == 50);
	CHECK(*foo_static_addr() == 7);
	foo_counter = 50;
	CHECK(foo_add(1) == 51 && *foo_ptr == 50);
	CHECK(foo_ext_add(0) == 150);
	printf("done fails=%d\n", fails);
	return fails;
}
