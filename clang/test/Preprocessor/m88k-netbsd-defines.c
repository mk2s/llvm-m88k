// RUN: %clang_cc1 -E -dM -triple m88k-unknown-netbsd -x c /dev/null \
// RUN:   | FileCheck --check-prefix=NETBSD %s
// NETBSD-DAG: #define __NetBSD__ 1
// NETBSD-DAG: #define __unix__ 1
// NETBSD-DAG: #define __ELF__ 1
// NETBSD-DAG: #define __m88k__ 1
// NETBSD-DAG: #define __SIZE_TYPE__ unsigned int
// NETBSD-DAG: #define __PTRDIFF_TYPE__ int
// NETBSD-DAG: #define __WCHAR_TYPE__ int
// NETBSD-DAG: #define __WINT_TYPE__ int
// NETBSD-DAG: #define __INTMAX_TYPE__ long long int
// NETBSD-DAG: #define __INT64_TYPE__ long long int
// NETBSD-DAG: #define __SIZE_MAX__ 4294967295U

// OpenBSD keeps size_t as unsigned long.
// RUN: %clang_cc1 -E -dM -triple m88k-unknown-openbsd -x c /dev/null \
// RUN:   | FileCheck --check-prefix=OPENBSD-SZ %s
// OPENBSD-SZ: #define __SIZE_TYPE__ long unsigned int
