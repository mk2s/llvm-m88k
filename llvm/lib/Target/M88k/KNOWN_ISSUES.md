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
