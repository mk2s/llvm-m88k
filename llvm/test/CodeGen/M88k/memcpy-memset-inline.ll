; Regression test: llvm.memcpy.inline had no legalizer rule and llvm.memset.inline
; was left as an unlegalized intrinsic, aborting Zig's Debug-mode compiler_rt.
;
; RUN: llc < %s -mtriple=m88k-unknown-none-elf -mcpu=mc88100 -O0 -verify-machineinstrs -m88k-enable-delay-slot-filler=false | FileCheck %s

declare void @llvm.memcpy.inline.p0.p0.i32(ptr, ptr, i32, i1)
declare void @llvm.memset.inline.p0.i32(ptr, i8, i32, i1)

; CHECK-LABEL: cpy1:
; CHECK-NOT: bsr
; CHECK: ld.b
; CHECK: st.b
define void @cpy1(ptr %d, ptr %s) {
  call void @llvm.memcpy.inline.p0.p0.i32(ptr align 1 %d, ptr align 1 %s, i32 1, i1 false)
  ret void
}

; CHECK-LABEL: cpy8:
; CHECK-NOT: bsr
; CHECK: ld.d
; CHECK: st.d
define void @cpy8(ptr %d, ptr %s) {
  call void @llvm.memcpy.inline.p0.p0.i32(ptr align 8 %d, ptr align 8 %s, i32 8, i1 false)
  ret void
}

; CHECK-LABEL: set3:
; CHECK-NOT: bsr
; CHECK-COUNT-3: st.b
define void @set3(ptr %d, i8 %v) {
  call void @llvm.memset.inline.p0.i32(ptr align 1 %d, i8 %v, i32 3, i1 false)
  ret void
}
