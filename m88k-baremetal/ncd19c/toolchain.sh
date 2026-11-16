#!/bin/bash
# Toolchain setup and end-to-end validation (tests/val.c) for the NCD 19c BSP.
# (build.sh in this directory builds a single test.)
# Usage: toolchain.sh [clang|rt|picolibc|prog|run|all]   (default: all)
# Env: LLVM, PICOLIBC (checkout, branch m88k), M88K_CPU (default mc88100),
#      NO_FPX=1 (leave the default SFU1 FP completion handler out of val), JOBS (default 2), CAP (memory-cap wrapper, default /home/mk2s/bin/capped if present).
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
TESTS="$HERE/tests"
LLVM="${LLVM:-/home/mk2s/Documents/llvm-m88k}"
PICO="${PICOLIBC:-/home/mk2s/Documents/picolibc}"
CPU="${M88K_CPU:-mc88100}"
JOBS="${JOBS:-2}"
CAP="${CAP-/home/mk2s/bin/capped}"
BSP="$HERE"
BIN="$LLVM/build/bin"

step_clang() {   # 1. clang + lld + tools (M88k is an experimental target)
  [ -f "$LLVM/build/build.ninja" ] || cmake -G Ninja -S "$LLVM/llvm" -B "$LLVM/build" \
    -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS='clang;lld' \
    -DLLVM_TARGETS_TO_BUILD= -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=M88k \
    -DLLVM_PARALLEL_COMPILE_JOBS=4 -DLLVM_PARALLEL_LINK_JOBS=1
  $CAP ninja -C "$LLVM/build" -j"$JOBS" clang lld llc llvm-mc llvm-ar llvm-nm llvm-ranlib \
    llvm-objdump llvm-readelf llvm-strip llvm-config FileCheck not
}
step_rt() {      # 2. compiler-rt builtins (64-bit mul/div, FP helpers), installed in clang's resource dir
  local RES; RES="$($BIN/clang -print-resource-dir)"
  [ -f "$LLVM/build-rt-m88k/build.ninja" ] || cmake -G Ninja -S "$LLVM/compiler-rt" -B "$LLVM/build-rt-m88k" \
    -DCMAKE_C_COMPILER="$BIN/clang" -DCMAKE_CXX_COMPILER="$BIN/clang++" -DCMAKE_ASM_COMPILER="$BIN/clang" \
    -DCMAKE_AR="$BIN/llvm-ar" -DCMAKE_NM="$BIN/llvm-nm" -DCMAKE_RANLIB="$BIN/llvm-ranlib" \
    -DCMAKE_C_COMPILER_TARGET=m88k-unknown-none-elf -DCMAKE_CXX_COMPILER_TARGET=m88k-unknown-none-elf \
    -DCMAKE_ASM_COMPILER_TARGET=m88k-unknown-none-elf \
    -DCMAKE_C_FLAGS="-mcpu=$CPU -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-crash-diagnostics" \
    -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY -DCMAKE_BUILD_TYPE=Release \
    -DCOMPILER_RT_BUILTINS_ENABLE_PIC=OFF -DCOMPILER_RT_BAREMETAL_BUILD=ON \
    -DLLVM_ENABLE_PER_TARGET_RUNTIME_DIR=ON -DCOMPILER_RT_DEFAULT_TARGET_ONLY=ON \
    -DCOMPILER_RT_BUILD_BUILTINS=ON -DCOMPILER_RT_BUILD_SANITIZERS=OFF -DCOMPILER_RT_BUILD_XRAY=OFF \
    -DCOMPILER_RT_BUILD_LIBFUZZER=OFF -DCOMPILER_RT_BUILD_PROFILE=OFF -DCOMPILER_RT_BUILD_MEMPROF=OFF \
    -DCOMPILER_RT_BUILD_ORC=OFF -DCOMPILER_RT_BUILD_CTX_PROFILE=OFF -DCOMPILER_RT_BUILD_CRT=OFF \
    -DLLVM_CONFIG_PATH="$BIN/llvm-config" -DCMAKE_INSTALL_PREFIX="$RES"
  $CAP ninja -C "$LLVM/build-rt-m88k" -j"$JOBS" install
}
step_picolibc() { # 3. picolibc (branch m88k), installed to $PICO/install-m88k
  [ -f "$PICO/build-m88k/build.ninja" ] || (cd "$PICO" && meson setup build-m88k \
    --cross-file scripts/cross-clang-m88k-unknown-none-elf.txt -Dmultilib=false -Dpicocrt=false \
    -Dsemihost=false -Dtests=false -Dsingle-thread=true -Dthread-local-storage=false \
    -Datomic-ungetc=false -Dio-long-long=true -Dnewlib-global-errno=true -Dspecsdir=none \
    --buildtype=minsize --prefix="$PICO/install-m88k")
  $CAP ninja -C "$PICO/build-m88k" -j"$JOBS" install
}
step_prog() {    # 4. build the validation program -> BSP out/val.coff
  PICOLIBC="$PICO/install-m88k" M88K_CPU="$CPU" LLVM="$LLVM" $CAP "$BSP/build.sh" "$TESTS/val.c"
}
step_run() {     # 5. run in MAME (-window is hard-coded in the runner), compare with val.expected
  "$BSP/run-ncd19c.py" "$BSP/out/val.coff" --shot "$BSP/out/val.png" > "$BSP/out/val.serial"
  tr -d '\004' < "$BSP/out/val.serial" | diff - "$TESTS/val.expected" && echo "VAL OK: MAME output matches val.expected"
}
case "${1:-all}" in
  clang) step_clang;; rt) step_rt;; picolibc) step_picolibc;; prog) step_prog;; run) step_run;;
  all) step_clang; step_rt; step_picolibc; step_prog; step_run;;
  *) echo "usage: toolchain.sh [clang|rt|picolibc|prog|run|all]" >&2; exit 2;;
esac
