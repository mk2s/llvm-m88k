	.file	"gv.c"
	.text
	.align	2
	.align	4
	.globl	get
	.type	get, @function
get:
	subu	 %r31,%r31,80
	st	 %r1,%r31,4
	st	 %r30,%r0,%r31
	addu	 %r30,%r31,0
	addu	 %r11,%r30,80
	subu	 %r13,%r0,28
	st	 %r3,%r11,%r13
	addu	 %r12,%r11,%r13
	or	 %r13,%r0,1
	st.d	 %r4,%r12,4
	st.d	 %r8,%r12,20
	st.d	 %r6,%r12,12
	addu	 %r2,%r30,24
	addu	 %r12,%r30,48
	st	 %r13,%r30,8
	or	 %r3,%r0,%r11
	addu	 %r13,%r30,92
	or	 %r4,%r0,12
	st	 %r12,%r30,16
	bsr.n	 memcpy
	st	 %r13,%r30,12
	addu	 %r31,%r30,0
	ld	 %r2,%r30,32
	ld	 %r1,%r31,4
	ld	 %r30,%r0,%r31
	addu	 %r31,%r31,80
	jmp	 %r1

	.size	get, .-get
	.align	2
	.align	4
	.globl	put
	.type	put, @function
put:
	subu	 %r31,%r31,32
	st	 %r1,%r31,20
	st	 %r30,%r31,16
	ld	 %r11,%r0,%r2
	ld	 %r12,%r2,4
	or	 %r13,%r0,%r2
	st	 %r11,%r0,%r31
	ld	 %r11,%r13,8
	addu	 %r30,%r31,16
	or	 %r2,%r0,1
	st	 %r12,%r31,4
	bsr.n	 get
	st	 %r11,%r31,8
	subu	 %r31,%r30,16
	ld	 %r1,%r31,20
	ld	 %r30,%r31,16
	addu	 %r31,%r31,32
	jmp	 %r1

	.size	put, .-put
	.align	2
	.align	4
	.globl	recv
	.type	recv, @function
recv:
	subu	 %r31,%r31,16
	st	 %r1,%r31,4
	st	 %r30,%r0,%r31
	addu	 %r30,%r31,0
	or	 %r9,%r0,%r2
	ld	 %r8,%r30,32
	ld	 %r2,%r30,24
	addu	 %r31,%r30,0
	ld	 %r30,%r0,%r31
	addu	 %r2,%r2,%r8
	addu	 %r2,%r2,%r9
	addu	 %r31,%r31,16
	jmp	 %r1

	.size	recv, .-recv
	.align	2
	.align	4
	.globl	send
	.type	send, @function
send:
	subu	 %r31,%r31,48
	st	 %r1,%r31,36
	st	 %r30,%r31,32
	ld	 %r10,%r0,%r2
	ld	 %r11,%r3,4
	ld	 %r12,%r0,%r3
	st	 %r10,%r0,%r31
	ld	 %r10,%r2,8
	ld	 %r13,%r2,4
	addu	 %r30,%r31,32
	or	 %r2,%r0,7
	st	 %r13,%r31,4
	st	 %r10,%r31,8
	st	 %r12,%r31,12
	bsr.n	 recv
	st	 %r11,%r31,16
	subu	 %r31,%r30,32
	ld	 %r1,%r31,36
	ld	 %r30,%r31,32
	addu	 %r31,%r31,48
	jmp	 %r1

	.size	send, .-send
	.align	2
	.align	4
	.globl	stk
	.type	stk, @function
stk:
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

	.size	stk, .-stk
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
