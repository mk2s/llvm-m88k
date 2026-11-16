// va_arg uses the 88open BCS va_list { __va_arg, __va_stk, __va_reg }, where
// __va_arg is an argument word index. 8-byte types start at an even word.
// RUN: %clang_cc1 -triple m88k-unknown-none-elf -O1 -disable-llvm-passes -emit-llvm -o - %s | FileCheck %s

#include <stdarg.h>

// CHECK-LABEL: @get_int(
// CHECK:       [[IDX:%.*]] = load i32, ptr {{.*}}
// CHECK:       [[NEXT:%.*]] = add i32 [[IDX]], 1
// CHECK:       store i32 [[NEXT]]
// CHECK:       icmp ule i32 [[NEXT]], 8
// CHECK:       select i1
// CHECK:       mul i32 [[IDX]], 4
// CHECK:       load i32, ptr {{.*}}, align 4
int get_int(va_list ap) { return va_arg(ap, int); }

// CHECK-LABEL: @get_double(
// CHECK:       [[IDX:%.*]] = load i32, ptr {{.*}}
// CHECK:       [[T:%.*]] = add i32 [[IDX]], 1
// CHECK:       [[AL:%.*]] = and i32 [[T]], -2
// CHECK:       [[NEXT:%.*]] = add i32 [[AL]], 2
// CHECK:       icmp ule i32 [[NEXT]], 8
// CHECK:       mul i32 [[AL]], 4
// CHECK:       load double, ptr {{.*}}, align 8
double get_double(va_list ap) { return va_arg(ap, double); }

struct S { int a, b, c; };
// Aggregates are passed by reference: one word holding a pointer.
// CHECK-LABEL: @get_struct(
// CHECK:       add i32 {{.*}}, 1
// CHECK:       load ptr, ptr {{.*}}, align 4
int get_struct(va_list ap) { return va_arg(ap, struct S).c; }
