; rint, nearbyint and roundeven become libcalls (no FP rounding instruction
; with these semantics exists on the MC88100).
;
; RUN: llc < %s -mtriple=m88k-unknown-none-elf -mcpu=mc88100 -m88k-enable-delay-slot-filler=false | FileCheck %s

declare float @llvm.rint.f32(float)
declare double @llvm.rint.f64(double)
declare double @llvm.nearbyint.f64(double)
declare float @llvm.roundeven.f32(float)
declare double @llvm.roundeven.f64(double)

define float @t_rintf(float %x) {
; CHECK-LABEL: t_rintf:
; CHECK:       bsr rintf
  %r = call float @llvm.rint.f32(float %x)
  ret float %r
}

define double @t_rint(double %x) {
; CHECK-LABEL: t_rint:
; CHECK:       bsr rint
  %r = call double @llvm.rint.f64(double %x)
  ret double %r
}

define double @t_nearbyint(double %x) {
; CHECK-LABEL: t_nearbyint:
; CHECK:       bsr nearbyint
  %r = call double @llvm.nearbyint.f64(double %x)
  ret double %r
}

define float @t_roundevenf(float %x) {
; CHECK-LABEL: t_roundevenf:
; CHECK:       bsr roundevenf
  %r = call float @llvm.roundeven.f32(float %x)
  ret float %r
}

define double @t_roundeven(double %x) {
; CHECK-LABEL: t_roundeven:
; CHECK:       bsr roundeven
  %r = call double @llvm.roundeven.f64(double %x)
  ret double %r
}
