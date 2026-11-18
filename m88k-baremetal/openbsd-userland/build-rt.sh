#!/bin/bash
# Builds the compiler-rt builtins for m88k-unknown-openbsd into build-rt-m88k-openbsd and installs
# them in clang's resource dir (lib/m88k-unknown-openbsd/libclang_rt.builtins.a). The bare-metal
# library (m88k-unknown-none-elf) must not be used for OpenBSD programs: it allocates r26/r27,
# and r27 is OpenBSD's thread pointer (libc calls the executable's __udivdi3 etc. when linked
# dynamically). Env: LLVM, JOBS (default 6).
set -euo pipefail
LLVM="${LLVM:-/home/mk2s/Documents/llvm-m88k}"
BIN="$LLVM/build/bin"
T=m88k-unknown-openbsd
RES="$($BIN/clang -print-resource-dir)"
[ -f "$LLVM/build-rt-m88k-openbsd/build.ninja" ] || cmake -G Ninja -S "$LLVM/compiler-rt" -B "$LLVM/build-rt-m88k-openbsd" \
  -DCMAKE_C_COMPILER="$BIN/clang" -DCMAKE_CXX_COMPILER="$BIN/clang++" -DCMAKE_ASM_COMPILER="$BIN/clang" \
  -DCMAKE_AR="$BIN/llvm-ar" -DCMAKE_NM="$BIN/llvm-nm" -DCMAKE_RANLIB="$BIN/llvm-ranlib" \
  -DCMAKE_C_COMPILER_TARGET=$T -DCMAKE_CXX_COMPILER_TARGET=$T -DCMAKE_ASM_COMPILER_TARGET=$T \
  -DCMAKE_C_FLAGS="-mcpu=mc88100 -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-crash-diagnostics -fno-stack-protector" \
  -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY -DCMAKE_BUILD_TYPE=Release \
  -DCOMPILER_RT_BUILTINS_ENABLE_PIC=OFF -DCOMPILER_RT_BAREMETAL_BUILD=ON \
  -DLLVM_ENABLE_PER_TARGET_RUNTIME_DIR=ON -DCOMPILER_RT_DEFAULT_TARGET_ONLY=ON \
  -DCOMPILER_RT_BUILD_BUILTINS=ON -DCOMPILER_RT_BUILD_SANITIZERS=OFF -DCOMPILER_RT_BUILD_XRAY=OFF \
  -DCOMPILER_RT_BUILD_LIBFUZZER=OFF -DCOMPILER_RT_BUILD_PROFILE=OFF -DCOMPILER_RT_BUILD_MEMPROF=OFF \
  -DCOMPILER_RT_BUILD_ORC=OFF -DCOMPILER_RT_BUILD_CTX_PROFILE=OFF -DCOMPILER_RT_BUILD_CRT=OFF \
  -DLLVM_CONFIG_PATH="$BIN/llvm-config" -DCMAKE_INSTALL_PREFIX="$RES"
ninja -C "$LLVM/build-rt-m88k-openbsd" -j"${JOBS:-6}" install
