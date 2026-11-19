# REQUIRES: m88k
## Shared object for OpenBSD/m88k, shaped like libc.so: ET_DYN with OSABI 0,
## an RWE .plt LOAD, PT_OPENBSD_RANDOMIZE, PT_GNU_RELRO, DT_HASH, DT_BIND_NOW
## (the only tag OpenBSD's ld.so checks) and the 0x70000002/3 PLT range tags,
## BBASED_32 relocations sorted first and counted by DT_RELACOUNT.
# RUN: rm -rf %t && split-file %s %t && cd %t
# RUN: llvm-mc -filetype=obj -triple=m88k-unknown-openbsd lib.s -o lib.o
# RUN: ld.lld -shared -z now -soname=libfoo.so lib.o -o libfoo.so
# RUN: llvm-readelf -h -S -l -d -r libfoo.so | FileCheck %s
# RUN: llvm-mc -filetype=obj -triple=m88k-unknown-openbsd main.s -o main.o
# RUN: ld.lld -nopie --allow-shlib-undefined main.o libfoo.so -o main
# RUN: llvm-readelf -d -r main | FileCheck %s --check-prefix=EXE

# CHECK:      OS/ABI:                            UNIX - System V
# CHECK-NEXT: ABI Version:                       0
# CHECK:      Type:                              DYN
# CHECK:      Flags:                             0x0
# CHECK:      .hash
# CHECK:      .plt      PROGBITS [[#%x,PLT:]] {{.*}} 000060 00 WAX
# CHECK:      LOAD {{.*}} R E 0x1000
# CHECK:      LOAD {{.*}} RWE 0x1000
# CHECK:      LOAD {{.*}} RW  0x1000
# CHECK:      DYNAMIC
# CHECK:      GNU_RELRO
# CHECK:      OPENBSD_RANDOMIZE
# CHECK:      (SONAME)
# CHECK:      (BIND_NOW)
# CHECK:      (FLAGS_1)              NOW
# CHECK:      (RELASZ)               {{.*}}
# CHECK:      (RELAENT)              12 (bytes)
# CHECK-NEXT: (RELACOUNT)            3
# CHECK:      (PLTRELSZ)             12 (bytes)
# CHECK-NEXT: (PLTGOT)
# CHECK-NEXT: (PLTREL)               RELA
# CHECK-NEXT: 0x70000002 {{.*}} 0x[[#PLT]]
# CHECK-NEXT: 0x70000003 {{.*}} 0x[[#PLT+0x60]]
# CHECK:      (HASH)
# CHECK:      (INIT_ARRAY)
# CHECK-NOT:  (DEBUG)
# CHECK-NOT:  (FLAGS)
# CHECK-NOT:  TEXTREL
## BBASED_32 first, then symbolic relocations.
# CHECK:      Relocation section '.rela.dyn'
# CHECK:      R_88K_BBASED_32
# CHECK-NEXT: R_88K_BBASED_32
# CHECK-NEXT: R_88K_BBASED_32
# CHECK-NEXT: R_88K_32 {{.*}} extv + 0
# CHECK:      Relocation section '.rela.plt'
# CHECK-NEXT: Offset
# CHECK-NEXT: R_88K_GOTP_ENT {{.*}} extf + {{.*}}

# EXE:      (NEEDED)             Shared library: [libfoo.so]
# EXE:      (DEBUG)
# EXE-NOT:  (BIND_NOW)
# EXE:      R_88K_COPY {{.*}} counter

#--- lib.s
	.text
	.globl	foo
	.type	foo, @function
foo:
	or.u	%r25, %r0, %abdiff_hi16(.Lpc)
	bsr.n	.Lpc
	or	%r25, %r25, %abdiff_lo16(.Lpc)
.Lpc:
	addu	%r25, %r25, %r1
	ld	%r2, %r25, %got_lo16(loc)
	bsr	extf
	jmp	%r1
	.data
	.globl	ptr
ptr:
	.long	loc
	.long	extv
loc:
	.long	5
	.globl	counter
	.type	counter, @object
	.size	counter, 4
counter:
	.long	41
	.section .init_array,"aw",@init_array
	.long	loc
	.section .openbsd.randomdata,"aw",@progbits
	.long	0
#--- main.s
	.text
	.globl	_start
_start:
	bsr	foo
	or.u	%r2, %r0, %hi16(counter)
	ld	%r2, %r2, %lo16(counter)
	jmp	%r1
