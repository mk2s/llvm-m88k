; Regression test for a bug found during NCD 19c bare-metal bring-up: an i1
; argument feeding a branch and a phi failed instruction selection at -O2 with
; "cannot select: G_TRUNC s32->s1". A truncation to s1 is now a copy, because
; a boolean is held in bit 0 of a register.
;
; RUN: llc < %s -mtriple=m88k-unknown-none-elf -mcpu=mc88100 -O2 -verify-machineinstrs -m88k-enable-delay-slot-filler=false | FileCheck %s
define i32 @f(i1 zeroext %c, i32 %a, i32 %b) {
; CHECK-LABEL: f:
; CHECK:         or %r5, %r0, %r2
; CHECK-NEXT:    or %r2, %r0, %r3
; CHECK-NEXT:    bb1 0, %r5, .LBB0_2
; CHECK-NEXT:  | %bb.1:
; CHECK-NEXT:    or %r2, %r0, %r4
; CHECK-NEXT:  .LBB0_2:
; CHECK-NEXT:    jmp %r1
entry:
  br i1 %c, label %t, label %e
t:
  br label %m
e:
  br label %m
m:
  %p = phi i32 [ %a, %t ], [ %b, %e ]
  ret i32 %p
}
