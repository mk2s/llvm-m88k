	.file	"gv4.c"
	.text
	.align	2
	.align	4
	.globl	call
	.type	call, @function
call:
	subu	 %r31,%r31,32
	st	 %r1,%r31,20
	st	 %r30,%r31,16
	ld	 %r12,%r0,%r2
	ld	 %r13,%r2,4
	addu	 %r30,%r31,16
	st	 %r12,%r0,%r31
	ld	 %r12,%r2,8
	or	 %r2,%r0,%r3
	st	 %r13,%r31,4
	bsr.n	 use
	st	 %r12,%r31,8
	subu	 %r31,%r30,16
	ld	 %r1,%r31,20
	ld	 %r30,%r31,16
	addu	 %r31,%r31,32
	jmp	 %r1

	.size	call, .-call
	.align	2
	.align	4
	.globl	callee
	.type	callee, @function
callee:
	subu	 %r31,%r31,16
	st	 %r1,%r31,4
	st	 %r30,%r0,%r31
	addu	 %r30,%r31,0
	or	 %r9,%r0,%r2
	addu	 %r31,%r30,0
	ld	 %r2,%r30,20
	ld	 %r30,%r0,%r31
	addu	 %r2,%r2,%r9
	addu	 %r31,%r31,16
	jmp	 %r1

	.size	callee, .-callee
	.align	2
	.align	4
	.globl	sd
	.type	sd, @function
sd:
	subu	 %r31,%r31,16
	st	 %r1,%r31,4
	st	 %r30,%r0,%r31
	addu	 %r30,%r31,0
	trnc.sd	 %r9,%r2
	ld	 %r2,%r30,20
	addu	 %r31,%r30,0
	ld	 %r30,%r0,%r31
	addu	 %r2,%r2,%r4
	addu	 %r2,%r2,%r9
	addu	 %r31,%r31,16
	jmp	 %r1

	.size	sd, .-sd
	.align	2
	.align	4
	.globl	sdcall
	.type	sdcall, @function
sdcall:
	subu	 %r31,%r31,32
	st	 %r1,%r31,20
	st	 %r30,%r31,16
	ld	 %r12,%r2,4
	ld	 %r13,%r0,%r2
	addu	 %r30,%r31,16
	set	 %r2,%r0,1<30>
	or	 %r3,%r0,0
	or	 %r4,%r0,3
	st	 %r13,%r0,%r31
	bsr.n	 sd
	st	 %r12,%r31,4
	subu	 %r31,%r30,16
	ld	 %r1,%r31,20
	ld	 %r30,%r31,16
	addu	 %r31,%r31,32
	jmp	 %r1

	.size	sdcall, .-sdcall
	.align	2
	.align	4
	.globl	ret
	.type	ret, @function
ret:
	subu	 %r31,%r31,32
	st	 %r1,%r31,20
	st	 %r30,%r31,16
	st	 %r25,%r31,12
	addu	 %r30,%r31,16
	or	 %r3,%r0,%r2
	or	 %r4,%r0,12
	or	 %r2,%r0,%r12
	bsr.n	 memmove
	or	 %r25,%r0,%r12
	subu	 %r31,%r30,16
	or	 %r2,%r0,%r25
	ld	 %r1,%r31,20
	ld	 %r30,%r31,16
	ld	 %r25,%r31,12
	addu	 %r31,%r31,32
	jmp	 %r1

	.size	ret, .-ret
