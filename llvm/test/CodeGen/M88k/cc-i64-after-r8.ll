; A 64-bit argument that finds only r9 free goes on the stack, and so do all
; following arguments (88open BCS); r9 stays unused. va_arg relies on this.
;
; RUN: llc < %s -mtriple=m88k-unknown-none-elf -mcpu=mc88100 -m88k-enable-delay-slot-filler=false | FileCheck %s

declare void @callee(i32, i32, i32, i32, i32, i32, i32, double, i32)

define void @caller() {
; CHECK-LABEL: caller:
; CHECK-NOT:   %r9
; CHECK:       bsr callee
  call void @callee(i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, double 1.0, i32 9)
  ret void
}

define i32 @last(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e, i32 %f, i32 %g, double %h, i32 %i) {
; CHECK-LABEL: last:
; CHECK-NOT:   %r9
; CHECK:       ld %r2, %r{{[0-9]+}}, {{[0-9]+}}
  ret i32 %i
}
