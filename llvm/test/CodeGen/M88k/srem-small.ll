; Signed remainder of types narrower than 32 bits is widened (strftime, localtime_r).
;
; RUN: llc < %s -mtriple=m88k-unknown-none-elf -mcpu=mc88100 | FileCheck %s

define i8 @srem8(i8 %a, i8 %b) {
; CHECK-LABEL: srem8:
; CHECK:       jmp
  %r = srem i8 %a, %b
  ret i8 %r
}

define i16 @srem16(i16 %a, i16 %b) {
; CHECK-LABEL: srem16:
; CHECK:       jmp
  %r = srem i16 %a, %b
  ret i16 %r
}
