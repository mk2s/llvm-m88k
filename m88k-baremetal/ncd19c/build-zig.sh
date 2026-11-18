#!/bin/bash
# Usage: build-zig.sh [NAME]   (builds zig/NAME.zig -> out/zig-NAME.elf, out/zig-NAME.coff; default NAME=hello)
# Zig emits C (-ofmt=c) for a stand-in target and the m88k clang compiles it. Zig has no m88k target;
# mips-freestanding (32-bit big-endian, 8-byte long double) satisfies zig.h's ABI static asserts.
# (powerpc fails on long double/f128 size, arc on 64-bit alignment.) The .zig file must
# `export fn zig_main() c_int`; zig/main.c calls it from main().
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
LLVM="${LLVM:-/home/mk2s/Documents/llvm-m88k}"
CPU="${M88K_CPU:-mc88100}"
ZIG="${ZIG:-zig}"
ZTARGET="${ZIG_STAND_IN_TARGET:-mips-freestanding}"
NAME="${1:-hello}"
OUT="$HERE/out"; mkdir -p "$OUT"
ZLIB="$($ZIG env | sed -n 's/.*\.lib_dir = "\(.*\)",/\1/p')"
CC="$LLVM/build/bin/clang --target=m88k-unknown-none-elf -mcpu=$CPU"
CFLAGS="-O2 -ffreestanding -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -fno-crash-diagnostics"
$ZIG build-obj -target "$ZTARGET" -ofmt=c -O ReleaseSmall -fno-stack-check \
  -femit-bin="$OUT/zig-$NAME.c" "$HERE/zig/$NAME.zig"
$CC $CFLAGS -I"$ZLIB" -c "$OUT/zig-$NAME.c" -o "$OUT/zig-$NAME.o"
$CC $CFLAGS -c "$HERE/zig/main.c" -o "$OUT/zig-main.o"
$CC $CFLAGS -c "$HERE/crt0.S" -o "$OUT/crt0.o"
$CC $CFLAGS -c "$HERE/ncd19c_console.c" -o "$OUT/ncd19c_console.o"
$CC $CFLAGS -I"$HERE" -c "$HERE/ncd19c_fpx.S" -o "$OUT/ncd19c_fpx_s.o"
$CC $CFLAGS -I"$HERE" -c "$HERE/ncd19c_fpx.c" -o "$OUT/ncd19c_fpx_c.o"
RTLIB="$($CC -rtlib=compiler-rt -print-libgcc-file-name)"
$LLVM/build/bin/ld.lld -T "$HERE/ncd19c.ld" --gc-sections -o "$OUT/zig-$NAME.elf" \
  "$OUT/crt0.o" "$OUT/ncd19c_console.o" "$OUT/ncd19c_fpx_s.o" "$OUT/ncd19c_fpx_c.o" \
  "$OUT/zig-main.o" "$OUT/zig-$NAME.o" "$RTLIB"
python3 "$HERE/elf2ncd.py" "$OUT/zig-$NAME.elf" "$OUT/zig-$NAME.coff"
