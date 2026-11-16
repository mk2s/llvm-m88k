#!/bin/sh
# Build GENERIC_CLANG kernel with clang/lld. Usage: build.sh [make args, default: netbsd]
T=/home/mk2s/Documents/llvm-m88k/.agents/tasks/clang-netbsd-kernel
B=/home/mk2s/Documents/llvm-m88k/build/bin
cd /home/mk2s/obj-luna88k/sys/arch/luna88k/compile/GENERIC_CLANG || exit 1
exec /home/mk2s/tools-luna88k/bin/nbmake-luna88k -j8 NO_M88K_MOVSTR=1 EXTRA_LINKFLAGS="-z nognustack" CC=$T/clang-wrapper.sh LD=$B/ld.lld AR=$B/llvm-ar RANLIB=$B/llvm-ranlib \
  NM=$B/llvm-nm STRIP=$B/llvm-strip OBJCOPY=$B/llvm-objcopy SIZE="$B/llvm-objdump -h" "${@:-netbsd}"
