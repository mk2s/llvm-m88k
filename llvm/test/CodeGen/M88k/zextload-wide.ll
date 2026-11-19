; Regression test: a 64 bit load zero or sign extended to i128 produced a
; G_ZEXTLOAD/G_SEXTLOAD with a 64 bit memory type that could not be narrowed
; (Zig Debug-mode compiler_rt sqrt).
;
; RUN: llc < %s -mtriple=m88k-unknown-none-elf -mcpu=mc88100 -O0 -verify-machineinstrs -m88k-enable-delay-slot-filler=false | FileCheck %s

; CHECK-LABEL: zext:
; CHECK: ld.d
define i128 @zext(ptr %p) {
  %v = load i64, ptr %p, align 8
  %e = zext i64 %v to i128
  ret i128 %e
}

; CHECK-LABEL: sext:
; CHECK: ld.d
define i128 @sext(ptr %p) {
  %v = load i64, ptr %p, align 8
  %e = sext i64 %v to i128
  ret i128 %e
}
