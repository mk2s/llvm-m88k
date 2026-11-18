# Porting the m88k work to a new LLVM release

The m88k backend, clang/lld/lldb support and the bare-metal tests are out of tree and are not
upstreamed. Each time Zig moves to a new LLVM (Zig 0.17 = LLVM 22.1, Zig 0.18 = probably LLVM 23)
the work is replayed onto the new upstream release. The procedure is the workflow recipe
`.kiro/workflows/m88k-llvm-port.workflow.json` (name `m88k-llvm-port`). This file describes it, lists
its parameters and keeps the log of API adaptations per release. The recipe reads and appends to this file.

## Decisions

- Rebase per release with `git rebase --onto`, `rerere` enabled. No merges of upstream into the m88k branches.
- Each release gets a new branch `release/NN.x-m88k` created from the upstream tag. The four old branches
  (`main-m88k`, `gnu-asm-mode`, `clang-netbsd-target`, `clang-netbsd-kernel`) are archives and are not modified.
- The old four-branch stack is collapsed into that one branch. Later ports start from the previous
  `release/NN.x-m88k` alone.
- `origin/*` refs are ignored. Upstream is the local checkout `/home/mk2s/Documents/llvm-project`; remotes of record are `github` and `codeberg`.
- The recipe is designed during the 22.1 port and reused unchanged for the next release.

## Process (phases of the recipe)

| Phase | Recipe step | What happens | Status |
|---|---|---|---|
| 0 | `setup` | Defaults the parameters, writes `run.json` and a run dir under `.agents/tasks/m88k-llvm-port/` | drafted, dry-run checkable |
| 1 | `preflight` | Tools, rerere, stack is linear, clean tree, disk space, conflict forecast with `git merge-tree`, backup refs and a git bundle | drafted, dry-run checkable |
| 2 | `baseline` | `scripts/m88k-baseline.sh` on the source stack: results file in `baseline_dir` | script tested for the lit step only |
| 3 | `create-branch`, `replay-loop`, `replay-review` | Worktree on `release/NN.x-m88k` at the target tag, replay each stack segment with `rebase --onto`, resolve conflicts, review | UNTESTED |
| 4 | `build-fix-loop` | Up to 8 iterations: build, fix one error cluster, commit it as `adapt to LLVM NN: ...`, log it | UNTESTED (draft) |
| 5 | `verify-run`, `triage-loop`, `verify-review` | Baseline script on the new build, `m88k-baseline-diff.sh` against the old baseline, triage every difference | UNTESTED (draft) |
| 6 | `report` | `report.md` in the run dir, a new section in the adaptation log below | UNTESTED (draft) |

The recipe pauses and asks the user when a test regression needs triage (upstream behavior change versus a real
m88k bug), when a conflict resolution could change generated code or the ABI, when the build-fix loop stops
making progress, when `release_branch` already exists, and before a multi-hour rebuild of the source stack.

### Why a worktree

The new branch is built in a separate git worktree (`work_dir`, default `<repo_dir>-NN`). The old checkout and
its `build/` stay intact, so the baseline can be re-run and old and new results can be compared on the same machine.

## Parameters (`inputs` of the recipe)

An empty value is replaced by the default in step `setup`; the resolved values are in `<run dir>/run.json`.
Nothing about the LLVM version is hardcoded: NN comes from `target_tag`.

| Parameter | Default | Meaning |
|---|---|---|
| `target_tag` | none, required | Upstream tag to port to, e.g. `llvmorg-22.1.8` |
| `repo_dir` | `/home/mk2s/Documents/llvm-m88k` | The m88k repository |
| `upstream_dir` | `/home/mk2s/Documents/llvm-project` | Upstream checkout that has `target_tag` |
| `source_ref` | newest `release/NN.x-m88k`, else `clang-netbsd-kernel` | Tip of the previous port or of the current stack |
| `old_base` | merge-base of `source_ref` with upstream (22.1 port: `4590f755cf58`) | Upstream commit the source stack sits on |
| `stack_order` | `main-m88k gnu-asm-mode clang-netbsd-target clang-netbsd-kernel` | Branches bottom to top. For a later port: the single previous release branch |
| `release_branch` | `release/NN.x-m88k` | New branch |
| `work_dir` | `<repo_dir>-NN` | Worktree for the new branch |
| `build_dir` | `<work_dir>/build` | Build dir of the new branch. LLDB uses `<build_dir>-lldb` |
| `baseline_dir` | `<repo_dir>/.agents/tasks/m88k-llvm-port/baseline-<source_ref>` | Where the baseline results go |
| `baseline_build_dir` | `<repo_dir>/build` | Build dir of the source stack |
| `dry_run` | `false` | `true` = read-only checks, see below |

Port 22.1 (first port): `target_tag=llvmorg-22.1.8`, everything else default.
Port for Zig 0.18: `target_tag=<new tag>`, `source_ref=release/22.x-m88k`, `stack_order=release/22.x-m88k`,
`old_base=llvmorg-22.1.8`, `baseline_build_dir=<work_dir of the 22.1 port>/build`.

## Scripts

- `scripts/m88k-baseline.sh [--background] [--only STEPS] [--skip STEPS] BASELINE_DIR` runs: lit for the M88k tests
  (llvm CodeGen/MC, clang, lld; found with `git ls-files`), an LLDB smoke check, compiler-rt builtins for
  `m88k-unknown-none-elf` and `m88k-unknown-openbsd` (configure, build, install into clang's resource dir), and the
  ncd19c bare-metal tests in MAME. Output: `results.txt` (sorted, one `<suite>/<name> <STATUS>` per line, no
  timestamps), `meta.txt` (commit, versions, dates; not diffable) and `logs/`. Defaults come from the existing
  build dirs; override with `M88K_SRC`, `BUILD_DIR`, `LLDB_BUILD_DIR`, `RT_TRIPLES`, `RT_DIR_SUFFIX`, `M88K_CPU`, `JOBS`.
  `--background` runs it under nohup and logs to `BASELINE_DIR/logs/baseline.log`. MAME runs in window mode and needs a display.
- `scripts/m88k-baseline-diff.sh BASE RESULTS` compares two results files (or baseline dirs): new failures, new tests
  failing, fixed, status changed, missing, added. Exit 1 on new failures or missing tests.

## Dry run

`dry_run=true` makes the recipe read-only. It checks, and nothing more:

- inputs resolve and `target_tag` exists upstream;
- tools, rerere, DISPLAY and MAME are present, disk space is enough;
- every stack branch exists and is an ancestor of the next; commit counts per segment;
- backup is skipped, `release_branch` and `work_dir` do not exist;
- a whole-segment conflict forecast with `git merge-tree --write-tree` against the target tag;
- `m88k-baseline.sh --only lit` runs against the current build and `m88k-baseline-diff.sh` of that result against itself exits 0.

On dry_run, `setup` pre-writes `replay-state.json` (DONE), `build-review.json` (APPROVED) and `triage-state.json` (PASS), each flagged `dry_run: true`, so the three repeat loops exit at once. Every step after `setup` reads `repo_dir`, `baseline_dir` and the other paths from `run.json`, so only `target_tag` is required on the command line.
A dry run does not prove the replay, the build or the compiler-rt/bare-metal steps. The conflict forecast
approximates the real replay: a clean forecast does not guarantee a clean `rebase --onto` and the reverse.

## Baseline of the LLVM 21 stack
- Location: `.agents/tasks/m88k-llvm-port/baseline-llvm21/` (`results.txt`, `meta.txt`, `logs/`). Convention: `baseline-<source_ref>`.
- Date: 2026-11-18 run of `scripts/m88k-baseline.sh` (see `meta.txt` for the exact time). Source commit: `bb610c33b5f9ec6feac72e3a6b5ae4ad8b604b8c` on `clang-netbsd-kernel`, LLVM 21-based. Builds were used as they were, not rebuilt.
- Counts: lit 101 PASS; lldb 2 PASS; rt 4 PASS (build and install for none-elf and openbsd); baremetal 11 PASS, 1 FAIL.
- Pre-existing failure: `baremetal/fpx-real` FAIL. Two known NaN/inf lines differ from `fpx-real.expected` (`inf-inf class`, `nan cmp`). It is the baseline; do not fix it in the port.
- Script fixes made for this run: `rec` aborted under `set -u` when called with one argument (broke the rt and baremetal steps), and `plugin list` does not exist in this lldb (the ABI plugin check now greps liblldb).

## Untested parts

Updated after the 22.1 port. Earlier state:

- Tested by hand: `m88k-baseline.sh --only lit` on the LLVM 21-based build (101 results, all PASS), `m88k-baseline-diff.sh` on sample files, `bash -n` on both scripts, JSON validity of the recipe.
- Exercised by the 22.1 port (2026-11-18): worktree and per-segment replay, the build-fix loop, verify, triage and report steps, compiler-rt builtins rt build/install, baseline diff on the new build.
- Still untested: the `lldb` M88k behaviour beyond start-up (no lit tests); a dry run of the recipe; reuse of the recipe for another release (planned for Zig 0.18).
- Not available here: `shellcheck`.
- Known gap: `lldb` has no lit tests for M88k. The lldb step only checks that the binary starts and lists the M88k ABI plugin.

Remove an item from this list only when a run exercised it.

## Adaptation log

One section per release, added by the `report` step. Each row: commit, files, upstream API change, upstream commit.
Read the earlier sections before starting a port: the same breakage often returns in a different form.

### LLVM 22 (llvmorg-22.1.8, 2026-11-18)
Stack: 404 commits on llvmorg-22.1.8 (369+2+2+31), branch release/22.x-m88k. Build-fix loop took 7 iterations; test diff vs LLVM 21 baseline: 0 new failures, 0 missing (119 results, fpx-real still the known FAIL).

| Commit | Files | Upstream API change | Upstream commit |
|---|---|---|---|
| a92d633e61c8 | llvm/utils/TableGen/GlobalISelBURGEmitter.cpp | CodeGenTarget::getInstructionsByEnumValue() removed, use getInstructions() | (api removal, ordering now inherent) |
| 4a641a1e604c | BURSInstructionSelector.h, BURSInstructionSelect.cpp, M88k GISel/*.cpp, M88kCombine.td | GISelKnownBits -> GISelValueTracking (+AnalysisLegacy, getValueTracking, KB->VT) | 1d0005a69ac8 |
| b9de61a85b41 | clang/lib/AST/ASTContext.cpp | ASTContext::getRecordType -> getCanonicalTagType | 91cdd35008e9 |
| 10d50696d2e0 | clang/lib/Basic/Targets/M88k.cpp | ArrayRef(std::nullopt) removed, return {} | (ArrayRef cleanup) |
| 3bcc7af1434c | llvm/lib/CodeGen/GlobalISel/BURSInstructionSelect.cpp | reportGISelFailure no longer takes TPC | (GlobalISel Utils change) |
| d48c5e62d5fb | llvm/lib/Target/M88k/M88k.td | defm RemapAllTargetPseudoPointerOperands<GPR> mandatory | 7bf459bce342 |
| d444fef8c205 | clang/lib/Driver/ToolChains/Arch/M88k.cpp | CommonArgs.h now clang/Driver/CommonArgs.h | (header move) |
| 42bd229d9086 | clang/lib/CodeGen/Targets/M88k.cpp | getNaturalAlignIndirect requires AddrSpace | 39ec9de7c230 |
| 0e5de518e4a4 | clang/lib/Driver/ToolChains/Arch/M88k.cpp | clang/Driver/Options.h -> clang/Options/Options.h | 9a783b63e647 |
| 1e8440030f6b | M88kInstrInfo.{h,cpp}, M88kRegisterInfo.{h,cpp}, M88kFrameLowering.cpp | TII/TRI hook signatures, GenInstrInfo ctor | 2331fbb01978, 1f3f522866ae, 8b7a76a2ace3 |
| a84435db5a70 | M88kCallingConv.h, M88kISelLowering.cpp, M88kTargetMachine.cpp | CCAssignFn OrigTy; TargetLowering(TM,STI); DataLayout::getManglingComponent removed | e92b7e964194, f3efbce4a73c |
| 117d7785d06d | M88kAsmParser.cpp, M88kDisassembler.cpp | MCAsmLexer->AsmLexer; MCD namespace for decoder tables | c9f6d26e0498, 7b6db769c347 |
| f3b8734d1149 | M88kInstrInfo.{h,cpp}, AsmParser/M88kAsmParser.cpp | loadRegFromStackSlot SubReg; MCParsedAsmOperand::print(OS,MAI); MCAsmInfo::useCodeAlign | 9e1606026c56, e9de1ee9f57c |
| 8581a7f723ef | M88k MCTargetDesc/*, AsmParser, M88kMCInstLower.cpp (M88kMCExpr.{h,cpp} removed) | MCTargetExpr->MCSpecifierExpr; applyFixup/maybeAddReloc replaces shouldForceRelocation; MCFixupKindInfo.h merged; MCFixup::create no SMLoc; getRelocType sig | 38658b54b244, dd2891535d4b, cd39eaeb376e, d5893fc2a7e1 |

Conflict patterns and resolutions:
- Triple.h/Triple.cpp: upstream reformats the enum and StringSwitch tables, so each m88k commit touching them conflicts as a block. Take upstream's block, re-add the m88k lines.
- New per-target clang builtin code goes to clang/lib/CodeGen/TargetBuiltins/<Target>.cpp, not CGBuiltin.cpp.
- LLVM_HAS_<T>_TARGET lives in llvm/include/llvm/Config/Targets.h.cmake (upstream 667209e45122).
- New declarations in public LLVM headers (InitializePasses.h) need LLVM_ABI.
- lld DynamicReloc::Kind removed: `AgainstSymbolWithTargetVA, X` -> `true, X`; `AgainstSymbol` -> `true, R_ADDEND`; `AddendOnlyWithTargetVA, X` -> `false, X`.
- GlobalISel InlineAsmLowering now handles non-indirect memory inputs; the m88k hunk was redundant.
- Clang driver include lists, compiler-rt arch lists and lldb ArchSpec/ABI CMake tables move often: take upstream, re-add M88k.
- Process: after a scripted conflict resolution check `grep -c '^<<<<<<<'` before `git add` (rerere.autoupdate is on; a failed script once committed markers).

Upstream API changes likely to recur: GISel (KnownBits -> ValueTracking, TPC args, buildObjectPtrOffset), MC (MCSpecifierExpr, fixup/reloc hooks, MCAsmLexer), TargetLowering/InstrInfo/RegisterInfo hook signatures, RuntimeLibcalls and DataLayout moving into tablegen/Triple-keyed code (m88k must be listed in each), clang Options.h/CommonArgs.h header moves, lldb ABI plugin return types.

Test expectation changes:
- MC/M88k/gnu-syntax.s: CHECK is now `65535&(foo-.Ltmp0-4)` (upstream 04a67528d303 prints fewer parentheses). No other test changed.

Fixes beyond API renames (each a separate commit):
- (A) MC/M88k/gnu-syntax.s: CHECK text now `65535&(foo-.Ltmp0-4)`; upstream 04a67528d303 prints fewer parentheses. Encodings unchanged.
- (B) Triple::getDefaultExceptionHandling had no m88k case (SjLj); upstream 1121034dd1c4 asserts MCAsmInfo matches Triple.
- (B) RuntimeLibcalls.td: m88k added to isDefaultLibcallArch (libcalls moved to tablegen, upstream b2f0ffd659e7).
- (B) TargetDataLayout.cpp computeDataLayout needs an m88k case (same string as M88kTargetMachine and clang).
- (B) clang BareMetal now derives Generic_ELF and defaults to system ld; m88k keeps ld.lld via getDefaultLinker and gets no -m option.
- (B) M88kTargetLowering::isCheapToSpeculateCttz = true; upstream becd418626e1 changed bitcount-odd/bitcount64 codegen, now byte-identical to LLVM 21.
- (B) lldb ABISysV_m88k: UnwindPlanSP return types, Expected GetByteSize, IsFloatingPointType(is_complex).
- (process) scripts/m88k-baseline.sh: `cmake --install --component builtins` is a silent no-op in 22; use plain `cmake --install`. LLDB for the port builds in <work_dir>/build-lldb (X86 plus experimental M88k, no python/libedit/curses).

Next port should: run the baseline on the new tag first and grep for m88k in Triple.cpp, RuntimeLibcalls.td, TargetDataLayout.cpp and BareMetal for new per-arch switches; expect the lldb build in <work_dir>/build-lldb; keep the cttz-style codegen diffs byte-identical via target hooks rather than editing expectations.

## Status notes

- codeberg push pending: needs user auth (repo exists but is empty; HTTPS push hangs on the credential-manager GUI prompt). github has all four branches and pre-llvm22/* tags.
