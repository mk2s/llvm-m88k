; Math intrinsics that have no m88k instruction are lowered to libm calls.
; RUN: llc < %s -mtriple=m88k-unknown-openbsd -mcpu=mc88100 | FileCheck %s

; CHECK-LABEL: tan_double:
; CHECK: bsr{{(\.n)?}} tan
define double @tan_double(double %x) {
  %r = call double @llvm.tan.f64(double %x)
  ret double %r
}

; CHECK-LABEL: tan_float:
; CHECK: bsr{{(\.n)?}} tanf
define float @tan_float(float %x) {
  %r = call float @llvm.tan.f32(float %x)
  ret float %r
}

; CHECK-LABEL: acos_double:
; CHECK: bsr{{(\.n)?}} acos
define double @acos_double(double %x) {
  %r = call double @llvm.acos.f64(double %x)
  ret double %r
}

; CHECK-LABEL: acos_float:
; CHECK: bsr{{(\.n)?}} acosf
define float @acos_float(float %x) {
  %r = call float @llvm.acos.f32(float %x)
  ret float %r
}

; CHECK-LABEL: asin_double:
; CHECK: bsr{{(\.n)?}} asin
define double @asin_double(double %x) {
  %r = call double @llvm.asin.f64(double %x)
  ret double %r
}

; CHECK-LABEL: asin_float:
; CHECK: bsr{{(\.n)?}} asinf
define float @asin_float(float %x) {
  %r = call float @llvm.asin.f32(float %x)
  ret float %r
}

; CHECK-LABEL: atan_double:
; CHECK: bsr{{(\.n)?}} atan
define double @atan_double(double %x) {
  %r = call double @llvm.atan.f64(double %x)
  ret double %r
}

; CHECK-LABEL: atan_float:
; CHECK: bsr{{(\.n)?}} atanf
define float @atan_float(float %x) {
  %r = call float @llvm.atan.f32(float %x)
  ret float %r
}

; CHECK-LABEL: cosh_double:
; CHECK: bsr{{(\.n)?}} cosh
define double @cosh_double(double %x) {
  %r = call double @llvm.cosh.f64(double %x)
  ret double %r
}

; CHECK-LABEL: cosh_float:
; CHECK: bsr{{(\.n)?}} coshf
define float @cosh_float(float %x) {
  %r = call float @llvm.cosh.f32(float %x)
  ret float %r
}

; CHECK-LABEL: sinh_double:
; CHECK: bsr{{(\.n)?}} sinh
define double @sinh_double(double %x) {
  %r = call double @llvm.sinh.f64(double %x)
  ret double %r
}

; CHECK-LABEL: sinh_float:
; CHECK: bsr{{(\.n)?}} sinhf
define float @sinh_float(float %x) {
  %r = call float @llvm.sinh.f32(float %x)
  ret float %r
}

; CHECK-LABEL: tanh_double:
; CHECK: bsr{{(\.n)?}} tanh
define double @tanh_double(double %x) {
  %r = call double @llvm.tanh.f64(double %x)
  ret double %r
}

; CHECK-LABEL: tanh_float:
; CHECK: bsr{{(\.n)?}} tanhf
define float @tanh_float(float %x) {
  %r = call float @llvm.tanh.f32(float %x)
  ret float %r
}

; CHECK-LABEL: exp10_double:
; CHECK: bsr{{(\.n)?}} exp10
define double @exp10_double(double %x) {
  %r = call double @llvm.exp10.f64(double %x)
  ret double %r
}

; CHECK-LABEL: exp10_float:
; CHECK: bsr{{(\.n)?}} exp10f
define float @exp10_float(float %x) {
  %r = call float @llvm.exp10.f32(float %x)
  ret float %r
}

; CHECK-LABEL: atan2_double:
; CHECK: bsr{{(\.n)?}} atan2
define double @atan2_double(double %y, double %x) {
  %r = call double @llvm.atan2.f64(double %y, double %x)
  ret double %r
}

; CHECK-LABEL: atan2_float:
; CHECK: bsr{{(\.n)?}} atan2f
define float @atan2_float(float %y, float %x) {
  %r = call float @llvm.atan2.f32(float %y, float %x)
  ret float %r
}
