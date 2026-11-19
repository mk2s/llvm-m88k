# RUN: llvm-mc -triple m88k-unknown-openbsd -filetype=obj %s -o %t.o
# RUN: llvm-objdump -dr %t.o | FileCheck %s
# RUN: llvm-readobj -r %t.o | FileCheck --check-prefix=READOBJ %s
# RUN: obj2yaml %t.o | FileCheck --check-prefix=YAML %s

# CHECK: ld %r2, %r25, 0
# CHECK-NEXT: R_88K_GOT_16L g
# CHECK: or.u %r3, %r0, 0
# CHECK-NEXT: R_88K_GOT_16H g
# CHECK: or.u %r25, %r0, 0
# CHECK-NEXT: R_88K_ABDIFF_16H
# CHECK: or %r25, %r25, 0
# CHECK-NEXT: R_88K_ABDIFF_16L

# READOBJ: R_88K_GOT_16L g 0x0
# READOBJ: R_88K_GOT_16H g 0x0
# READOBJ: R_88K_ABDIFF_16H
# READOBJ: R_88K_ABDIFF_16L

# YAML: Type: R_88K_GOT_16L
# YAML: Type: R_88K_GOT_16H
# YAML: Type: R_88K_ABDIFF_16H
# YAML: Type: R_88K_ABDIFF_16L

  .text
fn:
  ld %r2,%r25,%got_lo16(g)
  or.u %r3,%r0,%got_hi16(g)
  or.u %r25,%r0,%abdiff_hi16(.Lpc)
  or %r25,%r25,%abdiff_lo16(.Lpc)
.Lpc:
  addu %r25,%r25,%r1
