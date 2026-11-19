# RUN: llvm-mc -triple=m88k-unknown-none-elf -filetype=obj %s | llvm-objdump -s -j .text - | FileCheck %s

# Alignment padding in an executable section that starts at a non-word
# boundary is zero filled up to the word boundary, then nops.

	.text
	.byte	1
	.balign	8
	or	%r0, %r0, %r0
# CHECK: 0000 01000000 f4005800 f4005800
