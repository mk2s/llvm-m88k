// UNSUPPORTED: system-windows

// RUN: %clang -### --target=m88k-unknown-none-elf -mcpu=mc88100 \
// RUN:   --sysroot=%S/Inputs/basic_baremetal_tree %s 2>&1 \
// RUN:   | FileCheck --check-prefix=CHECK-LINK %s
// CHECK-LINK: "-cc1" "-triple" "m88k-unknown-none-elf"
// CHECK-LINK-SAME: "-mrelocation-model" "static"
// CHECK-LINK-SAME: "-nostdsysteminc"
// CHECK-LINK-SAME: "-target-cpu" "mc88100"
// CHECK-LINK-SAME: "-internal-isystem" "{{[^"]*}}basic_baremetal_tree{{[/\\]+}}include"
// CHECK-LINK-NEXT: ld.lld{{(\.exe)?}}"
// CHECK-LINK-SAME: "-Bstatic" "crt0.o"
// CHECK-LINK-SAME: "-L{{[^"]*}}basic_baremetal_tree{{[/\\]+}}lib"
// CHECK-LINK-SAME: "{{[^"]*}}libclang_rt.builtins.a"
// CHECK-LINK-SAME: "-lc"
// CHECK-LINK-SAME: "-o" "a.out"

// RUN: %clang -### --target=m88k-unknown-none-elf -mcpu=mc88100 -nostartfiles \
// RUN:   --sysroot=%S/Inputs/basic_baremetal_tree %s 2>&1 \
// RUN:   | FileCheck --check-prefix=CHECK-NOSTART %s
// CHECK-NOSTART: ld.lld{{(\.exe)?}}"
// CHECK-NOSTART-NOT: crt0.o

// The CPU stays selectable through -mcpu.
// RUN: %clang -### --target=m88k-unknown-none-elf -mcpu=mc88110 -c %s 2>&1 \
// RUN:   | FileCheck --check-prefix=CHECK-88110 %s
// CHECK-88110: "-cc1" "-triple" "m88k-unknown-none-elf"
// CHECK-88110-SAME: "-target-cpu" "mc88110"
