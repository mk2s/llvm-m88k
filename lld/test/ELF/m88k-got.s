# REQUIRES: m88k
## PIC relocations of OpenBSD/m88k: R_88K_ABDIFF_16H/16L (GETGOT prologue),
## R_88K_GOT_16L/16H (GOT slot offsets from the GOT base = .got.plt) and
## R_88K_PLT_DISP26.
# RUN: llvm-mc -filetype=obj -triple=m88k-unknown-openbsd %s -o %t.o
# RUN: ld.lld -shared -z now %t.o -o %t.so
# RUN: llvm-readelf -S -r -s %t.so | FileCheck %s --check-prefix=ELF
# RUN: llvm-objdump -d --no-show-raw-insn %t.so | FileCheck %s --check-prefix=DIS

## .got follows .got.plt directly (offsets from the GOT base are small).
# ELF:      .got.plt  PROGBITS [[#%x,GOTPLT:]] {{.*}} 000010
# ELF-NEXT: .got      PROGBITS [[#%.8x,GOTPLT+16]] {{.*}} 000008
## A GOT slot of a preemptible symbol: R_88K_32; of a local one: BBASED_32.
# ELF:      Relocation section '.rela.dyn'
# ELF:      [[#%.8x,GOTPLT+20]]  {{.*}} R_88K_BBASED_32
# ELF:      [[#%.8x,GOTPLT+16]]  {{.*}} R_88K_32 {{.*}} ext + 0
## The PLT slot of extf.
# ELF:      Relocation section '.rela.plt'
# ELF:      R_88K_GOTP_ENT {{.*}} extf +

# DIS:      <foo>:
## GOT - .Lpc fits in 16 bits (the or.u half is 0)
# DIS-NEXT:   or.u %r25, %r0, 0
# DIS-NEXT:   bsr.n 2
# DIS-NEXT:   or %r25, %r25, [[#%d,ABDIFF:]]
# DIS-NEXT:   addu %r25, %r25, %r1
## ext, loc: slots at .got.plt+16 and +20
# DIS-NEXT:   ld %r2, %r25, 16
# DIS-NEXT:   ld %r3, %r25, 20
# DIS-NEXT:   bsr [[#]]
# DIS-NEXT:   bsr [[#]]
# DIS-NEXT:   jmp %r1

	.text
	.globl	foo
	.type	foo, @function
foo:
	or.u	%r25, %r0, %abdiff_hi16(.Lpc)
	bsr.n	.Lpc
	or	%r25, %r25, %abdiff_lo16(.Lpc)
.Lpc:
	addu	%r25, %r25, %r1
	ld	%r2, %r25, %got_lo16(ext)
	ld	%r3, %r25, %got_lo16(loc)
	.reloc ., R_88K_PLT_DISP26, extf
	bsr	0
	bsr	extf
	jmp	%r1
	.size	foo, .-foo
	.data
loc:
	.long	5
