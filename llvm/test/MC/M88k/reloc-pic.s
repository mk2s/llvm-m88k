# RUN: llvm-mc -triple m88k-unknown-openbsd -filetype=obj %s -o %t.o
# RUN: llvm-readelf -h -r %t.o | FileCheck --check-prefix=RELOC %s
# RUN: llvm-mc -triple m88k-unknown-openbsd -show-encoding %s \
# RUN:   | FileCheck --check-prefix=ENC %s
# RUN: llvm-mc -triple m88k-unknown-openbsd %s | FileCheck --check-prefix=ASM %s

# OpenBSD m88k objects use ELFOSABI_NONE.
# RELOC: OS/ABI: UNIX - System V

# RELOC: Relocation section '.rela.text'
# RELOC: R_88K_GOT_16L {{.*}} g + 0
# RELOC: R_88K_GOT_16H {{.*}} g + 0
# RELOC: R_88K_GOT_16L {{.*}} g + 0
# RELOC: R_88K_ABDIFF_16H
# RELOC: R_88K_ABDIFF_16L
# RELOC: R_88K_PLT_DISP26 {{.*}} f + 0
# RELOC-NOT: R_88K_16

  .text
  .globl fn
fn:
# ASM: ld %r2, %r25, %got_lo16(g)
# ENC: fixup A - offset: 2, value: %got_lo16(g), kind: FK_88K_GOT_LO
  ld %r2,%r25,%got_lo16(g)
# ASM: or.u %r3, %r0, %got_hi16(g)
# ENC: fixup A - offset: 2, value: %got_hi16(g), kind: FK_88K_GOT_HI
  or.u %r3,%r0,%got_hi16(g)
# ASM: or %r3, %r3, %got_lo16(g)
  or %r3,%r3,%got_lo16(g)
# ASM: or.u %r25, %r0, %abdiff_hi16(.Lpc)
# ENC: fixup A - offset: 2, value: %abdiff_hi16(.Lpc), kind: FK_88K_ABDIFF_HI
  or.u %r25,%r0,%abdiff_hi16(.Lpc)
  bsr.n .Lpc
# ASM: or %r25, %r25, %abdiff_lo16(.Lpc)
# ENC: fixup A - offset: 2, value: %abdiff_lo16(.Lpc), kind: FK_88K_ABDIFF_LO
  or %r25,%r25,%abdiff_lo16(.Lpc)
.Lpc:
  addu %r25,%r25,%r1
  .reloc ., R_88K_PLT_DISP26, f
  bsr 0
