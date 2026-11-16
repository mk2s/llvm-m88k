// RUN: %clang_cc1 -triple m88k-unknown-netbsd -emit-llvm -o - %s | FileCheck --check-prefix=NETBSD %s
// RUN: %clang_cc1 -triple m88k-unknown-openbsd -emit-llvm -o - %s | FileCheck --check-prefix=OPENBSD %s

// size_t is unsigned int on NetBSD and unsigned long on OpenBSD; both are
// 32 bits wide, so check the type name via _Generic.
typedef __SIZE_TYPE__ size_t;
int is_uint(void) { return _Generic((size_t)0, unsigned int: 1, default: 0); }
int ptrdiff_is_int(void) { return _Generic((char *)0 - (char *)0, int: 1, default: 0); }
// NETBSD-LABEL: define {{.*}}i32 @is_uint()
// NETBSD: ret i32 1
// NETBSD-LABEL: define {{.*}}i32 @ptrdiff_is_int()
// NETBSD: ret i32 1
// OPENBSD-LABEL: define {{.*}}i32 @is_uint()
// OPENBSD: ret i32 0
// OPENBSD-LABEL: define {{.*}}i32 @ptrdiff_is_int()
// OPENBSD: ret i32 1
