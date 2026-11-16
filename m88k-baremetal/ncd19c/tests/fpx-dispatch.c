/* Dispatch test for the SFU1 vector patch (ncd19c_fpx.[Sc]), run in MAME in supervisor mode.
   Installs the handler, executes tb0 0,%r0,114 and tb0 0,%r0,115 (FPECR is 0, so the handler must
   return without changing anything), checks that every general register survived and that
   execution continued after the tb0, then uninstalls and checks both vector words. */
#include <stdint.h>
#include "../ncd19c_console.h"
#include "../ncd19c_fpx.h"
/* fpx_tb0_run(which, snap): all registers r1-r30 are loaded with 0xa5a50000+n, tb0 to vector
   114 (which = 0) or 115, then r1-r31 are stored into snap[1..31]. snap[0] = r31 before the tb0. */
void fpx_tb0_run(unsigned which, uint32_t *snap);
__asm__(
"	.text\n"
"	.globl	fpx_tb0_run\n"
"fpx_tb0_run:\n"
"	subu	%r31, %r31, 256\n"
"	st	%r1, %r31, 32\n"
"	st	%r14, %r31, 36\n"
"	st	%r15, %r31, 40\n"
"	st	%r16, %r31, 44\n"
"	st	%r17, %r31, 48\n"
"	st	%r18, %r31, 52\n"
"	st	%r19, %r31, 56\n"
"	st	%r20, %r31, 60\n"
"	st	%r21, %r31, 64\n"
"	st	%r22, %r31, 68\n"
"	st	%r23, %r31, 72\n"
"	st	%r24, %r31, 76\n"
"	st	%r25, %r31, 80\n"
"	st	%r26, %r31, 84\n"
"	st	%r27, %r31, 88\n"
"	st	%r28, %r31, 92\n"
"	st	%r29, %r31, 96\n"
"	st	%r30, %r31, 100\n"
"	st	%r3, %r31, 108\n"
"	bcnd	ne0, %r2, 2f\n"
"	or.u	%r1, %r0, 0xa5a5\n"
"	or	%r1, %r1, 1\n"
"	or.u	%r2, %r0, 0xa5a5\n"
"	or	%r2, %r2, 2\n"
"	or.u	%r3, %r0, 0xa5a5\n"
"	or	%r3, %r3, 3\n"
"	or.u	%r4, %r0, 0xa5a5\n"
"	or	%r4, %r4, 4\n"
"	or.u	%r5, %r0, 0xa5a5\n"
"	or	%r5, %r5, 5\n"
"	or.u	%r6, %r0, 0xa5a5\n"
"	or	%r6, %r6, 6\n"
"	or.u	%r7, %r0, 0xa5a5\n"
"	or	%r7, %r7, 7\n"
"	or.u	%r8, %r0, 0xa5a5\n"
"	or	%r8, %r8, 8\n"
"	or.u	%r9, %r0, 0xa5a5\n"
"	or	%r9, %r9, 9\n"
"	or.u	%r10, %r0, 0xa5a5\n"
"	or	%r10, %r10, 10\n"
"	or.u	%r11, %r0, 0xa5a5\n"
"	or	%r11, %r11, 11\n"
"	or.u	%r12, %r0, 0xa5a5\n"
"	or	%r12, %r12, 12\n"
"	or.u	%r13, %r0, 0xa5a5\n"
"	or	%r13, %r13, 13\n"
"	or.u	%r14, %r0, 0xa5a5\n"
"	or	%r14, %r14, 14\n"
"	or.u	%r15, %r0, 0xa5a5\n"
"	or	%r15, %r15, 15\n"
"	or.u	%r16, %r0, 0xa5a5\n"
"	or	%r16, %r16, 16\n"
"	or.u	%r17, %r0, 0xa5a5\n"
"	or	%r17, %r17, 17\n"
"	or.u	%r18, %r0, 0xa5a5\n"
"	or	%r18, %r18, 18\n"
"	or.u	%r19, %r0, 0xa5a5\n"
"	or	%r19, %r19, 19\n"
"	or.u	%r20, %r0, 0xa5a5\n"
"	or	%r20, %r20, 20\n"
"	or.u	%r21, %r0, 0xa5a5\n"
"	or	%r21, %r21, 21\n"
"	or.u	%r22, %r0, 0xa5a5\n"
"	or	%r22, %r22, 22\n"
"	or.u	%r23, %r0, 0xa5a5\n"
"	or	%r23, %r23, 23\n"
"	or.u	%r24, %r0, 0xa5a5\n"
"	or	%r24, %r24, 24\n"
"	or.u	%r25, %r0, 0xa5a5\n"
"	or	%r25, %r25, 25\n"
"	or.u	%r26, %r0, 0xa5a5\n"
"	or	%r26, %r26, 26\n"
"	or.u	%r27, %r0, 0xa5a5\n"
"	or	%r27, %r27, 27\n"
"	or.u	%r28, %r0, 0xa5a5\n"
"	or	%r28, %r28, 28\n"
"	or.u	%r29, %r0, 0xa5a5\n"
"	or	%r29, %r29, 29\n"
"	or.u	%r30, %r0, 0xa5a5\n"
"	or	%r30, %r30, 30\n"
"	st	%r31, %r31, 128\n"
"	tb0	0, %r0, 114\n"
"	br	3f\n"
"2:\n"
"	or.u	%r1, %r0, 0xa5a5\n"
"	or	%r1, %r1, 1\n"
"	or.u	%r2, %r0, 0xa5a5\n"
"	or	%r2, %r2, 2\n"
"	or.u	%r3, %r0, 0xa5a5\n"
"	or	%r3, %r3, 3\n"
"	or.u	%r4, %r0, 0xa5a5\n"
"	or	%r4, %r4, 4\n"
"	or.u	%r5, %r0, 0xa5a5\n"
"	or	%r5, %r5, 5\n"
"	or.u	%r6, %r0, 0xa5a5\n"
"	or	%r6, %r6, 6\n"
"	or.u	%r7, %r0, 0xa5a5\n"
"	or	%r7, %r7, 7\n"
"	or.u	%r8, %r0, 0xa5a5\n"
"	or	%r8, %r8, 8\n"
"	or.u	%r9, %r0, 0xa5a5\n"
"	or	%r9, %r9, 9\n"
"	or.u	%r10, %r0, 0xa5a5\n"
"	or	%r10, %r10, 10\n"
"	or.u	%r11, %r0, 0xa5a5\n"
"	or	%r11, %r11, 11\n"
"	or.u	%r12, %r0, 0xa5a5\n"
"	or	%r12, %r12, 12\n"
"	or.u	%r13, %r0, 0xa5a5\n"
"	or	%r13, %r13, 13\n"
"	or.u	%r14, %r0, 0xa5a5\n"
"	or	%r14, %r14, 14\n"
"	or.u	%r15, %r0, 0xa5a5\n"
"	or	%r15, %r15, 15\n"
"	or.u	%r16, %r0, 0xa5a5\n"
"	or	%r16, %r16, 16\n"
"	or.u	%r17, %r0, 0xa5a5\n"
"	or	%r17, %r17, 17\n"
"	or.u	%r18, %r0, 0xa5a5\n"
"	or	%r18, %r18, 18\n"
"	or.u	%r19, %r0, 0xa5a5\n"
"	or	%r19, %r19, 19\n"
"	or.u	%r20, %r0, 0xa5a5\n"
"	or	%r20, %r20, 20\n"
"	or.u	%r21, %r0, 0xa5a5\n"
"	or	%r21, %r21, 21\n"
"	or.u	%r22, %r0, 0xa5a5\n"
"	or	%r22, %r22, 22\n"
"	or.u	%r23, %r0, 0xa5a5\n"
"	or	%r23, %r23, 23\n"
"	or.u	%r24, %r0, 0xa5a5\n"
"	or	%r24, %r24, 24\n"
"	or.u	%r25, %r0, 0xa5a5\n"
"	or	%r25, %r25, 25\n"
"	or.u	%r26, %r0, 0xa5a5\n"
"	or	%r26, %r26, 26\n"
"	or.u	%r27, %r0, 0xa5a5\n"
"	or	%r27, %r27, 27\n"
"	or.u	%r28, %r0, 0xa5a5\n"
"	or	%r28, %r28, 28\n"
"	or.u	%r29, %r0, 0xa5a5\n"
"	or	%r29, %r29, 29\n"
"	or.u	%r30, %r0, 0xa5a5\n"
"	or	%r30, %r30, 30\n"
"	st	%r31, %r31, 128\n"
"	tb0	0, %r0, 115\n"
"3:\n"
"	st	%r1, %r31, 132\n"
"	st	%r2, %r31, 136\n"
"	st	%r3, %r31, 140\n"
"	st	%r4, %r31, 144\n"
"	st	%r5, %r31, 148\n"
"	st	%r6, %r31, 152\n"
"	st	%r7, %r31, 156\n"
"	st	%r8, %r31, 160\n"
"	st	%r9, %r31, 164\n"
"	st	%r10, %r31, 168\n"
"	st	%r11, %r31, 172\n"
"	st	%r12, %r31, 176\n"
"	st	%r13, %r31, 180\n"
"	st	%r14, %r31, 184\n"
"	st	%r15, %r31, 188\n"
"	st	%r16, %r31, 192\n"
"	st	%r17, %r31, 196\n"
"	st	%r18, %r31, 200\n"
"	st	%r19, %r31, 204\n"
"	st	%r20, %r31, 208\n"
"	st	%r21, %r31, 212\n"
"	st	%r22, %r31, 216\n"
"	st	%r23, %r31, 220\n"
"	st	%r24, %r31, 224\n"
"	st	%r25, %r31, 228\n"
"	st	%r26, %r31, 232\n"
"	st	%r27, %r31, 236\n"
"	st	%r28, %r31, 240\n"
"	st	%r29, %r31, 244\n"
"	st	%r30, %r31, 248\n"
"	st	%r31, %r31, 252\n"
"	ld	%r3, %r31, 108\n"
"	addu	%r4, %r31, 128\n"
"	or	%r5, %r0, 32\n"
"4:	ld	%r6, %r4, 0\n"
"	st	%r6, %r3, 0\n"
"	addu	%r4, %r4, 4\n"
"	addu	%r3, %r3, 4\n"
"	subu	%r5, %r5, 1\n"
"	bcnd	ne0, %r5, 4b\n"
"	ld	%r1, %r31, 32\n"
"	ld	%r14, %r31, 36\n"
"	ld	%r15, %r31, 40\n"
"	ld	%r16, %r31, 44\n"
"	ld	%r17, %r31, 48\n"
"	ld	%r18, %r31, 52\n"
"	ld	%r19, %r31, 56\n"
"	ld	%r20, %r31, 60\n"
"	ld	%r21, %r31, 64\n"
"	ld	%r22, %r31, 68\n"
"	ld	%r23, %r31, 72\n"
"	ld	%r24, %r31, 76\n"
"	ld	%r25, %r31, 80\n"
"	ld	%r26, %r31, 84\n"
"	ld	%r27, %r31, 88\n"
"	ld	%r28, %r31, 92\n"
"	ld	%r29, %r31, 96\n"
"	ld	%r30, %r31, 100\n"
"	addu	%r31, %r31, 256\n"
"	jmp	%r1\n"
);
static uint32_t snap[32];
static int fails;
static void fail(const char *what)
{
	ncd_console_puts("FAIL ");
	ncd_console_puts(what);
	ncd_console_putc('\n');
	++fails;
}
static int check_tb0(unsigned which)
{
	unsigned before = ncd_fpx_count[which];
	for (unsigned n = 0; n < 32; ++n)
		snap[n] = 0;
	fpx_tb0_run(which, snap);
	if (ncd_fpx_count[which] != before + 1) {
		fail("handler not called exactly once");
		return 0;
	}
	if (snap[0] != snap[31]) {
		fail("r31 changed");
		return 0;
	}
	for (unsigned n = 1; n < 31; ++n)
		if (snap[n] != 0xa5a50000u + n) {
			fail("register changed");
			ncd_console_puts("  r");
			ncd_console_putc('0' + n / 10);
			ncd_console_putc('0' + n % 10);
			ncd_console_putc('\n');
			return 0;
		}
	return 1;
}
int main(void)
{
	volatile uint32_t *slot;
	uint32_t orig[4];
	ncd_console_puts("fpx-dispatch start\n");
	/* crt0 installed the handler before main. Remove it to see the original words. */
	ncd_fpx_uninstall();
	slot = (volatile uint32_t *)(ncd_fpx_rd_vbr() + 114 * 8);
	for (unsigned i = 0; i < 4; ++i)
		orig[i] = slot[i];
	ncd_fpx_install();
	if (slot[0] != (0xc0000000u | (((uint32_t)ncd_fpx_stub114 - (uint32_t)&slot[0]) >> 2 & 0x03ffffffu)) ||
	    slot[2] != (0xc0000000u | (((uint32_t)ncd_fpx_stub115 - (uint32_t)&slot[2]) >> 2 & 0x03ffffffu)) ||
	    slot[1] != 0xf4005800u || slot[3] != 0xf4005800u || (slot[0] == orig[0] && slot[2] == orig[2]))
		fail("install did not write br to the stubs");
	else
		ncd_console_puts("install ok\n");
	if (check_tb0(0))
		ncd_console_puts("tb0 114 ok\n");
	if (check_tb0(1))
		ncd_console_puts("tb0 115 ok\n");
	/* once more, each, to see that the handler can be re-entered */
	if (check_tb0(0) && check_tb0(1))
		ncd_console_puts("tb0 repeat ok\n");
	ncd_fpx_uninstall();
	if (slot[0] != orig[0] || slot[1] != orig[1] || slot[2] != orig[2] || slot[3] != orig[3])
		fail("uninstall did not restore the vector words");
	else
		ncd_console_puts("uninstall ok\n");
	ncd_console_puts(fails ? "FPX DISPATCH FAILED\n" : "FPX DISPATCH DONE\n");
	return fails;
}
