; Jump tables under PIC hold absolute block addresses in .data.rel.ro (they
; get dynamic relocations); the table address comes from the GOT.
;
; RUN: llc < %s -mtriple=m88k-unknown-openbsd -relocation-model=pic -m88k-enable-delay-slot-filler=false -verify-machineinstrs | FileCheck --check-prefix=PIC %s
; RUN: llc < %s -mtriple=m88k-unknown-openbsd -relocation-model=static -m88k-enable-delay-slot-filler=false -verify-machineinstrs | FileCheck --check-prefix=STATIC %s
; RUN: llc < %s -mtriple=m88k-unknown-openbsd -relocation-model=pic -filetype=obj -o %t.o
; RUN: llvm-readelf -S -r %t.o | FileCheck --check-prefix=OBJ %s

; PIC-LABEL: sw:
; PIC:      %abdiff_hi16(
; PIC:      ld %r{{[0-9]+}}, %r25, %got_lo16(.LJTI0_0)
; PIC-NOT:  %hi16(.LJTI0_0)
; PIC:      jmp %r{{[0-9]+}}
; PIC:      .section .data.rel.ro,"aw",@progbits
; PIC-NEXT: .align 2
; PIC-NEXT: .LJTI0_0:
; PIC-NEXT: .long .LBB0_
; STATIC-LABEL: sw:
; STATIC:   %hi16(.LJTI0_0)
; STATIC:   .section .rodata
; STATIC:   .LJTI0_0:
; STATIC-NOT: %got
; OBJ: .data.rel.ro {{.*}} WA
; OBJ: R_88K_GOT_16L {{.*}} .LJTI0_0 + 0
; OBJ: Relocation section '.rela.data.rel.ro'
; OBJ-COUNT-6: R_88K_32 {{.*}} .text
define i32 @sw(i32 %x) {
  switch i32 %x, label %d [i32 0, label %a i32 1, label %b i32 2, label %c i32 3, label %e i32 4, label %h i32 5, label %k]
a: ret i32 10
b: ret i32 21
c: ret i32 32
e: ret i32 43
h: ret i32 54
k: ret i32 65
d: ret i32 0
}
