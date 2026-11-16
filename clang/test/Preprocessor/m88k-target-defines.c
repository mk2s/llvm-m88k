// RUN: %clang_cc1 -E -dM -triple m88k-unknown-none-elf -target-cpu mc88100 \
// RUN:   -x c /dev/null | FileCheck --check-prefix=NONE88100 %s
// NONE88100-DAG: #define __m88k 1
// NONE88100-DAG: #define __m88k__ 1
// NONE88100-DAG: #define __m88000__ 1
// NONE88100-DAG: #define __mc88100__ 1
// NONE88100-NOT: #define m88k
// NONE88100-NOT: #define m88000
// NONE88100-NOT: __mc88110__

// RUN: %clang_cc1 -E -dM -triple m88k-unknown-none-elf -target-cpu mc88110 \
// RUN:   -x c /dev/null | FileCheck --check-prefix=NONE88110 %s
// NONE88110-DAG: #define __m88k__ 1
// NONE88110-DAG: #define __mc88110__ 1
// NONE88110-NOT: __mc88100__

// RUN: %clang_cc1 -E -dM -triple m88k-unknown-openbsd -target-cpu mc88100 \
// RUN:   -x c /dev/null | FileCheck --check-prefix=OPENBSD %s
// OPENBSD-DAG: #define __m88k__ 1
// OPENBSD-DAG: #define __m88000__ 1
// OPENBSD-DAG: #define __mc88100__ 1

// RUN: %clang_cc1 -E -dM -triple m88k-unknown-sysv4 -target-cpu mc88100 \
// RUN:   -x c /dev/null | FileCheck --check-prefix=SYSV4 %s
// SYSV4-DAG: #define __m88k__ 1
// SYSV4-DAG: #define __m88000__ 1
// SYSV4-DAG: #define m88k 1
// SYSV4-DAG: #define m88000 1
// SYSV4-DAG: #define __mc88100__ 1
