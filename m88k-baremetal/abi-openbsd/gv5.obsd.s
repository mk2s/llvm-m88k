	.file	"gv5.c"
	.text
	.align	2
	.align	4
	.globl	ov
	.type	ov, @function
ov:
	subu	 %r31,%r31,16
	st	 %r1,%r31,4
	st	 %r30,%r0,%r31
	addu	 %r30,%r31,0
	ld.d	 %r8,%r30,32
	addu	 %r31,%r30,0
	ld	 %r2,%r30,24
	trnc.sd	 %r7,%r8
	ld	 %r9,%r30,28
	ld	 %r8,%r30,40
	ld	 %r30,%r0,%r31
	addu	 %r2,%r2,%r9
	addu	 %r2,%r2,%r8
	addu	 %r2,%r2,%r7
	addu	 %r31,%r31,16
	jmp	 %r1

	.size	ov, .-ov
	.align	2
	.align	4
	.globl	ovcall
	.type	ovcall, @function
ovcall:
	subu	 %r31,%r31,48
	st	 %r1,%r31,36
	st	 %r30,%r31,32
	ld	 %r12,%r0,%r2
	ld	 %r13,%r2,4
	or	 %r11,%r0,9
	st	 %r12,%r0,%r31
	ld	 %r12,%r2,8
	addu	 %r30,%r31,32
	st	 %r13,%r31,4
	st	 %r12,%r31,8
	st	 %r11,%r31,12
	set	 %r12,%r0,10<20>
	or	 %r13,%r0,0
	or	 %r11,%r0,10
	or	 %r3,%r0,2
	or	 %r4,%r0,3
	or	 %r5,%r0,4
	or	 %r6,%r0,5
	or	 %r7,%r0,6
	or	 %r8,%r0,7
	or	 %r9,%r0,8
	or	 %r2,%r0,1
	st.d	 %r12,%r31,16
	bsr.n	 ov
	st	 %r11,%r31,24
	subu	 %r31,%r30,32
	ld	 %r1,%r31,36
	ld	 %r30,%r31,32
	addu	 %r31,%r31,48
	jmp	 %r1

	.size	ovcall, .-ovcall
	.align	2
	.align	4
	.globl	sum
	.type	sum, @function
sum:
	subu	 %r31,%r31,64
	st	 %r1,%r31,4
	st	 %r30,%r0,%r31
	addu	 %r30,%r31,0
	addu	 %r12,%r30,36
	subu	 %r13,%r0,28
	addu	 %r1,%r30,64
	subu	 %r2,%r2,1
	st	 %r3,%r1,%r13
	addu	 %r10,%r30,32
	st.d	 %r8,%r12,20
	addu	 %r11,%r2,1
	or	 %r9,%r0,1
	st.d	 %r4,%r12,4
	st.d	 %r6,%r12,12
	or	 %r13,%r0,0
	st	 %r9,%r30,8
	st	 %r1,%r30,12
	bcnd.n	 eq0,%r11,.L11
	st	 %r10,%r30,16
	or	 %r12,%r0,%r10
.L9:
	ld	 %r9,%r30,8
	cmp	 %r8,%r9,7
	addu	 %r6,%r9,1
	bb0.n	 ls,%r8,.L8
	lda	 %r7,%r12[%r9]
	st	 %r6,%r30,8
.L7:
	ld	 %r8,%r0,%r7
	subu	 %r2,%r2,1
	addu	 %r9,%r2,1
	bcnd.n	 ne0,%r9,.L9
	addu	 %r13,%r13,%r8
	br.n	 .L12
	addu	 %r31,%r30,0
.L8:
	ld	 %r7,%r30,12
	addu	 %r9,%r7,4
	br.n	 .L7
	st	 %r9,%r30,12
.L11:
	addu	 %r31,%r30,0
.L12:
	ld	 %r1,%r31,4
	ld	 %r30,%r0,%r31
	or	 %r2,%r0,%r13
	addu	 %r31,%r31,64
	jmp	 %r1

	.size	sum, .-sum
	.align	2
	.align	4
	.globl	sumcall
	.type	sumcall, @function
sumcall:
	subu	 %r31,%r31,32
	st	 %r1,%r31,20
	st	 %r30,%r31,16
	or	 %r13,%r0,8
	addu	 %r30,%r31,16
	st	 %r13,%r0,%r31
	or	 %r12,%r0,9
	or	 %r13,%r0,10
	or	 %r3,%r0,1
	or	 %r4,%r0,2
	or	 %r5,%r0,3
	or	 %r6,%r0,4
	or	 %r7,%r0,5
	or	 %r8,%r0,6
	or	 %r9,%r0,7
	or	 %r2,%r0,10
	st	 %r12,%r31,4
	bsr.n	 sum
	st	 %r13,%r31,8
	subu	 %r31,%r30,16
	ld	 %r1,%r31,20
	ld	 %r30,%r31,16
	addu	 %r31,%r31,32
	jmp	 %r1

	.size	sumcall, .-sumcall
