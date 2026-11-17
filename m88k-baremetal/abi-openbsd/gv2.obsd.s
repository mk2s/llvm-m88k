	.file	"gv2.c"
	.text
	.align	2
	.align	4
	.globl	get_int
	.type	get_int, @function
get_int:
	subu	 %r31,%r31,16
	st	 %r1,%r31,4
	st	 %r30,%r0,%r31
	addu	 %r30,%r31,0
	ld	 %r2,%r30,16
	cmp	 %r9,%r2,7
	bb0.n	 ls,%r9,.L3
	addu	 %r8,%r2,1
	ld	 %r9,%r30,24
	st	 %r8,%r30,16
	br.n	 .L2
	lda	 %r2,%r9[%r2]
.L3:
	ld	 %r2,%r30,20
	addu	 %r9,%r2,4
	st	 %r9,%r30,20
.L2:
	addu	 %r31,%r30,0
	ld	 %r2,%r0,%r2
	ld	 %r30,%r0,%r31
	addu	 %r31,%r31,16
	jmp	 %r1

	.size	get_int, .-get_int
	.align	2
	.align	4
	.globl	get_dbl
	.type	get_dbl, @function
get_dbl:
	subu	 %r31,%r31,16
	st	 %r1,%r31,4
	st	 %r30,%r0,%r31
	addu	 %r30,%r31,0
	ld	 %r9,%r30,16
	addu	 %r9,%r9,1
	and	 %r2,%r9,0xfffe
	cmp	 %r9,%r2,6
	bb0.n	 ls,%r9,.L6
	addu	 %r8,%r2,2
	ld	 %r9,%r30,24
	st	 %r8,%r30,16
	br.n	 .L5
	lda	 %r2,%r9[%r2]
.L6:
	ld	 %r9,%r30,20
	addu	 %r9,%r9,7
	and	 %r2,%r9,0xfff8
	addu	 %r8,%r2,8
	st	 %r8,%r30,20
.L5:
	addu	 %r31,%r30,0
	ld.d	 %r2,%r0,%r2
	ld	 %r30,%r0,%r31
	addu	 %r31,%r31,16
	jmp	 %r1

	.size	get_dbl, .-get_dbl
	.align	2
	.align	4
	.globl	start1
	.type	start1, @function
start1:
	subu	 %r31,%r31,64
	st	 %r1,%r31,4
	st	 %r30,%r0,%r31
	addu	 %r30,%r31,0
	addu	 %r11,%r30,64
	subu	 %r12,%r0,28
	addu	 %r13,%r11,%r12
	st	 %r3,%r11,%r12
	addu	 %r31,%r30,0
	st.d	 %r8,%r13,20
	st.d	 %r4,%r13,4
	st.d	 %r6,%r13,12
	addu	 %r9,%r30,32
	ld	 %r2,%r9,4
	ld	 %r30,%r0,%r31
	addu	 %r31,%r31,64
	jmp	 %r1

	.size	start1, .-start1
	.align	2
	.align	4
	.globl	start0
	.type	start0, @function
start0:
	subu	 %r31,%r31,64
	st	 %r1,%r31,4
	st	 %r30,%r0,%r31
	addu	 %r30,%r31,0
	addu	 %r12,%r30,64
	subu	 %r13,%r0,24
	st.d	 %r4,%r12,%r13
	addu	 %r12,%r30,32
	ld	 %r2,%r12,8
	addu	 %r13,%r13,8
	addu	 %r12,%r30,64
	st.d	 %r6,%r12,%r13
	subu	 %r7,%r0,8
	st.d	 %r8,%r12,%r7
	addu	 %r31,%r30,0
	ld	 %r30,%r0,%r31
	addu	 %r31,%r31,64
	jmp	 %r1

	.size	start0, .-start0
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
