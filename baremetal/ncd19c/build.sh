#!/bin/bash
# Usage: build.sh NAME   (builds tests/NAME.c -> out/NAME.elf, out/NAME.coff)
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
LLVM="${LLVM:-/home/mk2s/Documents/llvm-m88k}"
CPU="${M88K_CPU:-mc88100}"
CC="$LLVM/build/bin/clang --target=m88k-unknown-none-elf -mcpu=$CPU"
CFLAGS="-O2 -ffreestanding -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections"
NAME="${1:?usage: build.sh NAME}"
OUT="$HERE/out"; mkdir -p "$OUT"
$CC $CFLAGS -c "$HERE/crt0.S" -o "$OUT/crt0.o"
$CC $CFLAGS -c "$HERE/ncd19c_console.c" -o "$OUT/ncd19c_console.o"
$CC $CFLAGS -I"$HERE" -c "$HERE/tests/$NAME.c" -o "$OUT/$NAME.o"
$LLVM/build/bin/ld.lld -T "$HERE/ncd19c.ld" --gc-sections -o "$OUT/$NAME.elf" \
  "$OUT/crt0.o" "$OUT/ncd19c_console.o" "$OUT/$NAME.o"
python3 "$HERE/elf2ncd.py" "$OUT/$NAME.elf" "$OUT/$NAME.coff"
