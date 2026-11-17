// RUN: %clang_cc1 -triple m88k-unknown-none-elf -fsyntax-only -verify %s
// Inline asm operand constraints and register names on m88k.

int add1(int x) {
  int r;
  __asm__("addu %0,%1,1" : "=r"(r) : "r"(x));
  return r;
}
int add_imm(int x) {
  int r;
  __asm__("addu %0,%1,%2" : "=r"(r) : "r"(x), "i"(5));
  return r;
}
void clobbers(int *p) {
  __asm__ volatile("st %%r0,%0,0" ::"r"(p) : "memory", "cc", "r1", "r2", "r14", "r30", "r31");
}
void reg_var(void) {
  register int x __asm__("r14") = 1;
  __asm__ volatile("" ::"r"(x));
}
void bad_clobber(void) {
  __asm__ volatile("" ::: "r32"); // expected-error {{unknown register name 'r32' in asm}}
}
void bad_constraint(void) {
  __asm__ volatile("" ::"Z"(1)); // expected-error {{invalid input constraint 'Z' in asm}}
}
