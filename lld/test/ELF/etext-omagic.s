# REQUIRES: m88k
# RUN: llvm-mc -filetype=obj -triple=m88k-unknown-netbsd %s -o %t.o
# RUN: ld.lld -N %t.o -o %t
# RUN: llvm-objdump -t --section-headers %t | FileCheck %s

## With -N there is a single writable PT_LOAD and no read-only segment.
## etext must still point after the last read-only allocated section
## (like GNU ld) instead of the start of the image.

# CHECK:      Sections:
# CHECK-NEXT:  Idx Name          Size     VMA      Type
# CHECK-NEXT:    0               00000000 00000000
# CHECK-NEXT:    1 .rodata       00000004 [[#%x,RO:]] DATA
# CHECK-NEXT:    2 .text         00000004 [[#%x,TEXT:]] TEXT
# CHECK-NEXT:    3 .data         00000004 [[#%x,DATA:]] DATA
# CHECK:      SYMBOL TABLE:
# CHECK:      {{0*}}[[#TEXT+4]] g {{ +}}.text{{.*}} etext

.globl _start
_start:
  or r2, r0, r0
.section .rodata,"a"
  .long 1
.data
  .long etext
