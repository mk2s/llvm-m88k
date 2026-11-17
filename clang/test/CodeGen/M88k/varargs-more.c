// REQUIRES: m88k-registered-target
// More va_arg cases for the 88open BCS va_list { __va_arg, __va_stk, __va_reg }.
// RUN: %clang_cc1 -triple m88k-unknown-none-elf -O1 -disable-llvm-passes -emit-llvm -o - %s | FileCheck %s
#include <stdarg.h>

// CHECK: %struct.__va_list_tag = type { i32, ptr, ptr }

// 64-bit integers start at an even argument word and occupy two words.
// CHECK-LABEL: @get_ll(
// CHECK:       [[T:%.*]] = add i32 {{.*}}, 1
// CHECK:       [[AL:%.*]] = and i32 [[T]], -2
// CHECK:       [[NEXT:%.*]] = add i32 [[AL]], 2
// CHECK:       icmp ule i32 [[NEXT]], 8
// CHECK:       load i64, ptr {{.*}}, align 8
long long get_ll(va_list ap) { return va_arg(ap, long long); }

// Pointers are one word.
// CHECK-LABEL: @get_ptr(
// CHECK:       add i32 {{.*}}, 1
// CHECK-NOT:   and i32 {{.*}}, -2
// CHECK:       load ptr, ptr {{.*}}, align 4
void *get_ptr(va_list ap) { return va_arg(ap, void *); }

// Arguments narrower than int are promoted by the caller, so va_arg reads ints.
// CHECK-LABEL: @get_int_from_char(
// CHECK:       load i32, ptr {{.*}}, align 4
int get_int_from_char(va_list ap) { return (char)va_arg(ap, int); }

// Both the register save area (__va_reg, words 0-7) and the overflow area
// (__va_stk) are used, selected by whether the argument still fits.
// CHECK-LABEL: @get_int(
// CHECK:       [[REG:%.*]] = load ptr, ptr %__va_reg.addr
// CHECK:       [[STK:%.*]] = load ptr, ptr %__va_stk.addr
// CHECK:       select i1 %fits_in_regs, ptr [[REG]], ptr [[STK]]
int get_int(va_list ap) { return va_arg(ap, int); }

// va_start initializes the whole 12-byte structure, va_copy copies it.
// CHECK-LABEL: @start(
// CHECK:       call void @llvm.va_start.p0(ptr {{.*}})
// CHECK:       call void @llvm.va_end.p0(ptr {{.*}})
int start(int n, ...) {
  va_list ap;
  va_start(ap, n);
  int r = va_arg(ap, int);
  va_end(ap);
  return r;
}
// CHECK-LABEL: @copy(
// CHECK:       call void @llvm.va_copy.p0(ptr {{.*}}, ptr {{.*}})
void copy(va_list a, va_list b) { va_copy(a, b); }
