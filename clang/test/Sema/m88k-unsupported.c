// RUN: %clang_cc1 -triple m88k-unknown-none-elf -fsyntax-only -verify %s
// Features the m88k target does not provide are diagnosed.

// __int128 is not available on a 32-bit target.
__int128 i128; // expected-error {{__int128 is not supported on this target}}

// _BitInt is not supported.
_BitInt(8) b8;  // expected-error {{_BitInt is not supported on this target}}
_BitInt(64) b64; // expected-error {{_BitInt is not supported on this target}}

// Builtins take exactly one unsigned int.
int ff1_none(void) { return __builtin_m88k_ff1(); }     // expected-error {{too few arguments to function call, expected 1, have 0}}
int ff1_two(void) { return __builtin_m88k_ff1(1, 2); }  // expected-error {{too many arguments to function call, expected 1, have 2}}
int ff0_ptr(void) { return __builtin_m88k_ff0("x"); }   // expected-error {{incompatible pointer to integer conversion passing 'char[2]' to parameter of type 'unsigned int'}}
