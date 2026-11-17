/* debug-demo entry: NCD boot header, then the RSP stub (gdbstub), then main().
   The image is linked FIRST so this header is the image entry (like gdbstub/stub/test.s).
   The stub stops at its entry breakpoint and waits for the debugger on DUART channel B;
   on resume it continues at test_main. */
	.text
	.globl	start
	.globl	test_main
	or	%r0, %r0, %r0
	or	%r0, %r0, %r0
	br	start
	.2byte	0, 1			/* 1 = ignore CRC16 */
	.asciz	"Xncd19C"
	.4byte	0
start:
	br	stub_entry
test_main:
	/* r31 is still the ROM monitor's stack (data RAM); keep it 16-byte aligned */
	and	%r31, %r31, 0xfff0
	subu	%r31, %r31, 64
	bsr	main
1:	br	1b

	/* void ncd_rom_out1char(int c): ROM out1Char (0x0400b9ec, char in r2), as in ../crt0.S.
	   The ROM does not preserve all callee-saved registers, so save them around it. */
	.globl	ncd_rom_out1char
ncd_rom_out1char:
	subu	%r31, %r31, 96
	st.d	%r14, %r31, 32
	st.d	%r16, %r31, 40
	st.d	%r18, %r31, 48
	st.d	%r20, %r31, 56
	st.d	%r22, %r31, 64
	st.d	%r24, %r31, 72
	st	%r30, %r31, 80
	st	%r1, %r31, 84
	or.u	%r13, %r0, 0x0400
	or	%r13, %r13, 0xb9ec
	jsr	%r13
	ld.d	%r14, %r31, 32
	ld.d	%r16, %r31, 40
	ld.d	%r18, %r31, 48
	ld.d	%r20, %r31, 56
	ld.d	%r22, %r31, 64
	ld.d	%r24, %r31, 72
	ld	%r30, %r31, 80
	ld	%r1, %r31, 84
	addu	%r31, %r31, 96
	jmp	%r1
