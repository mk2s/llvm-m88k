# M88k known issues

## Argument passing differs from OpenBSD's GCC 3.3.6 (r9 back-fill, va_list)


Status: known, not fixed. Reference output: `m88k-baremetal/abi-openbsd/` (OpenBSD 7.1 luna88k `cc`
run under run88k). Most fixed-argument calls match OpenBSD: struct arguments are copied to the
stack at `r31+0` without using register words, stack arguments start at `r31+0`, struct results use
a hidden pointer in `r12`. The exception is the `r9` back-fill below; the rest is `va_list` handling.


### Fixed arguments: a 64-bit argument that does not fit leaves r9 unused

When a 64-bit argument (`double`, `long long`) no longer fits in a register pair (only `r9` is left),
OpenBSD GCC passes it on the stack and gives the next 32-bit argument `r9`. Clang
(`CC_M88k_Custom_64` in `M88kCallingConv.cpp`, since 04c925b708d2) marks `r9` used, so that
argument goes on the stack too. Example: `f(double, int, double, int, double e, int g)`: GCC passes
`g` in `r9`, Clang at `r31+8`. Mixed GCC/Clang calls with such a signature get wrong arguments
(`m88k-baremetal/openbsd-userland/tests/kinterop_r9`). Clang skips `r9` on purpose because its
`va_arg` uses one argument word index for registers and stack; changing the calling convention
alone would break Clang's own variadic functions, so this must be fixed together with `va_arg` below.

### va_list

The `va_list` layout is the same (`{ int __va_arg; int *__va_stk; int *__va_reg; }`), but the fields mean different things:

| | OpenBSD GCC | Clang (`M88kABIInfo::EmitVAArg`, G_VASTART lowering) |
|---|---|---|
| `__va_stk` after `va_start` | first stack argument (incoming `r31+0`, after named stack arguments) | incoming SP - 32 |
| `va_arg` of a scalar, index >= 8 | `*__va_stk`, then advance `__va_stk` | `__va_stk + 4 * idx`, `__va_arg` always advanced |
| `va_arg` of a struct | copy inline from `__va_stk`, advance by its size | read a pointer from a register word, dereference it |
| double that does not fit in the registers | goes to the stack; the next int may still use `r9` | index advances past the double; the next int is read from the stack |

Effects:
- `va_arg(ap, struct S)` reads garbage (the caller passes the struct inline on the stack, the callee
  reads a pointer).
- A `va_list` created by one compiler and consumed by the other (for example Clang code calling an
  OpenBSD-built `vfprintf`) returns wrong values once more than 7 variadic words are used.
- Code built only with Clang, with no struct passed through `...`, is not affected.

Fix outline: let `CC_M88k_Custom_64` leave `r9` free; in `clang/lib/CodeGen/Targets/M88k.cpp`, track the register index and the stack pointer
separately as OpenBSD does and read aggregates inline from `__va_stk`; in `M88kLegalizerInfo.cpp`
(G_VASTART) set `__va_stk` to the first stack argument. Add lit tests derived from
`m88k-baremetal/abi-openbsd/gv*.obsd.s`. Runtime check: `m88k-baremetal/openbsd-userland/run.sh`
(tests k07, k08 and kinterop_r9 must then pass).

Note: the NetBSD cross compiler (`m88k--netbsdelf3-gcc`) uses an older ABI (stack arguments at
`r31+32`, structs consume argument words). That is the "differs from GCC" remark in
`.agents/tasks/clang-netbsd-kernel/README.md`; it is not the OpenBSD ABI.

## Atomics are not implemented

Status: known, not fixed. Every atomic operation fails in the backend: `atomicrmw` and `cmpxchg`
("unable to legalize G_ATOMICRMW_* / G_ATOMIC_CMPXCHG_WITH_SUCCESS"), and atomic loads and stores
("cannot select: G_LOAD ... seq_cst"). From C: `__atomic_fetch_add`, `__atomic_load_n`,
`__atomic_store_n`, `__atomic_exchange_n`, `__atomic_compare_exchange_n` and the C11 equivalents.
Repro: `int f(int *p, int v) { return __atomic_fetch_add(p, v, __ATOMIC_SEQ_CST); }` with
`clang --target=m88k-openbsd -O2 -c`.

The MC88100 only has `xmem` (atomic swap). OpenBSD's libc has no `__atomic_*` or `__sync_*`
functions, and the compiler-rt builtins are built without `atomic.c`. Mixing native operations
(`xmem` for exchange, plain `ld`/`st` for loads and stores) with a lock-based `cmpxchg` is not
correct: a native store does not take the lock, so it can be lost between the locked read and write.

Options:
- A. Every atomic becomes an `__atomic_*` call (`setMaxAtomicSizeInBitsSupported(0)`), with
  compiler-rt's `atomic.c` built into the builtins and given an `xmem` spinlock (address-hashed lock
  table, implemented with inline `xmem`, not with C11 atomics). Correct on multiprocessors; every
  atomic load/store is a call.
- B. Native `xmem` exchange and native naturally aligned loads/stores up to 32 bits; `cmpxchg` and
  the other read-modify-write operations stay unsupported.

Related: Clang reports `__GCC_ATOMIC_INT_LOCK_FREE`, `__GCC_ATOMIC_LLONG_LOCK_FREE` and
`__GCC_ATOMIC_POINTER_LOCK_FREE` (and the `__CLANG_` forms) as 1 ("sometimes lock-free"), although
no atomic is lowered and the MC88100 has no 64-bit atomic at all. These must be set to match the
chosen option (0 for A).

## Inline asm "rm" uses the memory alternative

GlobalISel resolves an `"rm"` input to the memory alternative, so the value is stored to a stack slot
and the asm gets its address (`%r0, %rN`). This is correct but slower than GCC, which picks the
register. Test: `llvm/test/CodeGen/M88k/inline-asm-mem.ll`.

## lld: OpenBSD dynamic linking limits

- `-lc` does not find `libc.so.96.1`: lld only searches `lib<name>.so` and `lib<name>.a`, while
  OpenBSD's linkers also pick the highest `lib<name>.so.<major>.<minor>`. lld resolves `-l` before it
  knows the target OS, and searching versioned names on every OS would change Linux behaviour.
  Without an unversioned `libc.so` in a `-L` directory lld silently links `libc.a` (static).
  Workaround: `m88k-baremetal/openbsd-userland/run.sh` creates `libc.so`/`libm.so` links.
- PIC (`-relocation-model=pic`, `-fPIC`) is GOT based: each function that needs the GOT computes its
  base in a register (hinted to `%r25`) with GCC's `or.u/bsr.n/or/addu %r1` prologue
  (`R_88K_ABDIFF_16H/16L`), globals and function addresses load from `%got_lo16(sym)`
  (`R_88K_GOT_16L`, GOT offsets above 64 KiB are not supported), jump tables are absolute and live in
  `.data.rel.ro`. The local `bsr.n` of the prologue still carries a `R_88K_DISP26` against `.text`.
  Not PIC safe yet: indirect branch relaxation (`insertIndirectBranch`) and `LOAD_STACK_GUARD` use
  absolute `%hi16/%lo16` addresses. lld support for shared objects and these relocations is separate.
- Calls to an undefined weak symbol (e.g. `_Jv_RegisterClasses` from OpenBSD's `crtbegin.o`) get an
  unused PLT entry and `.rela.plt` relocation. GNU ld omits them; ld.so skips the missing symbol, so
  this is harmless (lld does the same on other targets).

## Clang driver: compiler-rt on OpenBSD

The OpenBSD driver links `-lcompiler_rt`. The OpenBSD/m88k sysroot has no `libcompiler_rt.a`
(OpenBSD uses GCC and `libgcc` on m88k), so it has to be provided, built for
`m88k-unknown-openbsd` (`m88k-baremetal/openbsd-userland/build-rt.sh`). The bare-metal library
(`m88k-unknown-none-elf`) must not be used for OpenBSD programs: it allocates `r26`/`r27`, and `r27`
is OpenBSD's thread pointer (in a dynamic executable libc.so calls the executable's `__udivdi3` etc.).

## Not yet verified

- The generic GlobalISel changes (big-endian split loads/stores in `LegalizerHelper.cpp`, a82a547bf161;
  non-indirect memory inputs in `InlineAsmLowering.cpp`, 0477306fb630) were tested on M88k and X86
  only. AArch64, RISCV and MIPS (not built here) still need a test run.
- The NetBSD luna88k kernel (`.agents/tasks/clang-netbsd-kernel`) has not been rebuilt and booted since
  the FP pattern fix (9cdb854bf23e), the r26/r27 change (OpenBSD only), the GlobalISel argument and
  legalizer fixes and the lld changes (RELA, DISP26 via PLT).
