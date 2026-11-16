# Pre-existing failures: CodeGen/M88k/add.ll and branch.ll

Final status: `llvm-lit llvm/test/CodeGen/M88k llvm/test/MC/M88k` → 63/63 pass (before: add.ll and branch.ll failed).
Both were fixed in one commit (see "Fix").

## branch.ll
(a) Valid. The CHECK lines (`subu %r2, %r4, 1`) are correct m88k code for `switch (op-1)`. Last updated 2023-12 (e4e7aa5aba81); it passed then. It broke later in the upstream author's own history, not in this work.
(b) Worth keeping. It exercises jump-table lowering and indirect `jmp` at -O0 for both mc88100 and mc88110.
(c) Root cause was the same as add.ll f3 (below). Fix size: 3 lines in M88kInstrInfo.td. No test change was needed.

## add.ll
Three failing places, two root causes.

### f3: `sub i32 %a, 512` expected `subu %r2, %r2, 512`
(a) Expectation is correct and was passing at 0877291fdda1 (2024-02-07). The legalizer rule for G_ADD/G_SUB (M88kLegalizerInfo.cpp) combined with generic GISel combines turns `G_SUB x, C` into `G_ADD x, -C`. After the author's later change "Move more code generation to TableGen patterns" (a6baece63e27, 2024-03-27), the only imm pattern for `add` took `uimm16`, so `-512` was materialised as `subu r3, r0, 512` and added with `addu`. Two instructions instead of one: a codegen regression (worse code), not noise.
(c) Fix: add `(add rs1, simm16Neg:$imm) -> SUBUri rs1, NEG16(imm)` in M88kInstrInfo.td (3 lines). It also gives `x - 1` for `add x, -1` (branch.ll, bitcount64 cttz).

### f8 and f16: `usub.with.overflow(0, a)` borrow added to b
(a) The old expectation was NOT correct m88k code. It copied the hardware carry with `addu.ci r,r0,r0`. On the 88k the carry after `subu.co` means "no borrow" (a==0 for 0-a), so the old code computed b + (a==0) instead of b + (a!=0). Commit da4a318597c1 (this repo, compiler-rt work) changed the copy to `subu.ci r,r0,r0`, which gives -1 for a borrow and 0 otherwise. That is the borrow in bit 0 only. The users of the generic carry-out (a `G_ADD` of the zext'd borrow, or `usubo` returned as i1) got -1 instead of 1, so f8 would have returned b-1 on a borrow. That is a miscompile, introduced in this repo and then baked into int-ops-lower.ll and the cttz CHECKs in bitcount64.ll.
(b) Keep: covers carry/borrow lowering in the post-legalizer combiner and selector, which is delicate code.
(c) Fix: in M88kInstructionSelector.cpp `selectAddSubWithCarry`, for a subtract negate the `subu.ci` result (`subu r,r0,t`) so the carry-out register is 0/1 (about 15 lines). Cost: one extra instruction when a borrow is used as a value. Carry-in/combined paths (`subu.co`/`subu.ci` chains) are unchanged.

## Fix
Files: M88kInstrInfo.td, GISel/M88kInstructionSelector.cpp. CHECK lines regenerated with update_llc_test_checks.py for add.ll (f8, f16), int-ops-lower.ll (usubsat, usubo) and bitcount64.ll (cttz, now `subu r,r,1` instead of `subu r,r0,1`+`addu`). Each diff was reviewed: the f3/branch/cttz changes are improvements, and the borrow changes are correctness fixes that add one instruction.
Not run: an end-to-end execution check of f8 on the emulator.
