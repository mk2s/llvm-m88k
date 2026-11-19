# RUN: llvm-mc -triple m88k-unknown-openbsd -filetype=obj %s -o %t.o
# RUN: llvm-readelf -r %t.o | FileCheck %s
# A GOT reference to a local (temporary) symbol keeps the symbol: the
# section symbol plus an offset would select the wrong GOT entry.
# CHECK: R_88K_GOT_16L {{.*}} .L.str1 + 0
# CHECK: R_88K_GOT_16L {{.*}} .L.str2 + 0
# CHECK: R_88K_16L {{.*}} .rodata.str1.1 + 4
  .text
fn:
  ld %r2,%r25,%got_lo16(.L.str1)
  ld %r3,%r25,%got_lo16(.L.str2)
  or %r4,%r4,%lo16(.L.str2)
  .section .rodata.str1.1,"aMS",@progbits,1
.L.str1:
  .asciz "abc"
.L.str2:
  .asciz "def"
