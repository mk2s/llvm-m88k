; llvm.va_copy copies the 12 byte va_list struct word by word.
;
; RUN: llc < %s -mtriple=m88k-unknown-none-elf -mcpu=mc88100 -m88k-enable-delay-slot-filler=false | FileCheck %s

declare void @llvm.va_copy.p0(ptr, ptr)

define void @copy(ptr %dst, ptr %src) {
; CHECK-LABEL: copy:
; CHECK:       ld %r{{[0-9]+}}, %r3, 0
; CHECK:       st %r{{[0-9]+}}, %r2, 0
; CHECK:       ld %r{{[0-9]+}}, %r3, 4
; CHECK:       st %r{{[0-9]+}}, %r2, 4
; CHECK:       ld %r{{[0-9]+}}, %r3, 8
; CHECK:       st %r{{[0-9]+}}, %r2, 8
  call void @llvm.va_copy.p0(ptr %dst, ptr %src)
  ret void
}
