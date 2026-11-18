#!/bin/bash
# m88k-baseline.sh: record the state of the m88k port as a diffable results file.
#
# Usage: m88k-baseline.sh [--background] [--only STEP[,STEP...]] [--skip STEP[,STEP...]] BASELINE_DIR
#
# Steps (run in this order): lit, lldb, rt, baremetal
#   lit        llvm-lit over the M88k tests of llvm/clang/lld found with git ls-files
#   lldb       lit over any lldb tests mentioning m88k, plus a smoke check of the lldb binary
#   rt         configure (if needed), build and install compiler-rt builtins for each RT_TRIPLES entry
#   baremetal  build each ncd19c test, run it in MAME (window mode), compare with tests/NAME.expected
#
# Output in BASELINE_DIR:
#   results.txt   sorted (LC_ALL=C), one "<suite>/<name> <PASS|FAIL|SKIP|UNSUPPORTED|XFAIL|XPASS|...>"
#                 per line, no timestamps. This is what m88k-baseline-diff.sh compares.
#   meta.txt      commit, versions, environment (may contain timestamps; do not diff it)
#   logs/*.log    full output of each step
#
# Environment (defaults are read from the existing build directories):
#   M88K_SRC        source tree (default: the repository containing this script)
#   BUILD_DIR       LLVM build dir with clang/lld/llvm-lit (default: $M88K_SRC/build)
#   LLDB_BUILD_DIR  LLDB build dir (default: $M88K_SRC/build-lldb)
#   RT_TRIPLES      space separated compiler-rt targets (default: "m88k-unknown-none-elf m88k-unknown-openbsd")
#   RT_BUILD_PREFIX compiler-rt build dir prefix; dir is "$M88K_SRC/build-rt-<suffix>" with suffix
#                   "m88k" for none-elf and "m88k-openbsd" for openbsd (override per triple with
#                   RT_BUILD_DIR_<TRIPLE with - replaced by _>)
#   RT_DIR_SUFFIX   appended to the default compiler-rt build dir names (e.g. "-22" -> build-rt-m88k-22)
#   M88K_CPU        -mcpu for compiler-rt and bare-metal tests (default mc88100)
#   JOBS            parallelism (default: nproc)
#   MAME_PORT_BASE  first TCP port used by run-ncd19c.py (default 2340)
#   MAME_WALL       wall-clock limit in seconds per bare-metal test (default 300)
#   BAREMETAL_TESTS space separated test names (default: every tests/*.c)
#   RT_RECONFIGURE  1 = always re-run cmake for compiler-rt (done automatically when the
#                   install prefix no longer matches clang's resource dir)
#
# Long runs: use --background (writes logs/baseline.log and prints the pid), or
#   nohup scripts/m88k-baseline.sh DIR > DIR/logs/baseline.log 2>&1 &
# MAME always runs in window mode (run-ncd19c.py passes -window); it needs a display.
# A step that cannot run (missing tool, missing display) records "<suite>/_step ERROR" so the
# diff shows it instead of silently dropping tests.
set -uo pipefail
export LC_ALL=C

SELF="$(readlink -f "${BASH_SOURCE[0]}")"
SRC="${M88K_SRC:-$(cd "$(dirname "$SELF")/.." && pwd)}"
BUILD="${BUILD_DIR:-$SRC/build}"
LLDB_BUILD="${LLDB_BUILD_DIR:-$SRC/build-lldb}"
RT_TRIPLES="${RT_TRIPLES:-m88k-unknown-none-elf m88k-unknown-openbsd}"
RT_DIR_SUFFIX="${RT_DIR_SUFFIX:-}"
CPU="${M88K_CPU:-mc88100}"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"
PORT_BASE="${MAME_PORT_BASE:-2340}"
WALL="${MAME_WALL:-300}"
BM="$SRC/m88k-baremetal/ncd19c"

STEPS="lit lldb rt baremetal"
BACKGROUND=0
while [ $# -gt 0 ]; do
  case "$1" in
    --background) BACKGROUND=1; shift;;
    --only) STEPS="${2//,/ }"; shift 2;;
    --skip) for s in ${2//,/ }; do STEPS=" $STEPS "; STEPS="${STEPS// $s / }"; done; shift 2;;
    -h|--help) sed -n '2,/^set -uo/p' "$SELF" | sed '$d' | sed 's/^# \{0,1\}//'; exit 0;;
    --) shift; break;;
    -*) echo "unknown option: $1" >&2; exit 2;;
    *) break;;
  esac
done
OUTDIR="${1:?usage: m88k-baseline.sh [--background] [--only STEPS] [--skip STEPS] BASELINE_DIR}"
mkdir -p "$OUTDIR/logs" || exit 2
OUTDIR="$(cd "$OUTDIR" && pwd)"
LOGS="$OUTDIR/logs"
RAW="$OUTDIR/results.raw"

if [ "$BACKGROUND" = 1 ]; then
  nohup env M88K_SRC="$SRC" BUILD_DIR="$BUILD" LLDB_BUILD_DIR="$LLDB_BUILD" "$SELF" --only "${STEPS// /,}" "$OUTDIR" \
    > "$LOGS/baseline.log" 2>&1 &
  echo "started pid $! log $LOGS/baseline.log"
  exit 0
fi

: > "$RAW"
rec() { printf '%s %s\n' "$1" "$2" >> "$RAW"; }
log() { printf '[baseline] %s\n' "$*" >&2; }

# ---- meta -------------------------------------------------------------------
{
  echo "date=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "src=$SRC"
  echo "branch=$(git -C "$SRC" branch --show-current)"
  echo "commit=$(git -C "$SRC" rev-parse HEAD)"
  echo "build_dir=$BUILD"
  echo "clang=$("$BUILD/bin/clang" --version 2>&1 | head -1)"
  echo "llvm_version_major=$(sed -n 's/.*set(LLVM_VERSION_MAJOR \([0-9]*\)).*/\1/p' "$SRC/cmake/Modules/LLVMVersion.cmake" 2>/dev/null | head -1)"
  echo "steps=$STEPS"
} > "$OUTDIR/meta.txt"

# ---- lit helpers ------------------------------------------------------------
# run_lit LIT_BIN BUILD_DIR SUITE FILE... ; maps lit JSON results to "SUITE/<test> STATUS"
run_lit() {
  local lit="$1" bdir="$2" suite="$3"; shift 3
  local json="$LOGS/lit-$suite.json"
  rm -f "$json"
  "$lit" -j "$JOBS" -o "$json" "$@" > "$LOGS/lit-$suite.log" 2>&1
  if [ ! -s "$json" ]; then rec "$suite/_step" ERROR; return 1; fi
  python3 - "$json" "$suite" "$SRC" >> "$RAW" <<'PY'
import json, sys
d = json.load(open(sys.argv[1])); suite = sys.argv[2]; src = sys.argv[3].rstrip('/') + '/'
for t in d.get('tests', []):
    name = t['name']
    # "LLVM :: CodeGen/M88k/add.ll" -> "<suite>/LLVM::CodeGen/M88k/add.ll"
    name = name.replace(' :: ', '::').replace(' ', '_')
    print(f"{suite}/{name} {t['code']}")
PY
}

# M88k tests tracked in git, grouped by the lit tree that owns them.
m88k_tests() { # PREFIX
  git -C "$SRC" ls-files "$1" | grep -iE '(m88k|88k)' | grep -E '\.(ll|mir|s|S|c|cpp|test|txt|m|yaml)$' | grep -E "/test/" | sed "s#^#$SRC/#"
}

step_lit() {
  local lit="$BUILD/bin/llvm-lit"
  if [ ! -x "$lit" ]; then log "no $lit"; rec lit/_step ERROR; return; fi
  local files=()
  mapfile -t files < <( { m88k_tests llvm/test; m88k_tests clang/test; m88k_tests lld/test; } | sort -u)
  # directories named M88k hold all their tests; add them whole (covers files without m88k in the name)
  local dirs=()
  for d in llvm/test/CodeGen/M88k llvm/test/MC/M88k clang/test/CodeGen/M88k; do
    [ -d "$SRC/$d" ] && dirs+=("$SRC/$d")
  done
  log "lit: ${#files[@]} files + ${#dirs[@]} dirs"
  run_lit "$lit" "$BUILD" lit "${dirs[@]}" "${files[@]}"
}

step_lldb() {
  local lit="$LLDB_BUILD/bin/llvm-lit" lldb="$LLDB_BUILD/bin/lldb"
  if [ ! -x "$lldb" ]; then log "no $lldb"; rec lldb/_step ERROR; return; fi
  # smoke: binary starts and the M88k ABI plugin is linked in
  if "$lldb" -b -o version > "$LOGS/lldb-smoke.log" 2>&1; then rec lldb/smoke-version PASS; else rec lldb/smoke-version FAIL; fi
  if "$lldb" -b -o 'plugin list' >> "$LOGS/lldb-smoke.log" 2>&1 && grep -qi m88k "$LOGS/lldb-smoke.log"; then
    rec lldb/smoke-abi-plugin PASS
  else
    rec lldb/smoke-abi-plugin FAIL
  fi
  local files=()
  mapfile -t files < <(m88k_tests lldb/test)
  if [ ${#files[@]} -gt 0 ] && [ -x "$lit" ]; then run_lit "$lit" "$LLDB_BUILD" lldb "${files[@]}"; fi
}

# ---- compiler-rt ------------------------------------------------------------
rt_dir_for() { # TRIPLE
  local t="$1" var
  var="RT_BUILD_DIR_${t//-/_}"
  if [ -n "${!var:-}" ]; then echo "${!var}"; return; fi
  case "$t" in
    m88k-unknown-none-elf) echo "$SRC/build-rt-m88k$RT_DIR_SUFFIX";;
    m88k-unknown-openbsd)  echo "$SRC/build-rt-m88k-openbsd$RT_DIR_SUFFIX";;
    *) echo "$SRC/build-rt-${t//-/_}$RT_DIR_SUFFIX";;
  esac
}

step_rt() {
  local clang="$BUILD/bin/clang" prefix t d extra
  [ -x "$clang" ] || { log "no $clang"; rec rt/_step ERROR; return; }
  prefix="$("$clang" -print-resource-dir)"
  for t in $RT_TRIPLES; do
    d="$(rt_dir_for "$t")"
    extra=""
    case "$t" in *openbsd*) extra="-fno-stack-protector";; esac
    local cur=""
    [ -f "$d/CMakeCache.txt" ] && cur="$(sed -n 's/^CMAKE_INSTALL_PREFIX:PATH=//p' "$d/CMakeCache.txt")"
    if [ "${RT_RECONFIGURE:-0}" = 1 ] || [ "$cur" != "$prefix" ]; then
      log "rt $t: configure $d (prefix $prefix)"
      cmake -G Ninja -S "$SRC/compiler-rt" -B "$d" \
        -DCMAKE_C_COMPILER="$clang" -DCMAKE_CXX_COMPILER="$BUILD/bin/clang++" -DCMAKE_ASM_COMPILER="$clang" \
        -DCMAKE_AR="$BUILD/bin/llvm-ar" -DCMAKE_NM="$BUILD/bin/llvm-nm" -DCMAKE_RANLIB="$BUILD/bin/llvm-ranlib" \
        -DCMAKE_C_COMPILER_TARGET="$t" -DCMAKE_CXX_COMPILER_TARGET="$t" -DCMAKE_ASM_COMPILER_TARGET="$t" \
        -DCMAKE_C_FLAGS="-mcpu=$CPU -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-crash-diagnostics $extra" \
        -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY -DCMAKE_BUILD_TYPE=Release \
        -DCOMPILER_RT_BUILTINS_ENABLE_PIC=OFF -DCOMPILER_RT_BAREMETAL_BUILD=ON \
        -DLLVM_ENABLE_PER_TARGET_RUNTIME_DIR=ON -DCOMPILER_RT_DEFAULT_TARGET_ONLY=ON \
        -DCOMPILER_RT_BUILD_BUILTINS=ON -DCOMPILER_RT_BUILD_SANITIZERS=OFF -DCOMPILER_RT_BUILD_XRAY=OFF \
        -DCOMPILER_RT_BUILD_LIBFUZZER=OFF -DCOMPILER_RT_BUILD_PROFILE=OFF -DCOMPILER_RT_BUILD_MEMPROF=OFF \
        -DCOMPILER_RT_BUILD_ORC=OFF -DCOMPILER_RT_BUILD_CTX_PROFILE=OFF -DCOMPILER_RT_BUILD_CRT=OFF \
        -DLLVM_CONFIG_PATH="$BUILD/bin/llvm-config" -DCMAKE_INSTALL_PREFIX="$prefix" \
        > "$LOGS/rt-$t.configure.log" 2>&1 || { rec "rt/$t/configure FAIL"; continue; }
      rec "rt/$t/configure PASS"
    fi
    if cmake --build "$d" --target builtins -j "$JOBS" > "$LOGS/rt-$t.build.log" 2>&1; then
      rec "rt/$t/build PASS"
      if cmake --install "$d" --component builtins > "$LOGS/rt-$t.install.log" 2>&1 ||
         cmake --install "$d" > "$LOGS/rt-$t.install.log" 2>&1; then
        rec "rt/$t/install PASS"
      else
        rec "rt/$t/install FAIL"
      fi
    else
      rec "rt/$t/build FAIL"
    fi
  done
}

# ---- bare-metal (MAME, window mode) -----------------------------------------
step_baremetal() {
  if [ -z "${DISPLAY:-}" ] && [ -z "${WAYLAND_DISPLAY:-}" ]; then
    log "no display: MAME runs with -window and needs one"; rec baremetal/_step ERROR; return
  fi
  [ -x "$BUILD/bin/clang" ] || { rec baremetal/_step ERROR; return; }
  # build.sh/rt-unit.sh use "$LLVM/build"; point LLVM at a shim so BUILD_DIR may be anywhere.
  local shim="$OUTDIR/.llvm-shim"
  rm -rf "$shim"; mkdir -p "$shim"
  ln -s "$BUILD" "$shim/build"
  ln -s "$SRC/compiler-rt" "$shim/compiler-rt"
  local names=() n port="$PORT_BASE"
  if [ -n "${BAREMETAL_TESTS:-}" ]; then read -ra names <<< "$BAREMETAL_TESTS"
  else for f in "$BM"/tests/*.c; do names+=("$(basename "$f" .c)"); done; fi
  for n in "${names[@]}"; do
    local lg="$LOGS/bm-$n"
    if ! LLVM="$shim" M88K_CPU="$CPU" "$BM/build.sh" "$n" > "$lg.build.log" 2>&1; then
      rec "baremetal/$n BUILD_FAIL"; continue
    fi
    local exp="$BM/tests/$n.expected" args=(--port "$port" --wall "$WALL")
    [ "$n" = boot-hello ] && args+=(--expect "boot ok")   # no .expected; others without one only need EOT
    port=$((port + 1))
    "$BM/run-ncd19c.py" "$BM/out/$n.coff" "${args[@]}" > "$lg.out" 2> "$lg.err"
    local rc=$?
    if [ -f "$exp" ]; then
      # serial capture: strip CR, NUL and EOT before comparing
      if [ $rc -eq 0 ] && tr -d '\r\000\004' < "$lg.out" | diff -q - "$exp" > /dev/null; then rec "baremetal/$n PASS"
      elif [ $rc -ne 0 ]; then rec "baremetal/$n RUN_FAIL"
      else rec "baremetal/$n FAIL"; tr -d '\r\000\004' < "$lg.out" | diff - "$exp" > "$lg.diff" 2>&1; fi
    else
      if [ $rc -eq 0 ]; then rec "baremetal/$n PASS"; else rec "baremetal/$n RUN_FAIL"; fi
    fi
  done
}

for s in $STEPS; do
  case "$s" in
    lit|lldb|rt|baremetal) log "step $s"; "step_$s" 2>&1 | tee -a "$LOGS/step-$s.log" >&2 ;;
    *) echo "unknown step: $s" >&2; exit 2;;
  esac
done

sort -u "$RAW" > "$OUTDIR/results.txt"
rm -f "$RAW"
log "wrote $OUTDIR/results.txt ($(wc -l < "$OUTDIR/results.txt") results)"
