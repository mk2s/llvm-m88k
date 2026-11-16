/* NCD 19c startup. Entered by the ROM launcher with jsr: r1 = monitor return address,
   r31 = monitor stack. PSR is left as the launcher set it. */
	.section .text.ncdhdr,"ax"
	.globl	_start
	.weak	__libc_init_array
	or	%r0, %r0, %r0
	or	%r0, %r0, %r0
	br	_start
	.2byte	0, 1			/* 1 = ignore CRC16 */
	.asciz	"Xncd19C"
	.4byte	0			/* CRC16 slot */

	.text
_start:
	/* save monitor r1/r31 */
	or.u	%r13, %r0, %hi16(__ncd_mon_r1)
	st	%r1, %r13, %lo16(__ncd_mon_r1)
	or.u	%r13, %r0, %hi16(__ncd_mon_r31)
	st	%r31, %r13, %lo16(__ncd_mon_r31)
	or.u	%r31, %r0, %hi16(__stack_top)
	or	%r31, %r31, %lo16(__stack_top)
	subu	%r31, %r31, 16
	/* zero .bss */
	or.u	%r12, %r0, %hi16(__bss_start)
	or	%r12, %r12, %lo16(__bss_start)
	or.u	%r11, %r0, %hi16(__bss_end)
	or	%r11, %r11, %lo16(__bss_end)
1:	cmp	%r10, %r12, %r11
	bb1	11, %r10, 2f
	st	%r0, %r12, 0
	br.n	1b
	addu	%r12, %r12, 4
2:	or.u	%r12, %r0, %hi16(__libc_init_array)
	or	%r12, %r12, %lo16(__libc_init_array)
	bcnd	eq0, %r12, 3f
	jsr	%r12
3:	or	%r2, %r0, 0
	or	%r3, %r0, 0
	bsr	main
	bsr	_exit

	.globl	_exit
_exit:
	/* signal end of run (EOT) on the DUART, then return to the monitor */
	or.u	%r13, %r0, 0x0100
1:	ld.bu	%r12, %r13, 0x24
	bb0	2, %r12, 1b
	or	%r12, %r0, 4
	st.b	%r12, %r13, 0x2c
	or.u	%r13, %r0, %hi16(__ncd_mon_r1)
	ld	%r1, %r13, %lo16(__ncd_mon_r1)
	or.u	%r13, %r0, %hi16(__ncd_mon_r31)
	ld	%r31, %r13, %lo16(__ncd_mon_r31)
	jmp	%r1

	/* First words of .data (see ncd19c.ld). The MAME loader checks __ncd_mon_r1 to confirm
	   that the jump to the image took effect. */
	.section .data.ncdmon,"aw"
	.p2align 2
__ncd_mon_r1:	.4byte 0
__ncd_mon_r31:	.4byte 0
