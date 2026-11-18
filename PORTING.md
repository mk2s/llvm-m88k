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

Nothing past the dry-run checks has been run. As of this file's creation:

- Tested by hand: `m88k-baseline.sh --only lit` on the LLVM 21-based build (101 results, all PASS), `m88k-baseline-diff.sh` on sample files, `bash -n` on both scripts, JSON validity of the recipe.
- Untested: compiler-rt reconfiguration when the resource dir changes (the `lldb`, `rt` build/install and `baremetal` steps ran on 2026-11-18, see the baseline section); the whole recipe in the workflow engine (parameter substitution, `fileCheck` stop conditions, `captureOutput`); worktree and per-segment replay; the build-fix loop; verify, triage and report steps; the baremetal expectation that `fpx-real` differs from `.expected` in two known NaN lines (it will show as FAIL in the baseline, which is correct, because only differences are reported).
- Not available here: `shellcheck`.
- Known gap: `lldb` has no lit tests for M88k. The lldb step only checks that the binary starts and lists the M88k ABI plugin.

Remove an item from this list only when a run exercised it.

## Adaptation log

One section per release, added by the `report` step. Each row: commit, files, upstream API change, upstream commit.
Read the earlier sections before starting a port: the same breakage often returns in a different form.

### LLVM 22.1 (llvmorg-22.1.8)

Seeded, not yet run. Starting point: stack forked from upstream `4590f755cf58` (6 Feb 2025, LLVM 21 development),
about 21,000 upstream commits behind the tag; the stack has 397 commits (`main-m88k` 369, +2, +2, +24; counts from `git rev-list --count`).

| Commit | Files | Upstream API change | Upstream commit |
|---|---|---|---|
| (none yet) | | | |

Conflicts and resolutions: (none yet)

Test expectation changes: (none yet)

Lessons for the next port: (none yet)

## Status notes

- codeberg push pending: needs user auth (repo exists but is empty; HTTPS push hangs on the credential-manager GUI prompt). github has all four branches and pre-llvm22/* tags.
