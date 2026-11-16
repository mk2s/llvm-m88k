# RUN: llvm-mc %s -triple=m88k-unknown-netbsd -mcpu=mc88100 -show-encoding | FileCheck %s
# GNU syntax is enabled by default for NetBSD triples (no -mattr needed).
# CHECK: ldcr %r2, %cr1 {{.*}}| encoding: [0x80,0x40,0x40,0x20]
# CHECK: st %r0, %r2, 4 {{.*}}| encoding: [0x24,0x02,0x00,0x04]
# CHECK: bb1 6, %r9, foo
	ldcr r2, cr1
	st r0, r2, 4
	bb1 lt, r9, foo
foo:
