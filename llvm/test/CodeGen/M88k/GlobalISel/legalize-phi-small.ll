; Regression test: G_PHI on s8/s16 must be widened to s32 (crashed with
; "unable to legalize instruction: G_PHI" when compiling a C string loop).
;
; RUN: llc < %s -mtriple=m88k-unknown-none-elf -mcpu=mc88100 -verify-machineinstrs | FileCheck %s

; CHECK-LABEL: count_nonzero:
; CHECK:         ld.bu %r4, %r2, 0
; CHECK:       .LBB0_1:
; CHECK:         addu %r2, %r2, %r4
; CHECK:         ld.bu %r4, %r3, 1
; CHECK:         bcnd.n ne0, %r4, .LBB0_1
define i32 @count_nonzero(ptr %s) {
entry:
  %c0 = load i8, ptr %s
  %z0 = icmp eq i8 %c0, 0
  br i1 %z0, label %exit, label %loop

loop:
  %c = phi i8 [ %c0, %entry ], [ %cn, %loop ]
  %p = phi ptr [ %s, %entry ], [ %pn, %loop ]
  %n = phi i32 [ 0, %entry ], [ %nn, %loop ]
  %ext = zext i8 %c to i32
  %nn = add i32 %n, %ext
  %pn = getelementptr i8, ptr %p, i32 1
  %cn = load i8, ptr %pn
  %zn = icmp eq i8 %cn, 0
  br i1 %zn, label %exit, label %loop

exit:
  %r = phi i32 [ 0, %entry ], [ %nn, %loop ]
  ret i32 %r
}

; CHECK-LABEL: phi_i16:
; CHECK:         bcnd.n gt0, %r5, .LBB1_2
; CHECK-NEXT:    or %r2, %r0, %r3
; CHECK:         or %r2, %r0, %r4
; CHECK:         jmp %r1
define i16 @phi_i16(i32 %x, i16 %a, i16 %b) {
entry:
  %c = icmp sgt i32 %x, 0
  br i1 %c, label %t, label %f
t:
  br label %m
f:
  br label %m
m:
  %r = phi i16 [ %a, %t ], [ %b, %f ]
  ret i16 %r
}
