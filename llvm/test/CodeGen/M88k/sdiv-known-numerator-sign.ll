; sdiv where the sign of the numerator is known but the sign of the
; denominator is not. The negative-denominator block used to be mixed up with
; the non-negative one, which put a second G_UDIV after a terminator.
;
; RUN: llc < %s -mtriple=m88k-unknown-none-elf -mcpu=mc88100 -verify-machineinstrs | FileCheck %s

define i32 @num_nonneg(i16 zeroext %a, i32 %b) {
; CHECK-LABEL: num_nonneg:
; CHECK:       divu
; CHECK:       divu
  %x = zext i16 %a to i32
  %r = sdiv i32 %x, %b
  ret i32 %r
}

define i32 @num_neg(i16 signext %a, i32 %b) {
; CHECK-LABEL: num_neg:
; CHECK:       divu
  %y = sext i16 %a to i32
  %z = or i32 %y, -2147483648
  %r = sdiv i32 %z, %b
  ret i32 %r
}
