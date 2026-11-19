#!/bin/bash
# Build and run the m88k-openbsd shared object tests under run88k.
# Needs: a zig with m88k-openbsd shared object support, run88k, /mnt/obsd (OpenBSD 7.1 fs).
# The programs run under the real $ROOT/usr/libexec/ld.so. run88k takes the
# host path of the program and detects OpenBSD only from PT_OPENBSD_* headers,
# which --gc-sections drops with the unused __guard_local, so --os openbsd is
# passed. ld.so finds libfoo.so through the rpath /work of the exe.
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
ZIG=${ZIG:?set ZIG to a zig with m88k-openbsd shared object support}
RUN88K=${RUN88K:?set RUN88K to the run88k binary}
ROOT=${ROOT:-/mnt/obsd}
W=${W:-/tmp/so-rt}
L=$ROOT/usr/lib
rm -rf "$W"; mkdir -p "$W"; cd "$W" || exit 1
cp "$HERE"/*.c "$HERE"/*.zig .
cat > libc.txt <<EOT
include_dir=$ROOT/usr/include
sys_include_dir=$ROOT/usr/include
crt_dir=$L
msvc_lib_dir=
kernel32_lib_dir=
gcc_dir=
darwin_sdk_dir=
EOT
T="-target m88k-openbsd"
export ZIG_LOCAL_CACHE_DIR=$W/zc ZIG_GLOBAL_CACHE_DIR=$W/zg
pass=0; failn=0
res() { if [ "$1" = 0 ]; then echo "PASS $2"; pass=$((pass+1)); else echo "FAIL $2"; failn=$((failn+1)); fi; }
build() { # name cmd...
	n=$1; shift; "$@" >"build-$n.log" 2>&1; r=$?; [ $r = 0 ] || { echo "--- build $n"; cat "build-$n.log"; }; res $r "build $n"; return $r; }
run() { # name expected-file-or-pattern prog
	n=$1; exp=$2; shift 2
	"$RUN88K" --os openbsd --root "$ROOT" --mount /work="$W" --cwd /work --trace run "$@" >"out-$n.txt" 2>"trace-$n.txt"; rc=$?
	if [ $rc = 0 ] && diff -q "out-$n.txt" "$exp" >/dev/null; then res 0 "run $n"; else
		echo "--- run $n rc=$rc"; cat "out-$n.txt"; tail -20 "trace-$n.txt"; res 1 "run $n"; fi; }
LIBC=$L/libc.so.96.1
for opt in Debug ReleaseFast ReleaseSmall; do
	O=-O$opt; s=$opt
	# Debug C code would pull in Zig's ubsan runtime, which the m88k backend
	# cannot compile yet (G_MEMCPY_INLINE); keep the C checks as traps.
	[ $opt = Debug ] && O="$O -fno-sanitize-c"
	build "libfoo-$s" $ZIG build-lib $T -dynamic -fPIC $O --libc libc.txt -lc --name foo libfoo.c $LIBC || continue
	cp libfoo.so libfoo-$s.so
	build "main-$s" $ZIG build-exe $T $O --libc libc.txt -lc --name main-$s main.c libfoo.so $LIBC -rpath /work || continue
	cat > expected-main.txt <<EOT
foo: hello from libfoo
ok: foo_init_ran == 1234
ok: foo_counter == 41
ok: foo_ptr == &foo_counter && *foo_ptr == 41
ok: foo_add(1) == 42
ok: fp(2) == 43
ok: foo_call_exe(10) == 21
ok: foo_apply(triple, 5) == 15
ok: foo_apply(foo_add, 9) == 50
ok: *foo_static_addr() == 7
ok: foo_add(1) == 51 && *foo_ptr == 50
ok: foo_ext_add(0) == 150
done fails=0
EOT
	run "main-$s" expected-main.txt "$W/main-$s"
	if [ "$s" = Debug ]; then
		# --trace prints no path names: ld.so reads one ELF header per
		# library (libfoo.so, libc.so.96.1) and maps libc's 0xfd000 span.
		[ "$(grep -c 'syscall 3 (read) a0=3 .* a2=1000$' trace-main-$s.txt)" = 2 ] &&
			grep -q 'syscall 49 (mmap) a0=0 a1=fd000 ' trace-main-$s.txt; res $? "trace: ld.so loads two libraries"
		# ld.so makes each object's PLT (and text) read+exec after relocating.
		[ "$(grep -c 'syscall 74 (mprotect) .* a2=5$' trace-main-$s.txt)" -ge 3 ]; res $? "trace: PLT pages mprotected R|X"
		! grep -q 'kbind' trace-main-$s.txt; res $? "no kbind (BIND_NOW)"
	fi
done
# libc only
build main_libc_only $ZIG build-exe $T -OReleaseFast --libc libc.txt -lc --name main_libc_only main_libc_only.c $LIBC && {
	echo "libc-only 1 ok" > expected-libc.txt; run libc_only expected-libc.txt "$W/main_libc_only"; }
# GCC crt objects with an lld-linked exe and DSO
LLD=${LLD:-}
if [ -n "$LLD" ] && [ -x "$LLD" ]; then
	$ZIG build-obj $T -OReleaseFast -fPIC --libc libc.txt libfoo.c && $ZIG build-obj $T -OReleaseFast --libc libc.txt main.c &&
	"$LLD" -shared -z now -soname=libfoo.so $L/crtbeginS.o libfoo.o $L/crtendS.o -o libfoo.so &&
	"$LLD" -nopie -rpath /work -dynamic-linker /usr/libexec/ld.so $L/crt0.o $L/crtbegin.o main.o libfoo.so $LIBC $L/crtend.o -o main-crt
	res $? "build gcc-crt variant"
	run main-crt expected-main.txt "$W/main-crt"
fi
# Zig source shared object
for opt in ReleaseFast ReleaseSmall; do
	build "zfoo-$opt" $ZIG build-lib $T -dynamic -fPIC -O$opt --libc libc.txt -lc --name zfoo foo.zig $LIBC || continue
	cp libzfoo.so libzfoo-$opt.so
	build "main_zfoo-$opt" $ZIG build-exe $T -OReleaseFast --libc libc.txt -lc --name main_zfoo-$opt main_zfoo.c libzfoo.so $LIBC -rpath /work || continue
	echo "zfoo 7 1 5" > expected-zfoo.txt
	run "zfoo-$opt" expected-zfoo.txt "$W/main_zfoo-$opt"
done
echo "passed=$pass failed=$failn"
[ $failn = 0 ]
