; Regression test for a bug found during NCD 19c bare-metal bring-up: an i1
; argument feeding a branch and a phi fails instruction selection at -O2 with
; "cannot select: G_TRUNC s32->s1". Not yet fixed, so expected to fail.
;
; XFAIL: *
; RUN: llc < %s -mtriple=m88k-unknown-none-elf -mcpu=mc88100 -O2 -verify-machineinstrs -m88k-enable-delay-slot-filler=false | FileCheck %s

define i32 @f(i1 zeroext %c, i32 %a, i32 %b) {
; CHECK-LABEL: f:
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
