#!/bin/sh
# Reproduce the two miscompiles: build clang with the fixes reverted, compile the
# two functions to assembly, rebuild clang with the fixes, compile again.
# Output: bisect-before.s / bisect-after.s (full asm) and bisect.log (extracts).
L=/home/mk2s/Documents/llvm-m88k
T=$L/.agents/tasks/clang-netbsd-kernel
S=/home/mk2s/Documents/netbsd-src/sys
F1=llvm/lib/Target/M88k/M88kDelaySlotFiller.cpp
F2=llvm/lib/CodeGen/GlobalISel/CombinerHelper.cpp
cd $L || exit 1
FLAGS="-ffreestanding -O2 -Wall -Wno-main -Wpointer-arith -Dluna88k -I$S/arch -I$S -nostdinc -DM88100 -DMAX_CPUS=4 -DERRATA__XXX_USR -DDEBUG -DDIAGNOSTIC -DMAXUSERS=8 -D_KERNEL -D_KERNEL_OPT"
OBJ=/home/mk2s/obj-luna88k/sys/arch/luna88k/compile/GENERIC_CLANG
gen() { # $1 = suffix
  for f in uvm/uvm_map kern/kern_timeout; do
    n=$(basename $f)
    (cd $OBJ && $T/clang-wrapper.sh $FLAGS -I. -S -o /tmp/$n.$1.s $S/$f.c) || echo "compile $f failed"
  done
}
git checkout c3b64cf74ad7^ -- $F1
git checkout a484b97d8537^ -- $F2
ninja -C build clang > /tmp/bisect-ninja-before.log 2>&1 || { echo ninja-before failed; }
gen before
git checkout HEAD -- $F1 $F2
ninja -C build clang > /tmp/bisect-ninja-after.log 2>&1 || { echo ninja-after failed; }
gen after
git status --short -- llvm
