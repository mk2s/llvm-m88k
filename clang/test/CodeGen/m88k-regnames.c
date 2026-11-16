// RUN: %clang_cc1 -triple m88k-unknown-none-elf -target-cpu mc88100 -fsyntax-only -verify %s
// expected-no-diagnostics

// r30 must be accepted as a clobber register name (GCCRegNames typo r39).
void f(void) { __asm__ volatile("" ::: "r30"); }
