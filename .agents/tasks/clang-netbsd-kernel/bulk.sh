cd ~/obj-luna88k/sys/arch/luna88k/compile/GENERIC || exit 1
S=~/Documents/netbsd-src/sys
CL=~/Documents/llvm-m88k/build/bin/clang
EXTRA=${EXTRA--ferror-limit=0 -Wno-deprecated-non-prototype -Wno-strict-prototypes -Wno-header-guard -Wno-gnu-folding-constant -Wno-pointer-sign -Wno-unused-but-set-variable -Wno-knr-promoted-parameter -Wno-format-security -Wno-shift-negative-value -Wno-constant-conversion -Wno-incompatible-pointer-types-discards-qualifiers -Wno-pointer-compare -Wno-unused-function -Wno-enum-conversion -Wno-parentheses-equality -Wno-array-parameter -Wno-misleading-indentation -Wno-constant-logical-operand -Wno-tautological-constant-compare}
STD=${STD--std=gnu89}
FL="--target=m88k-unknown-netbsd -mcpu=mc88100 -ffreestanding -O2 -nostdinc -I. -I$S/arch -I$S -Dluna88k -DM88100 -DMAX_CPUS=4 -DERRATA__XXX_USR -DDEBUG -DDIAGNOSTIC -DMAXUSERS=8 -D_KERNEL -D_KERNEL_OPT -Werror -Wall -Wno-main -Wno-format-zero-length -Wpointer-arith -Wmissing-prototypes -Wstrict-prototypes -Wno-sign-compare -Wno-uninitialized $EXTRA -fno-pie $STD"
one() { f=$1; b=$(echo $f | tr / _); if timeout 300 $CL $FL $( [[ $f == *.S ]] && echo "-D_LOCORE -x assembler-with-cpp -traditional-cpp" ) -c $f -o /home/mk2s/Documents/llvm-m88k/.agents/tasks/clang-netbsd-kernel/bulk/$b.o 2>/home/mk2s/Documents/llvm-m88k/.agents/tasks/clang-netbsd-kernel/bulk/$b.err; then echo "OK $f"; else echo "FAIL $f"; fi; }
export -f one; export CL FL
mkdir -p /home/mk2s/Documents/llvm-m88k/.agents/tasks/clang-netbsd-kernel/bulk
sed -n '/^CFILES=/,/^$/p;/^SFILES=/,/^$/p' Makefile | sed 's/^[CS]FILES=//;s/\\$//' | tr ' \t' '\n\n' | grep . | sed "s#\$S#$S#" | xargs -P8 -I{} bash -c 'one {}' > /home/mk2s/Documents/llvm-m88k/.agents/tasks/clang-netbsd-kernel/bulk/result.txt
