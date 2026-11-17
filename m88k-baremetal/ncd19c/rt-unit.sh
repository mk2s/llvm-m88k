#!/bin/bash
# Usage: rt-unit.sh [-O<n>] [TEST ...]
# Builds compiler-rt's test/builtins/Unit/*_test.c for the NCD 19c BSP, links them into one image
# (out/rt-unit/rt-unit.coff) with a small runner that prints "PASS name" / "FAIL name rc" per
# test, and (if RUN=1) runs it in MAME. Tests that need 128-bit integers, long double (tf/xf),
# ARM VFP or another architecture are skipped. Compile failures are listed in out/rt-unit/skipped.txt.
# Env: LLVM, PICOLIBC, M88K_CPU as in build.sh; JOBS (default 4); OPT (default -O2); RUN=1 runs MAME.
set -uo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
LLVM="${LLVM:-/home/mk2s/Documents/llvm-m88k}"
CPU="${M88K_CPU:-mc88100}"
PICOLIBC="${PICOLIBC:-/home/mk2s/Documents/picolibc/install-m88k}"
OPT="${OPT:--O2}"
JOBS="${JOBS:-4}"
UNIT="$LLVM/compiler-rt/test/builtins/Unit"
BI="$LLVM/compiler-rt/lib/builtins"
OUT="$HERE/out/rt-unit"
mkdir -p "$OUT/obj"
CC="$LLVM/build/bin/clang --target=m88k-unknown-none-elf -mcpu=$CPU"
CFLAGS="$OPT -ffreestanding -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -fno-crash-diagnostics -w"

if [ $# -gt 0 ]; then
  TESTS=("$@")
else
  TESTS=()
  for f in "$UNIT"/*_test.c; do
    n=$(basename "$f" _test.c)
    case "$n" in
      mulsi3|*ti|*ti[0-9]|*tf|*tf[0-9]|*xf|*xf[0-9]|*tc3|*xc3|*vfp|*[a-z]l|aarch64*|cpu_model|trampoline_setup|clear_cache|enable_execute_stack|gcc_personality|atomic|*hf*|*tf*|*xf*|*ti*|*ti[0-9]) continue;;
    esac
    grep -qE "^// *(REQUIRES: (int128|x86-target-arch|librt_has_comparetf2|native-run)|UNSUPPORTED: .*m88k|XFAIL: \*)" "$f" && continue
    TESTS+=("$n")
  done
fi

compile_one() {
  n="$1"
  src="$UNIT/${n}_test.c"
  if $CC $CFLAGS -I"$BI" -I"$UNIT" -isystem "$PICOLIBC/include" -Dmain="rt_${n}_main" \
        -c "$src" -o "$OUT/obj/$n.o" 2> "$OUT/obj/$n.err"; then
    rm -f "$OUT/obj/$n.err"
    # make everything but the renamed main local so the tests' globals (cases, assumption_1, ...) do not clash
    "$LLVM/build/bin/llvm-objcopy" --keep-global-symbol="rt_${n}_main" "$OUT/obj/$n.o" || rm -f "$OUT/obj/$n.o"
  else
    rm -f "$OUT/obj/$n.o"
  fi
}
export -f compile_one; export LLVM CC CFLAGS BI UNIT PICOLIBC OUT
printf '%s\n' "${TESTS[@]}" | xargs -P "$JOBS" -I{} bash -c 'compile_one {}'

OK=(); : > "$OUT/skipped.txt"
for n in "${TESTS[@]}"; do
  if [ -f "$OUT/obj/$n.o" ]; then OK+=("$n"); else echo "$n (compile)" >> "$OUT/skipped.txt"; fi
done
[ -f "$OUT/exclude.txt" ] && while read -r x; do
  OK=("${OK[@]/$x}"); done < "$OUT/exclude.txt"

{
  echo '#include <stdio.h>'
  echo '#include "../../ncd19c_console.h"'
  for n in "${OK[@]}"; do [ -n "$n" ] && echo "int rt_${n}_main(void);"; done
  echo 'struct t { const char *name; int (*fn)(void); };'
  echo 'static const struct t tests[] = {'
  for n in "${OK[@]}"; do [ -n "$n" ] && echo "  {\"$n\", rt_${n}_main},"; done
  echo '};'
  cat <<'EOF'
int main(void) {
  unsigned pass = 0, fail = 0;
  for (unsigned i = 0; i < sizeof tests / sizeof *tests; ++i) {
    printf("RUN %s\n", tests[i].name);
    int rc = tests[i].fn();
    if (rc == 0) { ++pass; printf("PASS %s\n", tests[i].name); }
    else { ++fail; printf("FAIL %s rc=%d\n", tests[i].name, rc); }
  }
  printf("SUMMARY pass=%u fail=%u\n", pass, fail);
  return fail != 0;
}
EOF
} > "$OUT/runner.c"

$CC $CFLAGS -c "$HERE/crt0.S" -o "$OUT/crt0.o"
$CC $CFLAGS -c "$HERE/ncd19c_console.c" -o "$OUT/ncd19c_console.o"
$CC $CFLAGS -I"$HERE" -c "$HERE/ncd19c_fpx.S" -o "$OUT/fpx_s.o"
$CC $CFLAGS -I"$HERE" -c "$HERE/ncd19c_fpx.c" -o "$OUT/fpx_c.o"
$CC $CFLAGS -isystem "$PICOLIBC/include" -I"$HERE" -c "$HERE/ncd19c_stdio.c" -o "$OUT/stdio.o"
$CC $CFLAGS -isystem "$PICOLIBC/include" -I"$HERE" -c "$OUT/runner.c" -o "$OUT/runner.o"
RTLIB="$($CC -rtlib=compiler-rt -print-libgcc-file-name)"
OBJS=(); for n in "${OK[@]}"; do [ -n "$n" ] && OBJS+=("$OUT/obj/$n.o"); done
"$LLVM/build/bin/ld.lld" -T "$HERE/ncd19c.ld" --gc-sections -o "$OUT/rt-unit.elf" \
  "$OUT/crt0.o" "$OUT/ncd19c_console.o" "$OUT/fpx_s.o" "$OUT/fpx_c.o" "$OUT/stdio.o" "$OUT/runner.o" \
  "${OBJS[@]}" "$PICOLIBC/lib/libc.a" "$PICOLIBC/lib/libm.a" "$RTLIB" 2> "$OUT/link.err" || { echo "link failed: $OUT/link.err"; head -20 "$OUT/link.err"; exit 1; }
python3 "$HERE/elf2ncd.py" "$OUT/rt-unit.elf" "$OUT/rt-unit.coff"
echo "built ${#OBJS[@]} tests; skipped: $(wc -l < "$OUT/skipped.txt") (see $OUT/skipped.txt)"
if [ "${RUN:-0}" = 1 ]; then
  "$HERE/run-ncd19c.py" "$OUT/rt-unit.coff" --expect SUMMARY --timeout-emu "${EMU_TIMEOUT:-600}" | tee "$OUT/run.log"
fi
