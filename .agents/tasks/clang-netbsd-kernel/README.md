# Building the NetBSD luna88k GENERIC kernel with clang/lld

Result: a GENERIC kernel built entirely with clang/ld.lld from this LLVM tree
boots in nono 1.8.2 to the same `root device:` prompt as the GCC 3.3.6 kernel
(evidence: `boot-gcc.log`, `boot-clang.log`, `boot.diff`).

## Branches and commits

NetBSD (`/home/mk2s/Documents/netbsd-src`, branch `clang-luna88k-kernel`, off `tsutsui-luna88k` b9a40499ccc0):

| commit | change |
|---|---|
| e4e01e9f19e0 | sys: make kernel sources compile with clang (ip_var.h, db_interface.h, db_command.h, omron12x20.h, omrasops.c) |
| a70469c3ee05 | sys: libkern fixes for clang kernel build (`#undef` builtins in memcmp/memset/strcmp/strcpy/strlen.c; `NO_M88K_MOVSTR` in libkern arch/m88k Makefile.inc) |
| be931f7e0a40 | luna88k: volatile frame buffer plane probe in lunafb.c (clang detected 8bpp on a 4bpp board) |

LLVM (`/home/mk2s/Documents/llvm-m88k`, branch `clang-netbsd-kernel`, off `clang-netbsd-target` c509e4d51f90):

| commit | change |
|---|---|
| 84941470cc80 | bulk.sh, FEAT-001 |
| 7919fa4854d1 | lld `-N` etext fix (lld/ELF/Writer.cpp), wrapper/build scripts, FEAT-002 |
| c3b64cf74ad7 | M88k delay slot filler: record implicit operands (CARRY) of intervening instructions; test `llvm/test/CodeGen/M88k/delay-slot-filler.mir` |
| a484b97d8537 | GlobalISel `load_and_mask`: do not narrow loads on big-endian targets; test `llvm/test/CodeGen/M88k/load-and-mask-be.ll` |

The README/logs/FEAT-003 commit follows on the same LLVM branch. Nothing is pushed.

## Decisions

* Language standard: `-std=gnu89`. Clang's default gnu17 rejects the kernel's
  K&R definitions; gnu89 is GCC 3.3's default.
* `-fcommon`: GCC 3.3 default; without it the link has 11 duplicate symbols.
* `-fno-integrated-as` only for `-S` (genassym.sh `XYZZY` lines).
* `-Wno-*` list (clang-only noise, `-Werror` stays on; counts from FEAT-001):
  `-Wno-deprecated-non-prototype` (1124) `-Wno-strict-prototypes` (96)
  `-Wno-pointer-sign` (40) `-Wno-unused-but-set-variable` (33)
  `-Wno-knr-promoted-parameter` (8) `-Wno-unused-function` (4, SPLAY macros)
  `-Wno-incompatible-pointer-types-discards-qualifiers` (4) `-Wno-format-security` (4)
  `-Wno-constant-conversion` (3) `-Wno-shift-negative-value` (2)
  `-Wno-misleading-indentation` (2) `-Wno-enum-conversion` (2) `-Wno-array-parameter` (2)
  `-Wno-tautological-constant-compare` (1) `-Wno-pointer-compare` (1)
  `-Wno-parentheses-equality` (1) `-Wno-header-guard` (1) `-Wno-gnu-folding-constant` (1)
  `-Wno-constant-logical-operand` (1; lunaws.c `rr && RR_TXRDY` is a real bug, left unchanged).
* Where the config lives: everything clang-specific is in
  `clang-wrapper.sh` (CC) and `build.sh` (nbmake overrides) in this task dir;
  the NetBSD makefiles only gained the opt-in `NO_M88K_MOVSTR` knob. The GCC
  build is unchanged by default.

## Reproduction from clean

```sh
# 1. LLVM tools
ninja -C /home/mk2s/Documents/llvm-m88k/build clang lld llc
# 2. clang compile dir (once)
cd /home/mk2s/Documents/netbsd-src/sys/arch/luna88k/conf
~/tools-luna88k/bin/nbconfig -b ~/obj-luna88k/sys/arch/luna88k/compile/GENERIC_CLANG \
    -s /home/mk2s/Documents/netbsd-src/sys GENERIC
# 3. build (wrapper: /home/mk2s/Documents/llvm-m88k/.agents/tasks/clang-netbsd-kernel/clang-wrapper.sh)
sh /home/mk2s/Documents/llvm-m88k/.agents/tasks/clang-netbsd-kernel/build.sh netbsd
#    = nbmake-luna88k -j8 NO_M88K_MOVSTR=1 EXTRA_LINKFLAGS="-z nognustack" CC=clang-wrapper.sh
#      LD=ld.lld AR=llvm-ar RANLIB=llvm-ranlib NM=llvm-nm STRIP=llvm-strip OBJCOPY=llvm-objcopy
#      SIZE="llvm-objdump -h" netbsd      (link: ld.lld -N -Ttext 0x20000 -e start -z nognustack -S)
#    full rebuild: find GENERIC_CLANG -name '*.o' -o -name '*.a' | xargs rm -f
# 4. checks
C=~/obj-luna88k/sys/arch/luna88k/compile
/home/mk2s/Documents/llvm-m88k/build/bin/llvm-nm -u $C/GENERIC_CLANG/netbsd | wc -l          # 0
diff <(sed 's/[[:space:]]\+/ /g' $C/GENERIC/assym.h | sort) \
     <(sed 's/[[:space:]]\+/ /g' $C/GENERIC_CLANG/assym.h | sort)                           # no output
bash /home/mk2s/Documents/llvm-m88k/.agents/tasks/clang-netbsd-kernel/bulk.sh               # 284 OK
/home/mk2s/Documents/llvm-m88k/build/bin/llvm-lit llvm/test/CodeGen/M88k llvm/test/MC/M88k  # from llvm-m88k
# 5. boot in nono (headless, 20 s, nono is killed by timeout)
T=/home/mk2s/Documents/llvm-m88k/.agents/tasks/clang-netbsd-kernel
sh $T/boot-nono.sh $C/GENERIC/netbsd        $T/boot-gcc.log 20
sh $T/boot-nono.sh $C/GENERIC_CLANG/netbsd  $T/boot-clang.log 20
diff $T/boot-gcc.log $T/boot-clang.log
pgrep -a nono    # must print nothing
```

`boot-nono.sh` runs `nono -c nono-headless.cfg -V hostcom-driver=stdio -X <elf>`
under `script(1)` (nono's stdio console needs a tty) and `timeout -s KILL`.
`nono-headless.cfg`: luna88k, `luna-dipsw1 = 10111111` (DIP 1-2 down = serial
console), hostnet/hostsound `none`. The TCP console (`hostcom-driver=tcp`)
lost output depending on when the client connected, so it is not used for logs.
`-X` loads the ELF directly; no disk image or boot loader is needed.

## Results (done criteria)

| # | criterion | result | evidence |
|---|---|---|---|
| 1 | every kernel file compiles with clang, -Werror, documented -Wno-* | 284 OK / 0 FAIL, no LLVM ERROR / unable to legalize | `bulk/result.txt`, `bulk/*.err` |
| 2 | full link, no unresolved symbols | `llvm-nm -u` = 0; build log has 0 `m88k--netbsdelf3` invocations | `build-clang.log` |
| 3 | clang assym.h equals GCC assym.h | equal after whitespace normalisation and sort | command in step 4 |
| 4 | same image format as GCC | ELF32 BE MC88000 EXEC, entry 0x20000, one RWX PT_LOAD at 0x20000; 1789592 bytes vs 1735468 | `compare.md` |
| 5 | boot in nono | boots to `root device:` (same as GCC). Remaining diff: symtab address/entries, version string, avail memory 61536 vs 61588 KB (larger kernel), le0 MAC (random per run, also differs between GCC runs) | `boot-gcc.log`, `boot-clang.log`, `boot.diff` |
| 6 | GCC build still works | fresh `nbconfig` + `nbmake-luna88k -j8 netbsd` with original tools in `/tmp/nb/GENERIC_GCC`: exit 0, text 1422972 vs reference 1423148 (data/bss equal); boots to `root device:` | `build-gcc-regress.log`, `size-gcc-regress.txt`, `boot-gcc-regress.log` |
| 7 | README with exact commands | this file | |

Note for criterion 6: run `nbmake-luna88k assym.h` before `-j8` in a fresh
dir (`make depend` was not run); otherwise copy.S can race assym.h.

## Bugs found while boot-testing

1. **uvm panic** `uvm_km_suballoc: unable to allocate space in parent map`.
   Bisected (per file -O0, then per function with `optnone`, then llc options)
   to `uvm_map_findspace` and `-m88k-enable-delay-slot-filler`: the filler moved
   `addu.co` past the `addu.ci` that reads its carry. Fixed in M88kDelaySlotFiller.cpp.
2. **Hang after `scsibus0: waiting 2 seconds`** (callouts never fired). A
   memory breakpoint on the callout wheel showed `callout_hardclock()`
   corrupting bucket 0: `hardclock_ticks & 255` was compiled to `ld.bu` of the
   word's first byte (most significant on big-endian). Fixed in the generic
   GlobalISel `load_and_mask` combine.
3. **fb0 8bpp instead of 4bpp**: real source bug in lunafb.c (non-volatile read-back
   of a probe write); fixed portably.

## Remaining risks

* Only boot to the root prompt is tested; no root file system, userland,
  network or SCSI disk I/O was exercised.
* Argument passing differs from GCC 3.3: GCC callees read stack arguments at
  incoming `r31+32`, clang at `r31+0`, and struct-by-value arguments use
  different registers. An all-clang kernel is self-consistent, but hand
  assembly that passes or reads stack arguments, or mixing GCC and clang
  objects, would break (mixing GCC `uvm_map.o` into the clang kernel faulted).
* ddb stack traces (db_trace.c prologue parsing) are tuned to GCC code.
* `llvm-lit` for lld's new `etext-omagic.s` is UNSUPPORTED (no m88k lit feature).
