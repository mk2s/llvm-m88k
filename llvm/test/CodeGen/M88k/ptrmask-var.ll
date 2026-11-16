; llvm.ptrmask with a mask that is not a compile-time constant (memalign).
;
; RUN: llc < %s -mtriple=m88k-unknown-none-elf -mcpu=mc88100 -m88k-enable-delay-slot-filler=false | FileCheck %s

declare ptr @llvm.ptrmask.p0.i32(ptr, i32)

define ptr @mask(ptr %p, i32 %m) {
; CHECK-LABEL: mask:
; CHECK:       and %r2, %r2, %r3
  %r = call ptr @llvm.ptrmask.p0.i32(ptr %p, i32 %m)
  ret ptr %r
}
