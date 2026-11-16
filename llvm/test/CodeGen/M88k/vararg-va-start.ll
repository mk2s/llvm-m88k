; va_start fills the 88open BCS va_list { __va_arg, __va_stk, __va_reg }:
; __va_arg is the argument word index of the first variadic argument (not
; shifted into the upper half), __va_stk is biased so that word 8 is the first
; stack word, and __va_reg is biased so that word N is at __va_reg + 4*N.
;
; RUN: llc < %s -mtriple=m88k-unknown-none-elf -mcpu=mc88100 -m88k-enable-delay-slot-filler=false -verify-machineinstrs | FileCheck %s

declare void @llvm.va_start.p0(ptr)
declare void @g(ptr)

; Three named words: r2, r3:r4 is not a pair, so the double is in r4:r5.
; The first variadic word is 4 (r6); the save area starts with r6:r7.
define void @three(i32 %a, double %b, ...) {
; CHECK-LABEL: three:
; CHECK:       or %r{{[0-9]+}}, %r0, 4
; CHECK:       subu %r{{[0-9]+}}, %r{{[0-9]+}}, 32
; CHECK:       subu %r{{[0-9]+}}, %r{{[0-9]+}}, 16
  %ap = alloca [3 x i32], align 4
  call void @llvm.va_start.p0(ptr %ap)
  call void @g(ptr %ap)
  ret void
}

; Named arguments on the stack: the first variadic word follows them.
define void @stack(i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4, i32 %a5,
                   i32 %a6, i32 %a7, i32 %a8, ...) {
; CHECK-LABEL: stack:
; CHECK:       or %r{{[0-9]+}}, %r0, 9
  %ap = alloca [3 x i32], align 4
  call void @llvm.va_start.p0(ptr %ap)
  call void @g(ptr %ap)
  ret void
}
