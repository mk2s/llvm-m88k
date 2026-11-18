#!/bin/bash
# Usage: run.sh [TEST ...]   (default: all tests in tests/ and tests/interop)
# Compiles programs with the in-tree clang + ld.lld (static against the OpenBSD 7.1 luna88k libc.a, and
# dynamic against libc.so through the clang driver, both from $OBSD) and, as the reference, with OpenBSD's own GCC 3.3.6 `cc -c` (run under run88k; linked with
# the same ld.lld because the guest's libgcc.a is truncated), runs them in run88k and compares the output. Clang is built at -O0 and -O2. tests/k*.c exercise known issues (llvm/lib/Target/M88k/KNOWN_ISSUES.md)
# and are reported as KNOWN, not as failures. tests/*interop*/{lib,main}.c are built with every combination of
# gcc/clang for the two halves and linked with lld (kinterop*: mixed builds are a known issue).
# Env: LLVM, OBSD (read-only OpenBSD root, default /mnt/obsd), RUN88K, WORK (default /tmp/m88k-ou).
set -uo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
LLVM="${LLVM:-/home/mk2s/Documents/llvm-m88k}"
OBSD="${OBSD:-/mnt/obsd}"
RUN88K="${RUN88K:-/home/mk2s/Documents/run88k/zig-out/bin/x86_64-linux/run88k}"
WORK="${WORK:-/tmp/m88k-ou}"
TMO="${TMO:-300}"
CLANG="$LLVM/build/bin/clang --target=m88k-unknown-openbsd --sysroot=$OBSD -fno-pic -w"
LLD="$LLVM/build/bin/ld.lld"
# compiler-rt built for m88k-unknown-openbsd by ./build-rt.sh: it must not touch r26/r27.
RT="$($LLVM/build/bin/clang --target=m88k-unknown-openbsd -rtlib=compiler-rt -print-libgcc-file-name)"
L="$OBSD/usr/lib"
mkdir -p "$WORK/bin" "$WORK/obj" "$WORK/out" "$WORK/lib"
# Dynamic links go through the clang driver. lld does not search versioned lib*.so.N.M names
# (OpenBSD's own linkers do), so provide unversioned links, and compiler-rt as -lcompiler_rt.
ln -sf "$OBSD/usr/lib/$(cd "$OBSD/usr/lib" && ls libc.so.* | sort -V | tail -1)" "$WORK/lib/libc.so"
ln -sf "$OBSD/usr/lib/$(cd "$OBSD/usr/lib" && ls libm.so.* | sort -V | tail -1)" "$WORK/lib/libm.so"
cp -f "$RT" "$WORK/lib/libcompiler_rt.a"
CLANGLD="$LLVM/build/bin/clang --target=m88k-unknown-openbsd --sysroot=$OBSD -fuse-ld=lld -L$WORK/lib -w"
r88() { timeout "$TMO" "$RUN88K" --os openbsd --root "$OBSD" --mount /work="$WORK" --cwd /work "$@"; }
gcc_cc() { r88 run "$OBSD/usr/bin/cc" -std=gnu99 "$@"; }
link() { local out="$1"; shift; $LLD -e __start -static -o "$out" "$L/crt0.o" "$L/crtbegin.o" "$@" "$L/libm.a" "$L/libc.a" "$RT" "$L/crtend.o"; }
runbin() { r88 run "$1" 2>&1; echo "[exit $?]"; }
pass=0; fail=0; known=0
refdiff=0
report() { # name label result(0=same)
  if [[ "$2" == *"(reference)"* ]]; then
    [ "$3" = 0 ] || { printf "  %-34s %s\n" "$1" "$2 differs from the host result (GCC 3.3.6 issue, informational)"; refdiff=$((refdiff+1)); }
  elif [ "$3" = 0 ]; then printf "  %-34s %s\n" "$1" "$2 ok"; pass=$((pass+1));
  elif [[ "$1" == k* && "$1" != *"lib=gcc,main=gcc"* && "$1" != *"lib=clang,main=clang"* ]]; then printf "  %-34s %s\n" "$1" "$2 DIFFERS (known issue)"; known=$((known+1));
  else printf "  %-34s %s\n" "$1" "$2 DIFFERS"; fail=$((fail+1)); fi
}
tests=("$@")
if [ ${#tests[@]} -eq 0 ]; then for f in "$HERE"/tests/*.c; do tests+=("$(basename "$f" .c)"); done
  for f in "$HERE"/tests/*interop*/; do tests+=("$(basename "$f")"); done; fi
for t in "${tests[@]}"; do
  if [[ "$t" == *interop* ]]; then
    d="$HERE/tests/$t"; cp "$d/lib.c" "$d/main.c" "$WORK/"
    for part in lib main; do gcc_cc -O2 -fno-pic -c -o /work/obj/ref_$part.o $part.c 2>&1 | head -3; done
    link "$WORK/bin/interop_ref" "$WORK/obj/ref_main.o" "$WORK/obj/ref_lib.o" 2> "$WORK/out/interop.ref.lerr" || head -3 "$WORK/out/interop.ref.lerr"
    runbin "$WORK/bin/interop_ref" > "$WORK/out/interop.gccout"
    { cat "$d/main.expected"; echo "[exit 0]"; } > "$WORK/out/interop.ref"
    diff -q "$WORK/out/interop.ref" "$WORK/out/interop.gccout" > /dev/null; report "$t[gcc,gcc linked by lld]" "" $?
    for lib in gcc clang; do for main in gcc clang; do for opt in 0 2; do
      n="$t[lib=$lib,main=$main,O$opt]"
      for part in lib main; do
        cc=$([ $part = lib ] && echo $lib || echo $main)
        if [ $cc = gcc ]; then gcc_cc -O$opt -fno-pic -c -o /work/obj/$part.o "$part.c" 2>&1 | head -3
        else $CLANG -O$opt -c "$d/$part.c" -o "$WORK/obj/$part.o"; fi
      done
      if link "$WORK/bin/interop_x" "$WORK/obj/main.o" "$WORK/obj/lib.o" 2> "$WORK/out/link.err"; then
        runbin "$WORK/bin/interop_x" > "$WORK/out/interop.out"
        diff -q "$WORK/out/interop.ref" "$WORK/out/interop.out" > /dev/null; report "$n" "" $?
      else report "$n" "link FAILED:" 1; head -3 "$WORK/out/link.err"; fi
    done; done; done
    continue
  fi
  src="$HERE/tests/$t.c"; [ -f "$src" ] || { echo "no such test: $t"; continue; }
  cp "$src" "$WORK/$t.c"
  gcc_cc -O2 -fno-pic -c -o /work/obj/${t}_gcc.o "$t.c" 2>&1 | grep -v "warning" | head -3
  link "$WORK/bin/${t}_gcc" "$WORK/obj/${t}_gcc.o" 2> "$WORK/out/$t.gcc.lerr" || head -3 "$WORK/out/$t.gcc.lerr"
  runbin "$WORK/bin/${t}_gcc" > "$WORK/out/$t.gccout"
  # tests/NAME.expected (generated on the host, output + "[exit 0]") wins over the GCC reference
  if [ -f "$HERE/tests/$t.expected" ]; then { cat "$HERE/tests/$t.expected"; echo "[exit 0]"; } > "$WORK/out/$t.ref"; else cp "$WORK/out/$t.gccout" "$WORK/out/$t.ref"; fi
  diff -q "$WORK/out/$t.ref" "$WORK/out/$t.gccout" > /dev/null; report "$t" "gcc (reference)" $?
  for opt in 0 2; do
    if $CLANG -O$opt -c "$src" -o "$WORK/obj/$t.o" 2> "$WORK/out/$t.cerr" && link "$WORK/bin/${t}_O$opt" "$WORK/obj/$t.o" 2> "$WORK/out/$t.lerr"; then
      runbin "$WORK/bin/${t}_O$opt" > "$WORK/out/$t.O$opt.out"
      diff -q "$WORK/out/$t.ref" "$WORK/out/$t.O$opt.out" > /dev/null; report "$t" "clang -O$opt static" $?
    else report "$t" "clang -O$opt build FAILED:" 1; head -3 "$WORK/out/$t.cerr" "$WORK/out/$t.lerr"; fi
    # dynamic executable (libc.so via ld.so), linked by the clang driver
    if $CLANGLD -O$opt "$src" -o "$WORK/bin/${t}_O${opt}dyn" -lm 2> "$WORK/out/$t.dlerr"; then
      runbin "$WORK/bin/${t}_O${opt}dyn" > "$WORK/out/$t.O${opt}dyn.out"
      diff -q "$WORK/out/$t.ref" "$WORK/out/$t.O${opt}dyn.out" > /dev/null; report "$t" "clang -O$opt dynamic" $?
    else report "$t" "clang -O$opt dynamic build FAILED:" 1; head -3 "$WORK/out/$t.dlerr"; fi
  done
done
echo "pass=$pass fail=$fail known-issue=$known reference-differs=$refdiff (outputs in $WORK/out)"
