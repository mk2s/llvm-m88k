; Global accesses under PIC go through the GOT. The GOT base is computed once
; per function that needs it (GCC prologue sequence, GOT base in %r25).
;
; RUN: llc < %s -mtriple=m88k-unknown-openbsd -relocation-model=pic -m88k-enable-delay-slot-filler=false -verify-machineinstrs | FileCheck --check-prefix=PIC %s
; RUN: llc < %s -mtriple=m88k-unknown-openbsd -relocation-model=static -m88k-enable-delay-slot-filler=false -verify-machineinstrs | FileCheck --check-prefix=STATIC %s
; RUN: llc < %s -mtriple=m88k-unknown-openbsd -relocation-model=pic -filetype=obj -o %t.o
; RUN: llvm-readelf -r %t.o | FileCheck --check-prefix=RELOC %s
; RUN: llvm-readelf -h %t.o | FileCheck --check-prefix=HDR %s

@ext = external global i32
@def = global i32 5, align 4

; PIC-LABEL: rd:
; PIC:      or.u %r25, %r0, %abdiff_hi16(.Ltmp[[#L:]])
; PIC-NEXT: bsr.n .Ltmp[[#L]]
; PIC-NEXT: or %r25, %r25, %abdiff_lo16(.Ltmp[[#L]])
; PIC-NEXT: .Ltmp[[#L]]:
; PIC-NEXT: addu %r25, %r25, %r1
; PIC:      ld %r{{[0-9]+}}, %r25, %got_lo16(ext)
; PIC:      ld %r{{[0-9]+}}, %r{{[0-9]+}}, 0
; PIC:      ld %r{{[0-9]+}}, %r25, %got_lo16(def)
; PIC-NOT:  %hi16
; PIC-NOT:  %lo16
; PIC-NOT:  abdiff
; STATIC-LABEL: rd:
; STATIC:   or.u %r{{[0-9]+}}, %r0, %hi16(ext)
; STATIC-NOT: %got
; STATIC-NOT: abdiff
define i32 @rd() {
  %v = load i32, ptr @ext
  %w = load i32, ptr @def
  %s = add i32 %v, %w
  ret i32 %s
}

; PIC-LABEL: wr:
; PIC:      bsr.n .Ltmp
; PIC:      ld %r{{[0-9]+}}, %r25, %got_lo16(def)
; PIC:      st %r2, %r{{[0-9]+}}, 0
define void @wr(i32 %x) {
  store i32 %x, ptr @def
  ret void
}

; A function without GOT use has no GOT base.
; PIC-LABEL: nogot:
; PIC-NOT:  abdiff
; PIC-NOT:  bsr
; PIC:      jmp %r1
define i32 @nogot(i32 %x) {
  %r = add i32 %x, 1
  ret i32 %r
}

; RELOC: R_88K_ABDIFF_16H
; RELOC: R_88K_ABDIFF_16L
; RELOC: R_88K_GOT_16L {{.*}} ext + 0
; RELOC: R_88K_GOT_16L {{.*}} def + 0
; RELOC-NOT: R_88K_16H
; RELOC-NOT: R_88K_16L
; HDR: OS/ABI: UNIX - System V
