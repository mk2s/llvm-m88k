/* SFU1 exception entry stubs (vectors 114 and 115) for the NCD 19c, MC88100.
   Manual section numbers are from the MC88100 User's Manual, 2nd ed. (1990).

   On entry (6.8.2): the exception is taken in supervisor mode with PSR.SFRZ (shadowing frozen),
   PSR.SFD1 (FPU disabled and frozen) and PSR.IND set. EPSR/SSBR/SXIP/SNIP/SFIP hold the context,
   r1-r31 hold the interrupted values, FPECR..FPIT hold the exception data (6.8.1).

   Steps:
   1. Stash r31 in SR3 and use r31 as the pointer to the static frame (nothing else is touched).
      Save r1-r3 and the shadow and FP control registers into the frame.
   2. Clear the scoreboard (SSBR) bits of the destination register(s) of the faulted FP
      instruction. The result is written by software, so the hardware will never clear them
      (6.8.4, 6.8.5, OpenBSD eh_common.S clear_FPp/FPi_ssbr_bit).
   3. Restart the FPU (6.4.3.4): rte to fpx_restart with EPSR having SFD1 and SFRZ cleared. The
      first instruction there is a trap-not-taken, which waits until all FP operations that were in
      flight have completed. This is needed because the C handler uses mul/div, which run on the FPU.
      Only r1-r3 were used before the rte and they are reloaded, so r1-r31 are still the
      interrupted values. SR2 carries the frame pointer across the rte.
   4. Save r2-r31 into the frame and call ncd_fpx_handle(frame, vector) on a private stack.
   5. Write back the control and general registers and rte. The faulted instruction is not
      executed again: rte resumes at SNIP/SFIP, which the hardware set to the following
      instruction(s) (6.4.3.1.2/3), also if the faulted instruction was in a delay slot.
   An exception while the FPU restart is in progress (busy flag set) is not handled: it ends in
   ncd_fpx_fatal. */
#include "ncd19c_fpx.h"
	.section .bss.ncd_fpx,"aw",@nobits
	.p2align 3
	.globl	ncd_fpx_frame_buf
ncd_fpx_frame_buf:
	.space	FPX_SIZE
	.p2align 3
ncd_fpx_stack:
	.space	4096
ncd_fpx_stack_end:
	.text
	.p2align 3
	.globl	ncd_fpx_stub114
ncd_fpx_stub114:
	stcr	%r31, %SR3
	or.u	%r31, %r0, %hi16(ncd_fpx_frame_buf)
	or	%r31, %r31, %lo16(ncd_fpx_frame_buf)
	st	%r1, %r31, FPX_R(1)
	br.n	fpx_common
	 or	%r1, %r0, NCD_FPX_VEC_PRECISE
	.p2align 3
	.globl	ncd_fpx_stub115
ncd_fpx_stub115:
	stcr	%r31, %SR3
	or.u	%r31, %r0, %hi16(ncd_fpx_frame_buf)
	or	%r31, %r31, %lo16(ncd_fpx_frame_buf)
	st	%r1, %r31, FPX_R(1)
	or	%r1, %r0, NCD_FPX_VEC_IMPRECISE
fpx_common:
	st	%r1, %r31, FPX_VECTOR
	ld	%r1, %r31, FPX_BUSY
	bcnd	ne0, %r1, fpx_nested
	st	%r2, %r31, FPX_R(2)
	st	%r3, %r31, FPX_R(3)
	ldcr	%r2, %EPSR
	ldcr	%r3, %SSBR
	st	%r2, %r31, FPX_EPSR
	st	%r3, %r31, FPX_SSBR
	ldcr	%r2, %SXIP
	ldcr	%r3, %SNIP
	st	%r2, %r31, FPX_SXIP
	st	%r3, %r31, FPX_SNIP
	ldcr	%r2, %SFIP
	fldcr	%r3, %fcr0
	st	%r2, %r31, FPX_SFIP
	st	%r3, %r31, FPX_FPECR
	fldcr	%r2, %fcr1
	fldcr	%r3, %fcr2
	st	%r2, %r31, FPX_FPHS1
	st	%r3, %r31, FPX_FPLS1
	fldcr	%r2, %fcr3
	fldcr	%r3, %fcr4
	st	%r2, %r31, FPX_FPHS2
	st	%r3, %r31, FPX_FPLS2
	fldcr	%r2, %fcr5
	fldcr	%r3, %fcr6
	st	%r2, %r31, FPX_FPPT
	st	%r3, %r31, FPX_FPRH
	fldcr	%r2, %fcr7
	fldcr	%r3, %fcr8
	st	%r2, %r31, FPX_FPRL
	st	%r3, %r31, FPX_FPIT
	fldcr	%r2, %fcr62
	fldcr	%r3, %fcr63
	st	%r2, %r31, FPX_FPSR
	st	%r3, %r31, FPX_FPCR
	/* Scoreboard: r2 = SSBR. A clr with a register operand takes width<<5 | offset. */
	ldcr	%r2, %SSBR
	ld	%r1, %r31, FPX_VECTOR
	cmp	%r3, %r1, NCD_FPX_VEC_PRECISE
	bb1	3, %r3, 10f			/* ne: imprecise */
	/* Precise: FIOV, FROP and FDVZ leave the destination scoreboard bits set (6.8.4). */
	ld	%r3, %r31, FPX_FPECR
	mask	%r3, %r3, 0x98			/* FIOV | FROP | FDVZ */
	bcnd	eq0, %r3, 30f			/* software tb0 or FPU disabled: nothing to clear */
	ld	%r1, %r31, FPX_FPPT
	extu	%r3, %r1, 5<0>			/* destination register number */
	bb1	5, %r1, 1f			/* TD = double: clear two bits */
	br.n	2f
	 set	%r3, %r3, 1<5>
1:	set	%r3, %r3, 1<6>
2:	br.n	30f
	 clr	%r2, %r2, %r3
	/* Imprecise: the second register of a double result keeps its bit set (6.8.5). */
10:	ld	%r3, %r31, FPX_FPECR
	mask	%r3, %r3, 0x07			/* FUNF | FOVF | FINX */
	bcnd	eq0, %r3, 30f
	ld	%r1, %r31, FPX_FPIT
	bb0	10, %r1, 30f			/* DESTSIZ = single */
	extu	%r3, %r1, 5<0>
	set	%r3, %r3, 1<6>
	clr	%r2, %r2, %r3
30:	stcr	%r2, %SSBR
	/* EPSR for the restart: the current PSR (supervisor, IND set) with SFD1 and SFRZ clear. */
	ldcr	%r2, %PSR
	clr	%r2, %r2, 1<3>
	clr	%r2, %r2, 1<0>
	stcr	%r2, %EPSR
	or.u	%r2, %r0, %hi16(fpx_restart + 2)	/* +2 = valid bit */
	or	%r2, %r2, %lo16(fpx_restart + 2)
	stcr	%r2, %SNIP
	addu	%r2, %r2, 4
	stcr	%r2, %SFIP
	or	%r2, %r0, 1
	st	%r2, %r31, FPX_BUSY
	stcr	%r31, %SR2			/* frame pointer for fpx_restart */
	ld	%r1, %r31, FPX_R(1)
	ld	%r2, %r31, FPX_R(2)
	ld	%r3, %r31, FPX_R(3)
	ldcr	%r31, %SR3
	rte
	.p2align 3
fpx_restart:
	tb1	0, %r0, 0			/* wait for FP operations in flight (6.4.3.4) */
	xcr	%r1, %r1, %SR2			/* r1 = frame, SR2 = interrupted r1 */
	st	%r0, %r1, FPX_BUSY
	st.d	%r2, %r1, FPX_R(2)
	st.d	%r4, %r1, FPX_R(4)
	st.d	%r6, %r1, FPX_R(6)
	st.d	%r8, %r1, FPX_R(8)
	st.d	%r10, %r1, FPX_R(10)
	st.d	%r12, %r1, FPX_R(12)
	st.d	%r14, %r1, FPX_R(14)
	st.d	%r16, %r1, FPX_R(16)
	st.d	%r18, %r1, FPX_R(18)
	st.d	%r20, %r1, FPX_R(20)
	st.d	%r22, %r1, FPX_R(22)
	st.d	%r24, %r1, FPX_R(24)
	st.d	%r26, %r1, FPX_R(26)
	st.d	%r28, %r1, FPX_R(28)
	st	%r30, %r1, FPX_R(30)
	st	%r31, %r1, FPX_R(31)
	ldcr	%r2, %SR2
	st	%r2, %r1, FPX_R(1)
	or	%r2, %r0, %r1			/* arg 1: frame */
	ld	%r3, %r2, FPX_VECTOR		/* arg 2: vector */
	or.u	%r31, %r0, %hi16(ncd_fpx_stack_end - 64)
	or	%r31, %r31, %lo16(ncd_fpx_stack_end - 64)
	bsr	ncd_fpx_handle
	/* Write back. All FP work is done; make sure nothing is in flight, then freeze shadowing. */
	tb1	0, %r0, 0
	or.u	%r2, %r0, %hi16(ncd_fpx_frame_buf)
	or	%r2, %r2, %lo16(ncd_fpx_frame_buf)
	ldcr	%r3, %PSR
	set	%r3, %r3, 1<1>			/* IND (already set) */
	set	%r3, %r3, 1<0>			/* SFRZ */
	stcr	%r3, %PSR
	ld	%r3, %r2, FPX_EPSR
	stcr	%r3, %EPSR
	ld	%r3, %r2, FPX_SNIP
	stcr	%r3, %SNIP
	ld	%r3, %r2, FPX_SFIP
	stcr	%r3, %SFIP
	stcr	%r0, %SSBR			/* the FPU is empty, so the scoreboard is clear */
	ld	%r3, %r2, FPX_FPSR
	fstcr	%r3, %fcr62
	ld	%r3, %r2, FPX_FPCR
	fstcr	%r3, %fcr63
	ld	%r3, %r2, FPX_FPECR
	fstcr	%r3, %fcr0
	ld.d	%r4, %r2, FPX_R(4)
	ld.d	%r6, %r2, FPX_R(6)
	ld.d	%r8, %r2, FPX_R(8)
	ld.d	%r10, %r2, FPX_R(10)
	ld.d	%r12, %r2, FPX_R(12)
	ld.d	%r14, %r2, FPX_R(14)
	ld.d	%r16, %r2, FPX_R(16)
	ld.d	%r18, %r2, FPX_R(18)
	ld.d	%r20, %r2, FPX_R(20)
	ld.d	%r22, %r2, FPX_R(22)
	ld.d	%r24, %r2, FPX_R(24)
	ld.d	%r26, %r2, FPX_R(26)
	ld.d	%r28, %r2, FPX_R(28)
	ld	%r30, %r2, FPX_R(30)
	ld	%r3, %r2, FPX_R(3)
	ld	%r1, %r2, FPX_R(1)
	ld	%r31, %r2, FPX_R(31)
	ld	%r2, %r2, FPX_R(2)
	rte
	/* Exception while the FPU restart was running. */
fpx_nested:
	or.u	%r31, %r0, %hi16(ncd_fpx_stack_end - 64)
	or	%r31, %r31, %lo16(ncd_fpx_stack_end - 64)
	bsr	ncd_fpx_fatal_nested
1:	br	1b

	/* uint32_t ncd_fpx_rd_vbr(void) */
	.globl	ncd_fpx_rd_vbr
ncd_fpx_rd_vbr:
	jmp.n	%r1
	 ldcr	%r2, %VBR
	/* uint32_t ncd_fpx_rd_psr(void) */
	.globl	ncd_fpx_rd_psr
ncd_fpx_rd_psr:
	jmp.n	%r1
	 ldcr	%r2, %PSR
	/* void ncd_fpx_wr_psr(uint32_t): an stcr that sets IND takes effect after the next
	   instruction (6.4.3.3), so the return is delayed by one nop. */
	.globl	ncd_fpx_wr_psr
ncd_fpx_wr_psr:
	stcr	%r2, %PSR
	or	%r0, %r0, %r0
	jmp	%r1
	/* void ncd_fpx_reset_psr(void): back to the monitor's PSR (supervisor, SFU1 on, interrupts on,
	   shadowing on) before leaving through _exit after a fatal error. */
	.globl	ncd_fpx_reset_psr
ncd_fpx_reset_psr:
	tb1	0, %r0, 0
	stcr	%r0, %SSBR
	or.u	%r2, %r0, 0x8000
	or	%r2, %r2, 0x03f0
	stcr	%r2, %PSR
	jmp	%r1
