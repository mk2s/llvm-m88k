# RUN: llvm-mc %s -triple=m88k-unknown-netbsd -filetype=obj -o %t.o
# RUN: llvm-objdump -s -j .text -j .data %t.o | FileCheck %s
# RUN: llvm-readobj -r %t.o | FileCheck --check-prefix=RELOC %s
# GNU as syntax: undotted directives (word, text, data, align, space, string).
# The data below matches m88k--netbsdelf3-as (GNU as 2.16): "align N" counts
# bytes, "word" emits 4 bytes without alignment and "string" emits no NUL.
# The only difference is that alignment padding in .text is "or r0,r0,r0"
# here and zero in GNU as.
	text
	or r0, r0, r0
	align 8
	or.u r2, r0, hi16(tab)
	or r2, r2, lo16(tab)
	data
	align 4
tab:	word 1, 2, tab
	word tab+8
	string "ab\0"
	string "cd"
	align 4
	space 6
	space 2, 0xaa
	align 8
	word 0xdeadbeef
	text
	align 16
	or r0, r0, r0

# CHECK: Contents of section .text:
# CHECK-NEXT: 0000 f4005800 f4005800 5c400000 58420000
# CHECK-NEXT: 0010 f4005800
# CHECK: Contents of section .data:
# CHECK-NEXT: 0000 00000001 00000002 00000000 00000000
# CHECK-NEXT: 0010 61620063 64000000 00000000 0000aaaa
# CHECK-NEXT: 0020 deadbeef

# RELOC:      0xA R_88K_16H .data 0x0
# RELOC-NEXT: 0xE R_88K_16L .data 0x0
# RELOC:      0x8 R_88K_32 .data 0x0
# RELOC-NEXT: 0xC R_88K_32 .data 0x8
