# Hand-written asm vs the clang/GCC stack-argument difference

Difference (README): GCC callees read stack arguments at incoming `r31+32`, clang at `r31+0`;
struct-by-value uses different registers. It matters only where asm calls C (or C calls asm)
with more than 8 word arguments, struct-by-value arguments, or varargs beyond r9.

Method: `grep -nE '\b(bsr|jsr)(\.n)?\b'` over `sys/arch/luna88k/luna88k/*.S`, `sys/arch/m88k/m88k/*.S`,
`sys/lib/libkern/arch/m88k/*.S`, then reading each call site's argument setup.

Result: no call site passes stack arguments.
* C functions called from asm and their arguments (all in r2-r4):
  `bzero`(locore.S:192), `vector_init`, `luna88k_bootstrap`, `main` (no args), `panic(fmt[, r4])` (locore.S:262, eh_common.S:2547, mutex.S:97; r4 is a register vararg),
  `error_fatal(frame)` (eh_common.S:712, 1711), `m88100_trap/m88110_trap(type, frame)` via `CALL`/`XCALL` (eh_common.S, m88100_fp.S:127, m88110_fp.S:87,98),
  `m88100_syscall/m88110_syscall(frame)`, `cache_flush(frame)`, `getipl()`, `setipl/raiseipl/spl0(ipl)`, `softintr_dispatch`, `proc_trampoline_mp`, `cpu_switch(NULL,NULL)`, `__savectx`.
* The `CALL(NAME, ARG1, ARG2)` macro (eh_common.S:230) only loads r2 and r3; `XCALL` loads nothing.
* Asm functions called from C (copy.S, fetch_store.S, mutex.S, process.S, libkern bcopy/bzero/memcpy/setjmp, cpu_switchto) take at most 3 register arguments and read none from the stack.
* The only `st rN, r31, 0/4/8/0xc` sequences before calls are frame-pointer/return-address saves in locally allocated frames (eh_common.S:703, 1349, 1702, 2103; m88100_fp.S:122-126; m88110_fp.S:84-96), not outgoing arguments.
* No struct-by-value argument crosses an asm boundary.

Conclusion: the all-clang kernel has no asm/C stack-argument interface; the ABI difference cannot
break the hand-written asm. Not exercised at run time: trap-return, syscall and context-switch paths
(boot ends at the `root device:` prompt).
