; Floating point constants are materialized with immediates (no constant pool),
; so they need neither the GOT nor a relocation under PIC.
;
; RUN: llc < %s -mtriple=m88k-unknown-openbsd -relocation-model=pic -m88k-enable-delay-slot-filler=false -verify-machineinstrs | FileCheck %s

; CHECK-LABEL: dc:
; CHECK-NOT: .LCPI
; CHECK-NOT: abdiff
; CHECK-NOT: got_
; CHECK:     or.u
define double @dc() {
  ret double 3.14159
}

; CHECK-LABEL: fc:
; CHECK-NOT: .LCPI
; CHECK-NOT: abdiff
define float @fc() {
  ret float 2.5
}
