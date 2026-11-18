# Review: m88k-llvm-port recipe, baseline scripts, PORTING.md (second pass)

Verdict: APPROVED. No blockers.

## Prior findings
- Dry-run loops could not finish (gate files never written): fixed. `setup` writes `replay-state.json` (DONE), `build-review.json` (APPROVED) and `triage-state.json` (PASS) with `dry_run:true` when dry_run is set, and writes none otherwise. All three `repeat` stop conditions read exactly those files and paths.
- Raw `{{repo_dir}}` etc. expanding to empty: fixed. Only `setup` uses raw inputs, with explicit defaults. Every later step reads `{{setup.output}}/run.json`, which setup resolves. `setup` returns only the run dir path.

## Checked
- Recipe is valid JSON (keys name, description, inputs, steps, planRevision). Parameters include target_tag, source_ref, stack_order, build_dir, baseline_dir, plus repo_dir, upstream_dir, old_base, release_branch, work_dir, baseline_build_dir, dry_run.
- Phases: preflight/backup (refs/backup plus verified bundle, skipped on dry run), baseline, worktree on release/NN.x-m88k from the tag, `rebase --onto` per segment with rerere, build-fix loop, verify-run, triage loop, report.
- Loops are bounded (6, 8, 5) with `onMaxIterations: pause` and one gate file each. Paths are absolute (`git -C`, `{{setup.output}}`, defaults under /home/mk2s).
- Build-fix commits each fix as `adapt to LLVM <llvm_major>: ...` and appends a row to adaptations.log. A 3-iteration no-progress rule stops it. build-check verifies commit/log correspondence.
- Triage pauses with send_message warning for class C, anything altering m88k codegen/ABI, and any bare-metal difference. It forbids XFAIL or disabling tests.
- Report appends to PORTING.md (read first), commits it on the release branch, and writes PORTING-proposed.md on dry run.
- No hardcoded LLVM version in the recipe. "22" appears only as examples. 4590f755 appears only in PORTING.md as a documented first-port default.
- `bash -n` passes on both scripts. Reading baseline.sh: step logic, results.txt format (sorted, no timestamps, `_step ERROR` marker), `--background` re-exec and env passing are consistent. The diff script's awk classifies new failures, new tests failing, missing, fixed, changed and added. Its exit code is 1 on failures or missing tests. Both match their help text.
- PORTING.md has process, parameters, adaptation log (seeded), a dry-run section and an untested list. The coder's recorded evidence (lit step on LLVM 21 build: 101 PASS, diff on samples, bash -n, JSON validity) matches what the file claims. I did not re-run anything.

## Dry-run validation (open point)
Reasonable and sufficient as the next step. The dry-run covers input resolution, tag presence, tools, ancestry, conflict forecast and the lit step with a self-diff. The recipe has not been executed in the workflow engine. The untested list says so, as it does for parameter substitution, `fileCheck` and `captureOutput`. Running dry_run=true with only target_tag is the right first validation. Two engine behaviors could still surprise: whether reviewer steps with `artifacts` fail when they skip writing files on dry run, and how an unset input renders. Neither is established as a defect here.

## Non-blocking notes
- The `.gitignore` change (`.agents/`, no trailing newline) is outside the stated task but harmless.
- lldb, rt and baremetal steps remain untested and are marked as such.
