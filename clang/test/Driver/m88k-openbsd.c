// OpenBSD/m88k does not default to PIE (OpenBSD's own cc produces ET_EXEC):
// static relocation model, crt0.o (not rcrt0.o) with -static, and -nopie for
// the linker. Other OpenBSD targets still default to PIE.
// RUN: %clang -### --target=m88k-unknown-openbsd %s 2>&1 | FileCheck %s --check-prefix=DYN
// DYN: "-cc1" "-triple" "m88k-unknown-openbsd"
// DYN-SAME: "-mrelocation-model" "static"
// DYN-NOT: "-pic-is-pie"
// DYN: ld{{.*}}" {{.*}}"-nopie"
// DYN-SAME: "{{[^"]*}}crt0.o"
// DYN-NOT: rcrt0.o

// RUN: %clang -### --target=m88k-unknown-openbsd -static %s 2>&1 | FileCheck %s --check-prefix=STATIC
// STATIC: "-mrelocation-model" "static"
// STATIC: "-Bstatic"
// STATIC-SAME: "{{[^"]*}}crt0.o"
// STATIC-NOT: rcrt0.o

// -pie (link) and -fPIE (compile) still select PIE explicitly.
// RUN: %clang -### --target=m88k-unknown-openbsd -fPIE -pie %s 2>&1 | FileCheck %s --check-prefix=PIE
// PIE: "-mrelocation-model" "pic" "-pic-level" "2" "-pic-is-pie"
// PIE: "-pie"
// PIE-NOT: "-nopie"

// RUN: %clang -### --target=x86_64-unknown-openbsd %s 2>&1 | FileCheck %s --check-prefix=X86
// X86: "-pic-is-pie"
