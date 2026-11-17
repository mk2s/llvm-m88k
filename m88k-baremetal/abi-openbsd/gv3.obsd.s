	.file	"gv3.c"
	.text
	.align	2
	.align	4
	.globl	stk2
	.type	stk2, @function
stk2:
	subu	 %r31,%r31,16
	st	 %r1,%r31,4
	st	 %r30,%r0,%r31
	addu	 %r30,%r31,0
	ld	 %r2,%r30,16
	ld	 %r9,%r30,20
	addu	 %r31,%r30,0
	ld	 %r30,%r0,%r31
	addu	 %r2,%r2,%r9
	addu	 %r31,%r31,16
	jmp	 %r1

	.size	stk2, .-stk2
	.align	2
	.align	4
	.globl	callstk
	.type	callstk, @function
callstk:
	subu	 %r31,%r31,32
	st	 %r1,%r31,20
	st	 %r30,%r31,16
	addu	 %r30,%r31,16
	or	 %r13,%r0,9
	or	 %r12,%r0,10
	or	 %r3,%r0,2
	or	 %r4,%r0,3
	or	 %r5,%r0,4
	or	 %r6,%r0,5
	or	 %r7,%r0,6
	or	 %r8,%r0,7
	or	 %r9,%r0,8
	or	 %r2,%r0,1
	st	 %r13,%r0,%r31
	bsr.n	 stk
	st	 %r12,%r31,4
	subu	 %r31,%r30,16
	ld	 %r1,%r31,20
	ld	 %r30,%r31,16
	addu	 %r31,%r31,32
	jmp	 %r1

	.size	callstk, .-callstk
	.align	2
	.align	4
	.globl	ll
	.type	ll, @function
ll:
	subu	 %r31,%r31,16
	st	 %r1,%r31,4
	st	 %r30,%r0,%r31
	addu	 %r30,%r31,0
	addu	 %r31,%r30,0
	or	 %r9,%r0,%r6
	ext	 %r8,%r6,0<31>
	ld	 %r30,%r0,%r31
	addu.co	 %r5,%r5,%r9
	addu.ci	 %r4,%r4,%r8
	or	 %r2,%r0,%r4
	or	 %r3,%r0,%r5
	addu	 %r31,%r31,16
	jmp	 %r1

	.size	ll, .-ll
	.align	2
	.align	4
	.globl	llcall
	.type	llcall, @function
llcall:
	subu	 %r31,%r31,16
	st	 %r1,%r31,4
	st	 %r30,%r0,%r31
	addu	 %r30,%r31,0
	or	 %r4,%r0,0
	or	 %r5,%r0,2
	or	 %r6,%r0,3
	bsr.n	 ll
	or	 %r2,%r0,1
	addu	 %r31,%r30,0
	ld	 %r1,%r31,4
	ld	 %r30,%r0,%r31
	addu	 %r31,%r31,16
	jmp	 %r1

	.size	llcall, .-llcall
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
