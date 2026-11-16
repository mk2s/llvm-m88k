; Inline asm with register constraints must go through GlobalISel
; (picolibc memset_explicit uses an empty asm with a "r" input).
;
; RUN: llc < %s -mtriple=m88k-unknown-none-elf -mcpu=mc88100 | FileCheck %s

define ptr @asm_in(ptr %p) {
; CHECK-LABEL: asm_in:
; CHECK:       APP
; CHECK:       NO_APP
  tail call void asm sideeffect "", "r,~{memory}"(ptr %p)
  ret ptr %p
}

define i32 @asm_inout(i32 %a) {
; CHECK-LABEL: asm_inout:
; CHECK:       APP
; CHECK-NEXT:  or %r{{[0-9]+}}, %r0, %r{{[0-9]+}}
; CHECK-NEXT:  NO_APP
  %r = call i32 asm "or $0, %r0, $1", "=r,r"(i32 %a)
  ret i32 %r
}
