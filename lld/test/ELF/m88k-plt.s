# REQUIRES: m88k
## Non-PIE dynamic executable for OpenBSD/m88k: PLT/GOT layout expected by
## OpenBSD's ld.so (libexec/ld.so/m88k/rtld_machine.c), copy relocations and
## symbolic dynamic relocations. RELA is used. DISP26 (bsr) goes through the PLT.

# RUN: rm -rf %t && split-file %s %t && cd %t
# RUN: llvm-mc -filetype=obj -triple=m88k-unknown-openbsd shared.s -o shared.o
# RUN: ld.lld -shared -soname=libs.so shared.o -o libs.so
# RUN: llvm-mc -filetype=obj -triple=m88k-unknown-openbsd main.s -o main.o
## -nopie is accepted (OpenBSD's clang driver passes it).
# RUN: ld.lld -nopie main.o libs.so -o main
# RUN: llvm-readelf -S -d -r main | FileCheck %s --check-prefix=ELF
# RUN: llvm-objdump -d --no-show-raw-insn main | FileCheck %s --check-prefix=DIS
# RUN: llvm-objdump -s -j .got.plt main | FileCheck %s --check-prefix=GOT

## The PLT is writable: ld.so rewrites the or.u/ld immediates of PLT entries.
# ELF:      .rela.plt RELA
# ELF:      .plt      PROGBITS [[#%x,PLT:]] {{.*}} 000078 00 WAX
# ELF:      .got.plt  PROGBITS [[#%x,GOTPLT:]]
# ELF:      (PLTGOT)  0x[[#GOTPLT]]
# ELF-NEXT: (PLTREL)  RELA
# ELF-NEXT: 0x70000002 {{.*}} 0x[[#PLT]]
# ELF-NEXT: 0x70000003 {{.*}} 0x[[#PLT+0x78]]

# ELF:      Relocation section '.rela.dyn'
# ELF:      [[#%.8x,FPTR:]] {{.*}} R_88K_32 {{.*}} fn2 + 0
# ELF-NEXT: [[#%.8x,VAR:]] {{.*}} R_88K_COPY {{.*}} var + 0
## The addend of R_88K_GOTP_ENT is the address of the PLT entry.
# ELF:      Relocation section '.rela.plt'
# ELF:      [[#%.8x,GOTPLT+12]] {{.*}} R_88K_GOTP_ENT {{.*}} fn + [[#%x,PLT+72]]
# ELF-NEXT: [[#%.8x,GOTPLT+16]] {{.*}} R_88K_GOTP_ENT {{.*}} fn2 + [[#%x,PLT+96]]

# DIS:      <_start>:
# DIS-NEXT:   bsr [[#]]
# DIS-NEXT:   bsr [[#]]
# DIS-NEXT:   bsr 4
# DIS:      <.plt>:
# DIS-NEXT:   subu %r31, %r31, 16
# DIS-NEXT:   st %r1, %r31, 12
# DIS-NEXT:   st %r11, %r31, 8
# DIS-NEXT:   bsr 1
# DIS-NEXT:   or.u %r11, %r0, 0
# DIS-NEXT:   or %r11, %r11, 8444
# DIS-NEXT:   addu %r11, %r11, %r1
# DIS-NEXT:   ld %r1, %r11, 4
# DIS-NEXT:   st %r1, %r31, 4
# DIS-NEXT:   ld %r11, %r11, 8
# DIS-NEXT:   st %r0, %r31, 0
# DIS-NEXT:   jsr %r11
# DIS-COUNT-6: or %r0, %r0, %r0
## fn: load the .got.plt slot, jump; lazy path loads the .rela.plt offset.
# DIS-NEXT:   or.u %r11, %r0, 0
# DIS-NEXT:   ld %r11, %r11, 21336
# DIS-NEXT:   jmp %r11
# DIS-NEXT:   or.u %r11, %r0, 0
# DIS-NEXT:   or %r11, %r11, 0
# DIS-NEXT:   br -23
# DIS-NEXT:   or.u %r11, %r0, 0
# DIS-NEXT:   ld %r11, %r11, 21340
# DIS-NEXT:   jmp %r11
# DIS-NEXT:   or.u %r11, %r0, 0
# DIS-NEXT:   or %r11, %r11, 12
# DIS-NEXT:   br -29

## .got.plt: _DYNAMIC, two words for ld.so, then lazy slots pointing 12 bytes
## into each PLT entry.
# GOT:      534c 000042b8 00000000 00000000 00003294
# GOT-NEXT: 535c 000032ac

#--- shared.s
	.text
	.globl	fn
	.type	fn, @function
fn:
	jmp	%r1
	.globl	fn2
	.type	fn2, @function
fn2:
	jmp	%r1
	.data
	.globl	var
	.type	var, @object
	.size	var, 4
var:
	.long	42

#--- main.s
	.text
	.globl	_start
	.type	_start, @function
_start:
	bsr	fn
	bsr	fn2
	bsr	local
	or.u	%r2, %r0, %hi16(var)
	ld	%r2, %r2, %lo16(var)
	jmp	%r1
local:
	jmp	%r1
	.data
	.globl	fptr
fptr:
	.long	fn2
