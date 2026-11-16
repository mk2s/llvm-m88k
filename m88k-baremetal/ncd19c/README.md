# NCD 19c bare-metal BSP (MAME ncd19c)

Files: `crt0.S` (NCD header + entry, bss clear, stack), `ncd19c.ld` (linker script),
`ncd19c_console.[ch]` (serial + screen console), `elf2ncd.py` (ELF -> NCD COFF),
`build.sh` (one test), `toolchain.sh` (clang/compiler-rt/picolibc + validation), `run-ncd19c.py` (MAME runner), `ncd19c-load.lua` (MAME autoboot script), `tests/`.

## Build
    ./build.sh boot-hello        # tests/boot-hello.c -> out/boot-hello.elf, out/boot-hello.coff
    ./build.sh libc-hello        # tests named libc-* / val* link picolibc + compiler-rt
picolibc: branch `m88k` of /home/mk2s/Documents/picolibc, installed to `install-m88k`
(`PICOLIBC` overrides). `ncd19c_stdio.c` gives tinystdio stdin/stdout/stderr on the console
(output: serial + screen, input: serial). The heap is picolibc's sbrk between `__heap_start`
and `__heap_end` from `ncd19c.ld`. Rebuild picolibc:

    cd /home/mk2s/Documents/picolibc
    meson setup build-m88k --cross-file scripts/cross-clang-m88k-unknown-none-elf.txt \
      -Dmultilib=false -Dpicocrt=false -Dsemihost=false -Dtests=false -Dsingle-thread=true \
      -Dthread-local-storage=false -Datomic-ungetc=false -Dio-long-long=true \
      -Dnewlib-global-errno=true -Dspecsdir=none --buildtype=minsize --prefix=$PWD/install-m88k
    ninja -C build-m88k -j2 && ninja -C build-m88k install

The CPU is set by `-mcpu=` in the cross file (mc88100; mc88110 later).
Env: `LLVM` (default /home/mk2s/Documents/llvm-m88k, uses build/bin/clang and ld.lld),
`M88K_CPU` (default mc88100). Target: `--target=m88k-unknown-none-elf`.

## Run in MAME (headless output capture)
    ./run-ncd19c.py out/boot-hello.coff --mode inject --expect "boot ok" --shot out/shot.png
    ./run-ncd19c.py out/boot-hello.coff --mode netboot
The runner opens a TCP listener itself (default 127.0.0.1:2340, `--port`) and starts MAME
(/home/mk2s/Documents/mame/mame ncd19c) with serial channel B connected to it via
`-serial1 null_modem -bitb socket.127.0.0.1:PORT`. Program serial output is printed to stdout.
Choose a port other than 2159/2160/2260/2323/2324.

- MAME is always started with `-window` (never fullscreen); it still needs a display.
- Exit code 0 only if EOT was seen (and `--expect` text found). The program ends output with
  byte 0x04 (EOT); the runner then lets the Lua script take the screenshot (`--shot`) and exit.
- Only the MAME process started by the runner is ever killed.
- `inject` (default): the Lua script waits `--wait` emulated seconds for the ROM monitor, writes
  the image into emulated RAM and jumps to it. Uses `-networkprovider none`; no root needed.
- `netboot`: copies the image over `~/Documents/mame/tftpboot/0A000205` and lets the firmware
  boot it via TFTP. A tftpd must already be running (no sudo is used). The original file is
  backed up to `0A000205.baremetal.bak` and restored afterwards with a sha256 check; the runner
  refuses if another mame is running or a differing backup already exists.
- A private copy of cfg/ncd19c.cfg is used so MAME's own config is not rewritten.

## Console
Output goes to the SCC2681 channel B (AUX, 0x0100002C) and, unconditionally by default, to the
screen through the ROM `out1Char` routine (`ncd_console_mask`), since the real unit has no serial
console option.

## Memory map (ncd19c.ld)
- 0x01000000 DUART (SR_B +0x24, THR_B +0x2C)
- .text/.rodata at 0x04020000 (code RAM; ROM at 0x04000000)
- .data at 0x08010000, .bss after it (NOLOAD); stack at top of RAM, `__ram_top` default 0x08200000,
  `__stack_size` 0x20000. Override with `--defsym`. Real unit has 2 MB data RAM at 0x08000000;
  MAME ncd19c has 8 MB data and 4 MB code.
- The boot ROM accepts only .text, .data, .bss sections.
## SFU1 floating-point exception handler (optional, `--fpx`)
A real MC88100 does not finish FP instructions with reserved operands (denormal, NaN, infinity),
division by zero or integer-conversion overflow. It takes a precise SFU1 exception (vector 114,
VBR+$390). Result underflow/overflow/inexact take an imprecise one (vector 115, VBR+$398).
The ROM's own handler is not known to emulate these.
`ncd19c_fpx.{S,c,h}` patch the two vector slots (VBR = 0x04004000, read with `ldcr`) with a `br`
to stubs that save r1-r31 and the FP control registers, restart the FPU (rte to a trap-not-taken,
manual 6.4.3.4), call `ncd_fpx_handle`, write everything back and `rte`. The C part completes
fadd/fsub/fmul/fdiv/fcmp/flt/int/nint/trnc with compiler-rt soft-float and applies the IEEE
default action for underflow, overflow and inexact. `crt0.S` calls `ncd_fpx_install` before `main`
and `ncd_fpx_uninstall` in `_exit` through weak references, so images without the handler are
unchanged (the original vector words are restored on exit).
    ./build.sh --fpx val.c ...   # or any test named fpx-*; FPX=1 ./toolchain.sh prog
Limits: user exception handlers (FPCR enable bits) are not dispatched, only the default action is
taken. Emulated precise arithmetic rounds to nearest even. The inexact flag is not tracked for
emulated precise operations. An exception during the FPU restart ends in `ncd_fpx_fatal` (message
and return to the monitor). Writing the vector words does not flush any code cache, which is
unverified on the real unit.
Tests (all in `tests/`):
- `fpx-unit.c`: calls `ncd_fpx_handle` with synthetic frames from the manual's encodings and prints
  the destination registers, FPSR and FPECR. `fpx-unit.expected` comes from the host (`cc -DHOST`).
  Run: `./run-ncd19c.py out/fpx-unit.coff` and compare with the expected file.
- `fpx-dispatch.c`: installs the handler, executes `tb0 0,%r0,114` and `tb0 0,%r0,115` in
  supervisor mode, checks r1-r31 and the call counts, uninstalls and checks the vector words.
  Output must equal `fpx-dispatch.expected`.
- `fpx-real.c`: **not verified in MAME.** For the real unit: build it, then
  `cp out/fpx-real.coff ~/Documents/mame/tftpboot/0A000207` (back up the old file first), power-cycle
  and compare screen/AUX output with `fpx-real.expected`. MAME never raises these exceptions and
  also gets NaN compares wrong (lines `inf-inf class` and `nan cmp` differ in MAME), so the
  exception path can only be checked on hardware.
## Toolchain and validation program (`toolchain.sh`)
`tests/val.c` (printf, malloc, qsort, setjmp, 64-bit divide, hard-float) is built with the in-tree
clang + ld.lld and picolibc (branch `m88k`) and runs in MAME `ncd19c`. `tests/val.expected` is the
output of the same file compiled with the host compiler (`gcc -DHOST tests/val.c`).
`./toolchain.sh all` runs steps 1-5, or give one step name:
1. `clang`    configure (first time) and build clang, lld and tools in `build/`
   (M88k is experimental: `-DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=M88k`).
2. `rt`       build compiler-rt builtins (`build-rt-m88k`), install to clang's resource dir
   (`libclang_rt.builtins.a` for `m88k-unknown-none-elf`).
3. `picolibc` meson/ninja build of `~/Documents/picolibc` (branch `m88k`), installed to `install-m88k`.
4. `prog`     build `tests/val.c` with `build.sh` -> `out/val.coff`.
5. `run`      run in MAME (serial channel B to a TCP listener), save `out/val.png`, diff the serial
   output against `tests/val.expected`.
Env: `LLVM`, `PICOLIBC`, `M88K_CPU` (default `mc88100`), `FPX=1`, `JOBS` (default 2), `CAP`
(memory-cap wrapper, default /home/mk2s/bin/capped; `CAP=` disables). Floating point: MAME
implements the mc88100 FP instructions with softfloat but does not model FP exceptions or reserved
operands, so it is more permissive than real silicon (see the SFU1 section above).
Real hardware: back up `~/Documents/mame/tftpboot/0A000207`, `cp out/val.coff` over it, power-cycle,
watch the screen (and AUX port, 9600 8N1).
