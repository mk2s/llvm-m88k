# Bare-metal m88k: validation program (printf, malloc, qsort, setjmp, 64-bit divide)
`val.c` is built with the in-tree clang + ld.lld and picolibc (branch `m88k`), and runs in MAME `ncd19c`.
`val.expected` is the output of the same file compiled with the host compiler (`gcc -DHOST val.c`).
BSP (crt0, linker script, console, ELF->COFF, MAME runner) is in `../m88k-baremetal/ncd19c/` (see its README).

## Steps (all in `build.sh`; `./build.sh all` runs 1-5, or run one step by name)
1. `./build.sh clang`    configure (first time) and build clang, lld and tools in `build/`
   (target M88k is experimental: `-DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=M88k`).
2. `./build.sh rt`       build compiler-rt builtins (`build-rt-m88k`) and install them to clang's resource dir
   (`libclang_rt.builtins.a` for `m88k-unknown-none-elf`).
3. `./build.sh picolibc` meson/ninja build of `~/Documents/picolibc` (branch `m88k`), installed to `install-m88k`.
4. `./build.sh prog`     compile `val.c` with crt0/console/stdio glue and link with `ld.lld -T ncd19c.ld`,
   then `elf2ncd.py` writes `out/val.coff`.
5. `./build.sh run`      runs MAME headless-captured (serial channel B to a TCP listener), saves
   `out/val.png` and diffs the serial output against `val.expected`.
Environment: `LLVM`, `PICOLIBC`, `M88K_CPU` (default `mc88100`; 88110 is not supported yet), `JOBS`
(default 2), `CAP` (memory-cap wrapper; set `CAP=` to disable).

## Rules
MAME is always started with `-window` (hard-coded in the runner). Console output is mirrored to the
screen unconditionally because the real unit has no serial-console option. Only the MAME process the
runner started is killed. No threads.

## Floating point
mc88100 FP instructions (fadd/fmul/fdiv/flt/trnc/fcmp) are implemented in the MAME model (softfloat),
and `val.c` executes them (hard-float code, `fmul.ddd`, `fdiv.ddd`, ...). MAME does not model FP
exceptions or reserved operands (denormals, NaN, infinity), so it is more permissive than real
silicon. Edge cases on a real unit would need an SFU1 exception handler.

## Real hardware
Back up `~/Documents/mame/tftpboot/0A000207`, then `cp ../m88k-baremetal/ncd19c/out/val.coff` over it,
power-cycle, and watch the screen (and AUX port, 9600 8N1).
