/* SFU1 (floating-point unit) exception handler for the MC88100 on the NCD 19c.
 *
 * References (MC88100 User's Manual, 2nd ed., 1990): FPECR/FPPT/FPIT/FPRH/FPRL layouts are
 * section 6.8.1, exception processing 6.8.2, precise exceptions 6.8.4 (vector 114, offset $390),
 * imprecise exceptions 6.8.5 (vector 115, offset $398), rte and FPU restart 6.4.3.2-6.4.3.4,
 * fcmp result string section 3 (fcmp). OpenBSD m88k m88100_fp.S / eh_common.S is the
 * cross-check (read, not copied).
 */
#ifndef NCD19C_FPX_H
#define NCD19C_FPX_H

/* Byte offsets into struct ncd_fpx_frame, shared with ncd19c_fpx.S. */
#define FPX_R(n)	((n) * 4)
#define FPX_EPSR	128
#define FPX_SSBR	132
#define FPX_SXIP	136
#define FPX_SNIP	140
#define FPX_SFIP	144
#define FPX_FPECR	148
#define FPX_FPHS1	152
#define FPX_FPLS1	156
#define FPX_FPHS2	160
#define FPX_FPLS2	164
#define FPX_FPPT	168
#define FPX_FPRH	172
#define FPX_FPRL	176
#define FPX_FPIT	180
#define FPX_FPSR	184
#define FPX_FPCR	188
#define FPX_VECTOR	192
#define FPX_BUSY	196
#define FPX_SIZE	200

#define NCD_FPX_VEC_PRECISE	114	/* SFU1 precise exception, VBR + $390 */
#define NCD_FPX_VEC_IMPRECISE	115	/* SFU1 imprecise exception, VBR + $398 */

/* FPECR bits (6.8.1.1). */
#define FPECR_FIOV	0x80
#define FPECR_FUNIMP	0x40
#define FPECR_FPRV	0x20
#define FPECR_FROP	0x10
#define FPECR_FDVZ	0x08
#define FPECR_FUNF	0x04
#define FPECR_FOVF	0x02
#define FPECR_FINX	0x01
/* FPSR accrued flags (6.8.1.2). */
#define FPSR_AFINV	0x10
#define FPSR_AFDVZ	0x08
#define FPSR_AFUNF	0x04
#define FPSR_AFOVF	0x02
#define FPSR_AFINX	0x01

#ifndef __ASSEMBLER__
#include <stdint.h>

/* Register snapshot taken by the entry stubs. r[] and the control registers are written back
   to the CPU when the handler returns. */
struct ncd_fpx_frame {
	uint32_t r[32];
	uint32_t epsr, ssbr, sxip, snip, sfip;
	uint32_t fpecr, fphs1, fpls1, fphs2, fpls2, fppt, fprh, fprl, fpit, fpsr, fpcr;
	uint32_t vector;
	uint32_t busy;
};

_Static_assert(__builtin_offsetof(struct ncd_fpx_frame, epsr) == FPX_EPSR, "epsr");
_Static_assert(__builtin_offsetof(struct ncd_fpx_frame, ssbr) == FPX_SSBR, "ssbr");
_Static_assert(__builtin_offsetof(struct ncd_fpx_frame, sxip) == FPX_SXIP, "sxip");
_Static_assert(__builtin_offsetof(struct ncd_fpx_frame, snip) == FPX_SNIP, "snip");
_Static_assert(__builtin_offsetof(struct ncd_fpx_frame, sfip) == FPX_SFIP, "sfip");
_Static_assert(__builtin_offsetof(struct ncd_fpx_frame, fpecr) == FPX_FPECR, "fpecr");
_Static_assert(__builtin_offsetof(struct ncd_fpx_frame, fphs1) == FPX_FPHS1, "fphs1");
_Static_assert(__builtin_offsetof(struct ncd_fpx_frame, fpls1) == FPX_FPLS1, "fpls1");
_Static_assert(__builtin_offsetof(struct ncd_fpx_frame, fphs2) == FPX_FPHS2, "fphs2");
_Static_assert(__builtin_offsetof(struct ncd_fpx_frame, fpls2) == FPX_FPLS2, "fpls2");
_Static_assert(__builtin_offsetof(struct ncd_fpx_frame, fppt) == FPX_FPPT, "fppt");
_Static_assert(__builtin_offsetof(struct ncd_fpx_frame, fprh) == FPX_FPRH, "fprh");
_Static_assert(__builtin_offsetof(struct ncd_fpx_frame, fprl) == FPX_FPRL, "fprl");
_Static_assert(__builtin_offsetof(struct ncd_fpx_frame, fpit) == FPX_FPIT, "fpit");
_Static_assert(__builtin_offsetof(struct ncd_fpx_frame, fpsr) == FPX_FPSR, "fpsr");
_Static_assert(__builtin_offsetof(struct ncd_fpx_frame, fpcr) == FPX_FPCR, "fpcr");
_Static_assert(__builtin_offsetof(struct ncd_fpx_frame, vector) == FPX_VECTOR, "vector");
_Static_assert(__builtin_offsetof(struct ncd_fpx_frame, busy) == FPX_BUSY, "busy");
_Static_assert(sizeof(struct ncd_fpx_frame) == FPX_SIZE, "size");

/* Patch the SFU1 vectors 114 and 115 (at VBR + 114*8, VBR + 115*8) with a br to the stubs.
   Interrupts are masked while the table is written. install/uninstall are idempotent. */
void ncd_fpx_install(void);
void ncd_fpx_uninstall(void);
/* C part of the handler. Called by the stubs with the frame and the vector number. */
void ncd_fpx_handle(struct ncd_fpx_frame *frame, unsigned vector);
/* Number of times ncd_fpx_handle ran for vector 114 ([0]) and 115 ([1]). */
extern volatile unsigned ncd_fpx_count[2];
/* Assembly helpers. */
uint32_t ncd_fpx_rd_vbr(void);
uint32_t ncd_fpx_rd_psr(void);
void ncd_fpx_wr_psr(uint32_t psr);
extern char ncd_fpx_stub114[], ncd_fpx_stub115[];
#endif
#endif
