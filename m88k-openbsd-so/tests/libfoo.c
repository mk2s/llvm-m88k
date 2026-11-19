#include <stdio.h>
int foo_counter = 41;
int *foo_ptr = &foo_counter;
int foo_init_ran;
extern int exe_callback(int);
static int local_static = 7;
int foo_add(int a) { return a + foo_counter; }
int foo_puts(void) { return puts("foo: hello from libfoo"); }
int foo_call_exe(int a) { return exe_callback(a) + 1; }
int *foo_static_addr(void) { return &local_static; }
int foo_apply(int (*f)(int), int a) { return f(a); }
int foo_ext_add(int a) { return foo_add(a) + 100; }
__attribute__((constructor)) static void foo_ctor(void) { foo_init_ran = 1234; }
