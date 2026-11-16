# RUN: not llvm-mc %s -triple=m88k-unknown-openbsd -mcpu=mc88100 2>&1 | FileCheck %s
# RUN: llvm-mc %s -triple=m88k-unknown-openbsd -mcpu=mc88100 -mattr=+gnu-asm | FileCheck --check-prefix=ON %s
# Without the gnu-asm feature (and on a non-NetBSD triple), bare register
# names are rejected; the %-prefixed syntax is unchanged.
# CHECK: error: Invalid operand for instruction
# ON: ldcr %r2, %cr1
	ldcr r2, cr1
