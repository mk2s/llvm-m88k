// Size and alignment of the fundamental types on m88k (big endian, 32-bit
// pointers, 8-byte alignment for 64-bit scalars, long double == double).
// RUN: %clang_cc1 -triple m88k-unknown-none-elf -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple m88k-unknown-openbsd -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple m88k-unknown-netbsd -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple m88k-unknown-none-elf -target-cpu mc88110 -fsyntax-only -verify %s
// expected-no-diagnostics

_Static_assert(sizeof(char) == 1 && _Alignof(char) == 1, "char");
_Static_assert(sizeof(short) == 2 && _Alignof(short) == 2, "short");
_Static_assert(sizeof(int) == 4 && _Alignof(int) == 4, "int");
_Static_assert(sizeof(long) == 4 && _Alignof(long) == 4, "long");
_Static_assert(sizeof(long long) == 8 && _Alignof(long long) == 8, "long long");
_Static_assert(sizeof(void *) == 4 && _Alignof(void *) == 4, "pointer");
_Static_assert(sizeof(float) == 4 && _Alignof(float) == 4, "float");
_Static_assert(sizeof(double) == 8 && _Alignof(double) == 8, "double");
_Static_assert(sizeof(long double) == 8 && _Alignof(long double) == 8, "long double is double");
_Static_assert(sizeof(__SIZE_TYPE__) == 4, "size_t");
_Static_assert(sizeof(__PTRDIFF_TYPE__) == 4, "ptrdiff_t");
_Static_assert(sizeof(__INTPTR_TYPE__) == 4, "intptr_t");
_Static_assert(sizeof(__WCHAR_TYPE__) == 4, "wchar_t");
_Static_assert(sizeof(_Bool) == 1, "bool");
_Static_assert(sizeof(_Complex float) == 8 && _Alignof(_Complex float) == 4, "complex float");
_Static_assert(sizeof(_Complex double) == 16 && _Alignof(_Complex double) == 8, "complex double");

// char is signed.
_Static_assert((char)-1 < 0, "char is signed");

// Struct layout follows natural alignment.
struct S1 { char c; int i; };
_Static_assert(sizeof(struct S1) == 8 && __builtin_offsetof(struct S1, i) == 4, "S1");
struct S2 { char c; long long l; };
_Static_assert(sizeof(struct S2) == 16 && __builtin_offsetof(struct S2, l) == 8, "S2");
struct S3 { char c; double d; char e; };
_Static_assert(sizeof(struct S3) == 24 && _Alignof(struct S3) == 8, "S3");
struct __attribute__((packed)) S4 { char c; int i; };
_Static_assert(sizeof(struct S4) == 5 && _Alignof(struct S4) == 1, "packed");

// The va_list is { int, ptr, ptr } (88open BCS).
_Static_assert(sizeof(__builtin_va_list) == 12, "va_list size");
_Static_assert(_Alignof(__builtin_va_list) == 4, "va_list alignment");

_Static_assert(__BIGGEST_ALIGNMENT__ == 8, "max alignment");
_Static_assert(__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__, "big endian");
