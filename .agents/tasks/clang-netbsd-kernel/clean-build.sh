#!/bin/sh
# Clean rebuild of GENERIC_CLANG; full log in build-clang.log.
T=/home/mk2s/Documents/llvm-m88k/.agents/tasks/clang-netbsd-kernel
D=/home/mk2s/obj-luna88k/sys/arch/luna88k/compile/GENERIC_CLANG
cd $D || exit 1
find . -name '*.o' -o -name '*.a' | xargs rm -f
rm -f netbsd
sh $T/build.sh netbsd > $T/build-clang.log 2>&1
echo "exit $?" >> $T/build-clang.log
