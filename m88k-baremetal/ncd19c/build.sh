#!/bin/bash
# Usage: build.sh NAME   (builds tests/NAME.c -> out/NAME.elf, out/NAME.coff)
# Tests named libc-* or val* link against picolibc (PICOLIBC, default
# /home/mk2s/Documents/picolibc/install-m88k) and the compiler-rt builtins.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
LLVM="${LLVM:-/home/mk2s/Documents/llvm-m88k}"
CPU="${M88K_CPU:-mc88100}"
CC="$LLVM/build/bin/clang --target=m88k-unknown-none-elf -mcpu=$CPU"
CFLAGS="-O2 -ffreestanding -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -fno-crash-diagnostics"
NAME="${1:?usage: build.sh NAME}"
OUT="$HERE/out"; mkdir -p "$OUT"
$CC $CFLAGS -c "$HERE/crt0.S" -o "$OUT/crt0.o"
$CC $CFLAGS -c "$HERE/ncd19c_console.c" -o "$OUT/ncd19c_console.o"
LIBC=0
case "$NAME" in libc-*|val*) LIBC=1;; esac
if [ "$LIBC" = 1 ]; then
  PICOLIBC="${PICOLIBC:-/home/mk2s/Documents/picolibc/install-m88k}"
  $CC $CFLAGS -isystem "$PICOLIBC/include" -I"$HERE" -c "$HERE/ncd19c_stdio.c" -o "$OUT/ncd19c_stdio.o"
  $CC $CFLAGS -isystem "$PICOLIBC/include" -I"$HERE" -c "$HERE/tests/$NAME.c" -o "$OUT/$NAME.o"
  EXTRA="$OUT/ncd19c_stdio.o"
  LIBS="$PICOLIBC/lib/libc.a"
else
  $CC $CFLAGS -I"$HERE" -c "$HERE/tests/$NAME.c" -o "$OUT/$NAME.o"
  EXTRA=""
  LIBS=""
fi
# compiler-rt builtins (64 bit mul/div/rem, int <-> FP conversions, ...).
RTLIB="$($CC -rtlib=compiler-rt -print-libgcc-file-name)"
$LLVM/build/bin/ld.lld -T "$HERE/ncd19c.ld" --gc-sections -o "$OUT/$NAME.elf" \
  "$OUT/crt0.o" "$OUT/ncd19c_console.o" $EXTRA "$OUT/$NAME.o" $LIBS "$RTLIB"
python3 "$HERE/elf2ncd.py" "$OUT/$NAME.elf" "$OUT/$NAME.coff"
