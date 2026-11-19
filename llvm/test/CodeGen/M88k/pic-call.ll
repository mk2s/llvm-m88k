; Direct calls stay "bsr sym" (DISP26, routed through the PLT by the linker);
; they do not need the GOT base.
;
; RUN: llc < %s -mtriple=m88k-unknown-openbsd -relocation-model=pic -m88k-enable-delay-slot-filler=false -verify-machineinstrs | FileCheck %s
; RUN: llc < %s -mtriple=m88k-unknown-openbsd -relocation-model=pic -filetype=obj -o %t.o
; RUN: llvm-readelf -r %t.o | FileCheck --check-prefix=RELOC %s

declare i32 @f(i32)

; CHECK-LABEL: call:
; CHECK-NOT: abdiff
; CHECK-NOT: got_
; CHECK:     bsr f
define i32 @call(i32 %x) {
  %r = call i32 @f(i32 %x)
  ret i32 %r
}

; An indirect call through a pointer argument does not use the GOT either.
; CHECK-LABEL: icall:
; CHECK-NOT: abdiff
; CHECK:     jsr %r2
define void @icall(ptr %p) {
  call void %p()
  ret void
}

; RELOC: R_88K_DISP26 {{.*}} f + 0
; RELOC-NOT: R_88K_PLT_DISP26
