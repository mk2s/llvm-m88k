# OpenBSD shared objects for m88k (LLVM 22 port)

Notes carried over from zig-m88k-bootstrap (branch openbsd-shared-objects). Commit hashes and the Zig parts refer to that repository; the LLVM and lld changes are on this branch.

## OpenBSD shared objects for m88k-openbsd (branch `openbsd-shared-objects`; Phase 0 findings, then the implementation below)
Full spec: `.agents/tasks/openbsd-so/spec.md` (untracked, in the main checkout). Reference source: OpenBSD 7.1 (CVS tag `TOPENBSD_7_1`) subset in `/home/mk2s/Documents/openbsd-src-7.1` (`libexec/ld.so`, `lib/csu`, binutils bfd/ld m88k, gcc m88k). Reference binaries: the real 7.1 luna88k filesystem at `/mnt/obsd`, run with `run88k --root /mnt/obsd [--mount G=H] --cwd DIR run PROG` (static `echo`, dynamic `true`/`uname`/`id` all run through the real `ld.so`).
- ABI: real system binaries (libc, ld.so, crt, /usr/bin) have `EI_OSABI` 0 (SysV), ABI version 0, `e_flags` 0. Target ABI stays `.none`, output OSABI 0. Before this work `zig build-obj -target m88k-openbsd` gave OSABI 12 (from `M88kMCAsmBackend`), and lld copies the first non-zero OSABI of its inputs, so the backend was changed to emit 0 (see the implementation section).
- Dynamic tags `0x70000002` / `0x70000003` are `DT_88K_PLTSTART` / `DT_88K_PLTEND` (`m88k/exec.h`): vaddr of the start and end (`.plt` vma + size) of `.plt`. ld.so reads them only in `_dl_md_reloc_got` to `_dl_cacheflush(plt_start + base, plt_end - plt_start)` (trap 451) after patching the PLT. `0x70000001` (ADDRBASE) and `0x70000004` (TDESC) are unused. They are plain vaddrs, not rebased by `_dl_finalize_object`.
- Relocations ld.so (7.1 `m88k/rtld_machine.c`) handles: the leading `DT_RELACOUNT` entries as `*P = base + addend`; then NONE, COPY, GOTP_ENT (JMPREL only; patches GOT slot and the PLT entry immediates at halfword 1 and 3), DISP26, BBASED_32, 32, 16H, 16L. Anything else dies. libc needs only BBASED_32 (2619), 32 (43), GOTP_ENT (5); ld.so itself only BBASED_32. No extra dynamic reloc type is needed.
- Static relocs lld needed for PIC objects (all now implemented, except that `R_88K_PLT_DISP26` already existed): `R_88K_ABDIFF_16H/16L` (`GOT - (label + A)`, GOT = start of `.got.plt` = DT_PLTGOT = `_GLOBAL_OFFSET_TABLE_`), `R_88K_GOT_16H/16L` (offset of the GOT entry from the `.got.plt` start, entry gets `R_88K_32` or `R_88K_BBASED_32`), `R_88K_PLT_DISP26`. Layout: `.plt` (72-byte header, 24-byte entries, already what lld writes) in its own RWE LOAD, `.got[0]=_DYNAMIC`, `[1]`/`[2]` set by ld.so, PLT slots next, then data slots. W+X segments are mapped without X, patched, then `mprotect`ed to R+X, so `.plt` needs its own page range.
- PIC convention (GCC 3.3.6): r25 = GOT base, set in the prologue with `or.u r25,r0,hi(L#abdiff); bsr.n L; or r25,r25,lo(L#abdiff); L: addu r25,r25,r1`; globals via `ld rX,r25,sym#got_rel`; calls via `bsr sym#plt`; r27 is the OpenBSD thread pointer. At the start of the work the backend had no PIC mode (`Reloc::Static` only) and Zig `-dynamic` failed with `unable to provide libc for target "m88k-openbsd.7.8...7.9-none"`; the implementation below addresses both (libc now comes from the guest filesystem via `--libc`).
- First version is BIND_NOW-only (`DT_BIND_NOW` + `DT_FLAGS_1 = NOW`), so `_dl_bind`/`kbind` are not exercised for DSOs.

## OpenBSD shared objects for m88k-openbsd: implementation (Phases 1-4)
Result: `zig build-lib -dynamic -target m88k-openbsd` produces ET_DYN shared objects, and `zig build-exe` produces non-PIE ET_EXEC that link them. The real OpenBSD 7.1 `ld.so` loads and runs them under run88k. Commits on the branch: `33e2f585d2` (LLVM PIC), `aebdd1cf76` (lld), `2741f3c723` and `59132c0459` (Zig and a GOT fix), `88bbc560fe` (runtime tests).

**PIC convention (same as GCC 3.3.6, so objects mix with the system libc and crt)**
- `-relocation-model=pic` selects GOT-based code. r25 is the GOT base, set in each function that needs it by the `GETGOT` pseudo: `or.u %r25,%r0,hi16(L#abdiff)`, `bsr.n L`, `or %r25,%r25,lo16(L#abdiff)` (delay slot), `L: addu %r25,%r25,%r1`. This yields `GOT - L`, then adds the `bsr.n` return address. GETGOT never gets a delay slot filled.
- Globals, function addresses and jump table addresses load with `ld %rX,%r25,%got_lo16(sym)`. Calls are plain `bsr sym` (DISP26); the linker routes preemptible targets through the PLT.
- Jump tables stay absolute and go to `.data.rel.ro` under PIC. r26 and r27 stay reserved (r27 is the OpenBSD thread pointer).
- Non-PIC (`Reloc::Static`) output is unchanged.

**LLVM changes (`llvm/lib/Target/M88k`)**
- New MC specifiers `%got_hi16`, `%got_lo16`, `%abdiff_hi16`, `%abdiff_lo16`, with fixups and ELF relocations `R_88K_GOT_16H/16L` and `R_88K_ABDIFF_16H/16L`.
- `M88kObjectWriter::needsRelocateWithSymbol` returns true for the GOT relocations. Without it MC rewrote `%got_lo16(.L.str)` as section+offset and lld added the offset to the section's GOT entry, which loaded NULL for string literals in PIC executables (fix `59132c0459`).
- Aligned atomic loads and stores of 32 bits or less select as plain `ld`/`st` (needed by compiler_rt emutls).
- Tests: `MC/M88k/reloc-pic*.s`, `reloc-got-local.s`, `CodeGen/M88k/pic-*.ll`, `atomic-load-store.ll`.

**LLD changes (`lld/ELF`)**
- `Arch/M88k.cpp`: `R_88K_ABDIFF_16H/16L` = `GOT - (S+A)` (via `R_GOTPLTREL`); `R_88K_GOT_16H/16L` = offset of the symbol's GOT entry from the start of `.got.plt`, the GOT base (via `R_GOTPLT`, 16L is range checked). `_GLOBAL_OFFSET_TABLE_` equals `DT_PLTGOT`.
- `Writer.cpp`: `.got` directly follows `.got.plt` so GOT offsets stay within 0..0xffff. `PT_OPENBSD_RANDOMIZE` and `.openbsd.randomdata` are produced for EM_88K even though OSABI is 0.
- `SyntheticSections.cpp`: emits `DT_BIND_NOW` (the tag OpenBSD `ld.so` reads) instead of `DF_BIND_NOW` for m88k.
- Dynamic relocations are only `R_88K_BBASED_32` (sorted first, counted by `DT_RELACOUNT`), `R_88K_32`, `R_88K_COPY` (executables) and `R_88K_GOTP_ENT` in `.rela.plt`. These are the ones `ld.so` supports. `.hash` and `.gnu.hash` are both emitted. Comparison with libc: `docs/openbsd-so-lld-check.md`.
- Tests: `lld/test/ELF/m88k-got.s`, `m88k-shared.s`, `m88k-plt.s`.

**Zig changes**
- `target.zig`, `Compilation/Config.zig`, `main.zig`: m88k precludes PIE. Executables default to non-PIE, and `-fPIE` gives `error.TargetPrecludesPie`. Dynamically linked code is still compiled `-fPIC`.
- `link/Lld.zig`: m88k-openbsd executables use `--image-base 0x1000` like system binaries. The existing `-shared -znow -soname`, `--dynamic-linker /usr/libexec/ld.so` flags already fit.
- `build.zig`, `test/llvm_targets.zig`, `test/cases.zig`, `test/src/Cases.zig`: m88k-freestanding/openbsd object cases and a `m88k_openbsd_shared_object` case, enabled by `-Dllvm-has-m88k -Dm88k-openbsd-root=<root>`.
- Zig ships no OpenBSD m88k libc stubs, so libc comes from the guest filesystem (`--libc` file with `crt_dir=/mnt/obsd/usr/lib`, and link `libc.so.96.1` explicitly). `-lc` alone resolves to `libc.a` and fails with a duplicate `atexit` symbol.

**ABI / OSABI decision**
Output has `EI_OSABI = 0` (ELFOSABI_NONE), ABI version 0, `e_flags = 0`, and the Zig target ABI stays `.none`. Every real 7.1 m88k binary (libc, ld.so, crt, /usr/bin) is built that way, and `ld.so` and run88k never read EI_OSABI. lld copies the first non-zero OSABI of its inputs, so one Zig object marked 12 (OpenBSD) would change the output and make it differ from the system. The m88k asm backend therefore emits 0. OpenBSD identity comes from `.note.openbsd.ident` and `PT_OPENBSD_RANDOMIZE`.

**Dynamic tags 0x70000002 and 0x70000003**
They are `DT_88K_PLTSTART` and `DT_88K_PLTEND` (`m88k/exec.h`): the start and end (`.plt` vaddr + size) of `.plt`, as plain unrelocated vaddrs. `ld.so` uses them only in `_dl_md_reloc_got`, which adds the load base and calls `_dl_cacheflush` (trap 451) over the range after patching the PLT entries. They are not needed for correctness in run88k (the flush is a no-op) but real hardware needs the I-cache flush, so the linker emits them whenever a PLT exists. `0x70000001` (ADDRBASE) and `0x70000004` (TDESC) are unused and not emitted.

**Build and validate**
- Build limits stay `-j4`, 8G cap. Tools: LLVM/lld tools in `out/build-so-tools` (build `lld llvm-readelf llvm-objdump llvm-mc llvm-readobj llc FileCheck not`; lld needs `CPLUS_INCLUDE_PATH=llvm-m88k-22/libunwind/include` for the Mach-O header). Zig links against `out/install-llvm22-so`, a copy of `llvm-m88k-22/install-zig` with the rebuilt `libLLVMM88k*.a`, `liblldELF.a`, `liblldCommon.a` overlaid (copy `libLLVMM88kDesc.a` again after backend changes). Then `cmake --build out/build-zig-so --target install -j4` gives `out/zig-so/bin/zig`.
- Unit tests: `out/build-so-tools/bin/llvm-lit -s lld/test/ELF/m88k-*.s llvm/test/MC/M88k llvm/test/CodeGen/M88k` (94 passed at commit `59132c0459`).
- Zig tests: `zig build test-cases -Denable-llvm -Dllvm-has-m88k -Dm88k-openbsd-root=/mnt/obsd -Dtest-filter=m88k -Dtest-target-filter=m88k`.
- Runtime tests: `bash tests-openbsd-so/run.sh`. Variables: `ZIG` (default `out/zig-so/bin/zig`), `RUN88K` (the default is an absolute path under `/home/mk2s/Documents/run88k`, specific to this machine; set it elsewhere), `ROOT` (default `/mnt/obsd`, OpenBSD 7.1), `W` (default `/tmp/so-rt`). It builds a C DSO plus exe at Debug (with `-fno-sanitize-c`, see gaps), ReleaseFast and ReleaseSmall, a libc-only exe, a GCC-crt variant, and Zig-source DSOs, then runs each as `run88k --os openbsd --root /mnt/obsd --mount /work=/tmp/so-rt --cwd /work --trace run /tmp/so-rt/<prog>`. It checks globals, data pointers, constructors via `.init_array`, function pointers, a callback into the exe, COPY relocation, `.data.rel.ro`, and that the trace shows PLT pages `mprotect`ed R|X and no `kbind`. Last result: `passed=22 failed=0`. `--os openbsd` is required because `--gc-sections` drops the unused `__guard_local` (`.openbsd.randomdata`), so run88k would not detect OpenBSD.

**Remaining gaps**
- Lazy binding is not implemented or tested for shared objects: DSOs are `DT_BIND_NOW` + `DF_1_NOW` only. The PLT and `.got[1]`/`.got[2]` are laid out so lazy binding can work, but `_dl_bind`/`kbind` is never exercised for a DSO. Executables keep lld's prefilled GOT.
- No `dlopen` test, no text relocations (`DF_TEXTREL`), no TLS in shared objects (OpenBSD uses emulated TLS).
- Zig has no OpenBSD m88k libc stubs; the guest filesystem must supply libc and crt. The default OS version range is 7.8 to 7.9 while the test filesystem is 7.1 (used only for libc selection).
- Not covered by the runtime tests: Zig-source Debug builds (std panic needs `G_ATOMICRMW_ADD` on s8) and C Debug with the UBSan runtime (the suite therefore builds C Debug with `-fno-sanitize-c`) (`G_MEMCPY_INLINE`). Both are legalizer gaps unrelated to shared objects.
- Not tested on real hardware or against the 7.1 kernel; only the real `ld.so` under run88k.
- Rarely used relocations (`GOT_32*`, `GOTP_*` other than ENT, `PLT_16*`, `ABREL_*`, `BBASED_16*`) are not handled and give an lld error.
