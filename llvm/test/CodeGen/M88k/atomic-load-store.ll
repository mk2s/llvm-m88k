; Aligned atomic loads and stores of up to 32 bits select as plain ld/st.
;
; RUN: llc < %s -mtriple=m88k-openbsd -mcpu=mc88100 -m88k-enable-delay-slot-filler=false -verify-machineinstrs | FileCheck %s

; CHECK-LABEL: store_monotonic:
; CHECK: st %r3, %r2, 0
define void @store_monotonic(ptr %p, i32 %v) {
  store atomic i32 %v, ptr %p monotonic, align 4
  ret void
}

; CHECK-LABEL: store_release:
; CHECK: st %r3, %r2, 0
define void @store_release(ptr %p, i32 %v) {
  store atomic i32 %v, ptr %p release, align 4
  ret void
}

; CHECK-LABEL: load_acquire:
; CHECK: ld %r2, %r2, 0
define i32 @load_acquire(ptr %p) {
  %v = load atomic i32, ptr %p acquire, align 4
  ret i32 %v
}
