; The address of a function comes from its GOT slot under PIC.
;
; RUN: llc < %s -mtriple=m88k-unknown-openbsd -relocation-model=pic -m88k-enable-delay-slot-filler=false -verify-machineinstrs | FileCheck %s

declare i32 @f(i32)
@fp = global ptr @f, align 4

; CHECK-LABEL: addr:
; CHECK:     or.u %r25, %r0, %abdiff_hi16(
; CHECK:     ld %r2, %r25, %got_lo16(f)
; CHECK-NOT: %hi16(f)
define ptr @addr() {
  ret ptr @f
}

; Calling through a global function pointer: the pointer variable is loaded
; through the GOT, then the function through the pointer.
; CHECK-LABEL: callfp:
; CHECK:     ld %r{{[0-9]+}}, %r25, %got_lo16(fp)
; CHECK:     jsr
define void @callfp() {
  %p = load ptr, ptr @fp
  call void %p()
  ret void
}
