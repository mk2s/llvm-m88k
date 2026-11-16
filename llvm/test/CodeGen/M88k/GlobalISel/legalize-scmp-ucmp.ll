; Regression test: G_SCMP/G_UCMP (from the three-way compare idiom used by
; qsort comparators, (a > b) - (a < b)) must be lowered by the legalizer.
;
; RUN: llc < %s -mtriple=m88k-unknown-none-elf -mcpu=mc88100 -global-isel -verify-machineinstrs | FileCheck %s

; CHECK-LABEL: scmp32:
; CHECK:       cmp %r4, %r2, %r3
; CHECK:       subu %r2, %r4, %r2
define i32 @scmp32(i32 %a, i32 %b) {
  %r = call i32 @llvm.scmp.i32.i32(i32 %a, i32 %b)
  ret i32 %r
}

; CHECK-LABEL: ucmp32:
; CHECK:       subu.ci %r2, %r4, %r0
define i32 @ucmp32(i32 %a, i32 %b) {
  %r = call i32 @llvm.ucmp.i32.i32(i32 %a, i32 %b)
  ret i32 %r
}
