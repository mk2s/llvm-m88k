; RUN: llc -mtriple=m88k -O0 < %s | FileCheck %s
; RUN: llc -mtriple=m88k -O2 < %s | FileCheck %s

; m88k is big-endian. The load_and_mask combine must not turn
; (and (load i32 p), 255) into a byte load from p, which would read the most
; significant byte (miscompiled callout_hardclock() in the NetBSD kernel).

@g = global i32 0

; CHECK-LABEL: f8:
; CHECK-NOT:   ld.bu
; CHECK:       ld %r{{[0-9]+}}, %r{{[0-9]+}}, %lo16(g)
; CHECK:       mask %r2, %r{{[0-9]+}}, 255
define i32 @f8() {
  %v = load i32, ptr @g
  %a = and i32 %v, 255
  ret i32 %a
}

; CHECK-LABEL: f16:
; CHECK-NOT:   ld.hu
; CHECK:       ld %r{{[0-9]+}}, %r2, 0
; CHECK:       mask %r2, %r{{[0-9]+}}, 65535
define i32 @f16(ptr %p) {
  %v = load i32, ptr %p
  %a = and i32 %v, 65535
  ret i32 %a
}

; A load of the same width as the mask is still fine.
; CHECK-LABEL: h8:
; CHECK:       ld.bu %r2, %r2, 0
define i32 @h8(ptr %p) {
  %v = load i8, ptr %p
  %z = zext i8 %v to i32
  %a = and i32 %z, 255
  ret i32 %a
}
