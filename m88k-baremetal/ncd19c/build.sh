#!/bin/bash
# Usage: build.sh [--no-fpx] NAME|FILE.c   (builds tests/NAME.c or FILE.c -> out/NAME.elf, out/NAME.coff)
# Tests named libc-*, val* or fpx-real link against picolibc (PICOLIBC, default
# /home/mk2s/Documents/picolibc/install-m88k) and the compiler-rt builtins.
# The SFU1 FP completion handler (ncd19c_fpx.[Sc]) is linked and installed by crt0 by default: the
# MC88100 traps trnc/int/nint for |x| >= 2^30 and leaves NaN/denormal cases to software.
# --no-fpx (or NO_FPX=1) leaves it out; tests named fpx-* always link it. --fpx is accepted, a no-op.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
LLVM="${LLVM:-/home/mk2s/Documents/llvm-m88k}"
CPU="${M88K_CPU:-mc88100}"
CC="$LLVM/build/bin/clang --target=m88k-unknown-none-elf -mcpu=$CPU"
CFLAGS="-O2 -ffreestanding -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -fno-crash-diagnostics"
FPX=1
[ "${NO_FPX:-0}" = 1 ] && FPX=0
case "${1:-}" in --fpx) FPX=1; shift;; --no-fpx) FPX=0; shift;; esac
NAME="${1:?usage: build.sh [--no-fpx] NAME|FILE.c}"
SRC="$HERE/tests/$NAME.c"
case "$NAME" in *.c) SRC="$(cd "$(dirname "$NAME")" && pwd)/$(basename "$NAME")"; NAME="$(basename "$NAME" .c)";; esac
OUT="$HERE/out"; mkdir -p "$OUT"
$CC $CFLAGS -c "$HERE/crt0.S" -o "$OUT/crt0.o"
$CC $CFLAGS -c "$HERE/ncd19c_console.c" -o "$OUT/ncd19c_console.o"
LIBC=0
case "$NAME" in libc-*|val*|fpx-real) LIBC=1;; esac
case "$NAME" in fpx-*) FPX=1;; esac
FPXOBJ=""
if [ "$FPX" = 1 ]; then
  $CC $CFLAGS -I"$HERE" -c "$HERE/ncd19c_fpx.S" -o "$OUT/ncd19c_fpx_s.o"
  $CC $CFLAGS -I"$HERE" -c "$HERE/ncd19c_fpx.c" -o "$OUT/ncd19c_fpx_c.o"
  FPXOBJ="$OUT/ncd19c_fpx_s.o $OUT/ncd19c_fpx_c.o"
fi
if [ "$LIBC" = 1 ]; then
  PICOLIBC="${PICOLIBC:-/home/mk2s/Documents/picolibc/install-m88k}"
  $CC $CFLAGS -isystem "$PICOLIBC/include" -I"$HERE" -c "$HERE/ncd19c_stdio.c" -o "$OUT/ncd19c_stdio.o"
  $CC $CFLAGS -isystem "$PICOLIBC/include" -I"$HERE" -c "$SRC" -o "$OUT/$NAME.o"
  EXTRA="$OUT/ncd19c_stdio.o"
  LIBS="$PICOLIBC/lib/libc.a"
else
  $CC $CFLAGS -I"$HERE" -c "$SRC" -o "$OUT/$NAME.o"
  EXTRA=""
  LIBS=""
fi
# compiler-rt builtins (64 bit mul/div/rem, int <-> FP conversions, ...).
RTLIB="$($CC -rtlib=compiler-rt -print-libgcc-file-name)"
$LLVM/build/bin/ld.lld -T "$HERE/ncd19c.ld" --gc-sections -o "$OUT/$NAME.elf" \
  "$OUT/crt0.o" "$OUT/ncd19c_console.o" $FPXOBJ $EXTRA "$OUT/$NAME.o" $LIBS "$RTLIB"
python3 "$HERE/elf2ncd.py" "$OUT/$NAME.elf" "$OUT/$NAME.coff"
