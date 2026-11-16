# GENERIC kernel: GCC 3.3.6 vs clang/lld (FEAT-002)

GCC reference: `~/obj-luna88k/sys/arch/luna88k/compile/GENERIC/netbsd` (1735468 bytes, untouched).
Clang build: `~/obj-luna88k/sys/arch/luna88k/compile/GENERIC_CLANG/netbsd` (1789592 bytes before strip step; plain ELF, no post-processing).

## Build command

```
sh /home/mk2s/Documents/llvm-m88k/.agents/tasks/clang-netbsd-kernel/build.sh        # = make netbsd in GENERIC_CLANG
```

`build.sh` runs `nbmake-luna88k -j8 NO_M88K_MOVSTR=1 EXTRA_LINKFLAGS="-z nognustack" CC=clang-wrapper.sh LD=ld.lld AR=llvm-ar RANLIB=llvm-ranlib NM=llvm-nm STRIP=llvm-strip OBJCOPY=llvm-objcopy SIZE="llvm-objdump -h" netbsd`.
Dir seeded with `nbconfig -b .../GENERIC_CLANG -s .../sys GENERIC` (run from sys/arch/luna88k/conf). Full build (all 284 files, libkern, vers, link) takes about 6 s. No `m88k--netbsdelf3-*` tool is invoked (0 matches in the build log).

`clang-wrapper.sh`: `--target=m88k-unknown-netbsd -mcpu=mc88100 -fno-pie -std=gnu89 -fcommon`, then the makefile's flags, then the FEAT-001 `-Wno-*` list (after the makefile flags, so it overrides `-Wstrict-prototypes`), and `-fno-integrated-as` only when `-S` is given.
Why each wrapper item:
* `-fcommon`: GCC 3.3 default. With clang's `-fno-common` the link has 11 duplicate symbols (boothowto, avail_start, ...).
* `-fno-integrated-as` for `-S`: `genassym.sh` emits `__asm("XYZZY name %0")` and compiles with `-S`; the integrated assembler would try to parse `XYZZY` and error. The text pass-through leaves `genassym.sh` unchanged.

## NetBSD source/Makefile changes in FEAT-002

* `sys/lib/libkern/{memcmp,memset,strcmp,strcpy,strlen}.c`: `#undef` the name after including libkern.h. libkern.h maps them to `__builtin_*`; GCC accepts defining the builtin (the symbol still comes out as `memcmp`), clang errors "definition of builtin function".
* `sys/lib/libkern/arch/m88k/Makefile.inc`: the 16 `move*.s` GCC block-move helpers (`__movstr*`, Motorola syntax, generated from `gnu/dist/gcc/.../m88k-move.sh`) are skipped when `NO_M88K_MOVSTR` is set. Default behaviour (GCC) unchanged.

## LLVM change in FEAT-002

`lld/ELF/Writer.cpp`: with `-N` (omagic) lld has no read-only PT_LOAD, so `etext` stayed at the start of the image (0x1034). `pmap_bootstrap` (arch/m88k/m88k/pmap.c) uses `&etext` to round up and split the kernel text (RO) from data (RW): with 0x1034 `e_text` < `s_text`, which would break the kernel mappings. Now with `-N` etext is the end of the last read-only allocated section (0x1930b0, rounds to .data at 0x194000), as GNU ld does. Test: `lld/test/ELF/etext-omagic.s` (`REQUIRES: m88k`, not run by lit here because the m88k feature is not defined; its commands were run by hand).

## assym.h

`diff <(sort GENERIC/assym.h) <(sort GENERIC_CLANG/assym.h)`: same 105 defines and values. Only difference is whitespace: GCC emits `#define<TAB> NAME VALUE` (leading ` ` from the operand prefix), clang emits `#define NAME VALUE`. Identical after whitespace normalisation. Harmless (cpp/assembler macros).

## ELF header / program headers

| | GCC | clang+lld |
|---|---|---|
| class/data/machine | ELF32 BE MC88000 | same |
| type | EXEC | EXEC |
| entry | 0x20000 | 0x20000 |
| PT_LOAD | one, RWE, off 0x1000, vaddr/paddr 0x20000, filesz 0x16c640, memsz 0x1810e0, align 0x1000 | one, RWE, off 0x1000, vaddr/paddr 0x20000, filesz 0x182d4c, memsz 0x191fe4, align 0x1000 |
| other phdrs | none | none (lld emits GNU_STACK by default; removed with `-z nognustack`) |

## Sections

| section | GCC addr / size | clang addr / size |
|---|---|---|
| .text | 0x20000 / 0x15b490 (includes strings, WAX) | 0x20000 / 0x14e1d4 (AX) |
| .rodata | 0x17b490 / 4 | 0x16e1d8 / 0x24c3d |
| link_set_* (7) | 0x17b494..0x17b72c | 0x192e18..0x1930b0 (same sizes) |
| .data | 0x17c000 / 0x10640 | 0x194000 / 0xed4c |
| .sbss | 0 | absent |
| .bss | 0x18c700 / 0x149e0 | 0x1a2d50 / 0xf294 |
| .comment/.note/.ident | 0x3318 / 0x19f0 / 0x3db5 | 0x2c / absent / 0x3db5 |

Read-only part (text+rodata+link sets, up to .data): GCC 0x15c000 bytes, clang 0x174000 (+98 KB, about +6%). .data and .bss are smaller with clang (0xed4c/0xf294 vs 0x10640/0x149e0). Total in-memory size: GCC 0x1810e0, clang 0x191fe4 (+7%). `.symtab/.strtab` are present in both (needed by ddb and the boot loader's symbol loading).

## Symbols (global, defined: `llvm-nm -g --defined-only`)

GCC 4159, clang 4063. Missing in clang: 96.
* 87 `__movstr*`: GCC-only block-move helpers (see above).
* `__ashldi3 __ashrdi3 __lshrdi3 __cmpdi2 __ucmpdi2 __negdi2`: libkern archive members not pulled in because clang expands these operations inline. No undefined symbols remain.
* `__bss_start _edata _end`: GNU ld linker-script symbols; lld defines only `edata`/`end`/`etext` and `_edata`, `_end` on demand. The kernel uses `edata/end/etext/start`, all present.
Extra in clang: none. `llvm-nm -u netbsd`: 0 lines.

## ABI review

* Calling convention: LLVM M88k CSR set is r1, r14-r25, r30; r28/r29 reserved, r31 SP (M88kCallingConv.td / M88kRegisterInfo.cpp) which matches the SYS-V m88k ABI used by GCC and by `PCB_R14..PCB_R30` saved in cpu_switch/`eh_common.S`.
* Hand-written asm (locore.S, eh.S, arch/m88k/m88k/*.S, libkern bcopy/bzero/memcpy/setjmp): assembled by clang's integrated assembler without errors (all 284 files OK). The assembler does no delay-slot reordering, as GNU as `-O0`. Offsets used by asm come from assym.h which is identical to the GCC one, so struct layout (lwp, pcb, cpu_info, exception frame) agrees between the two compilers.
* Inline asm (arch/m88k/include/{asm_macro,lock,mmu,cpu,db_machdep}.h, boot fault.c, sig_machdep.c, m8820x_machdep.c): all compile; constraints used are simple `r`, `i`, `0`.  Not verified at run time; the pipeline flush string (`FLUSH_PIPELINE_STRING`) is emitted verbatim.
* Risks to check at boot (FEAT-003):
  * `db_trace.c` parses GCC prologues (`st r30, r31, imm16`) to build backtraces; clang -O2 frames differ, so ddb `trace` output may be wrong (cosmetic).
  * `etext`: fixed by lld patch (above); depends on the rebuilt lld.
  * `ld.lld -N` sets no PT_PHDR/GNU_STACK (with `-z nognustack`), layout is a single RWX LOAD like GCC's.
  * `-fno-zero-initialized-in-bss` is only added by Makefile.kern.inc for non-m88k, so both builds use default bss placement; tentative definitions go to common with `-fcommon`, as GCC 3.3 does by default.
  * 64-bit shifts/compare/negate are expanded inline by clang; the libkern versions are unused.
