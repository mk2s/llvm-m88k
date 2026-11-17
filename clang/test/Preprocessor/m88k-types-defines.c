// Predefined type macros on m88k. NetBSD differs in size_t, see
// m88k-netbsd-defines.c.
// RUN: %clang_cc1 -E -dM -triple m88k-unknown-none-elf -x c /dev/null | FileCheck %s
// RUN: %clang_cc1 -E -dM -triple m88k-unknown-openbsd -x c /dev/null | FileCheck %s
// CHECK-DAG: #define __BIGGEST_ALIGNMENT__ 8
// CHECK-DAG: #define __BYTE_ORDER__ __ORDER_BIG_ENDIAN__
// CHECK-DAG: #define __CHAR16_TYPE__ unsigned short
// CHECK-DAG: #define __CHAR32_TYPE__ unsigned int
// CHECK-DAG: #define __INT16_TYPE__ short
// CHECK-DAG: #define __INT32_TYPE__ int
// CHECK-DAG: #define __INT64_TYPE__ long long int
// CHECK-DAG: #define __INT8_TYPE__ signed char
// CHECK-DAG: #define __INTMAX_TYPE__ long long int
// CHECK-DAG: #define __INTPTR_TYPE__ int
// CHECK-DAG: #define __LDBL_MANT_DIG__ 53
// CHECK-DAG: #define __PTRDIFF_TYPE__ int
// CHECK-DAG: #define __SIZEOF_DOUBLE__ 8
// CHECK-DAG: #define __SIZEOF_INT__ 4
// CHECK-DAG: #define __SIZEOF_LONG_DOUBLE__ 8
// CHECK-DAG: #define __SIZEOF_LONG_LONG__ 8
// CHECK-DAG: #define __SIZEOF_LONG__ 4
// CHECK-DAG: #define __SIZEOF_POINTER__ 4
// CHECK-DAG: #define __SIZEOF_PTRDIFF_T__ 4
// CHECK-DAG: #define __SIZEOF_SIZE_T__ 4
// CHECK-DAG: #define __SIZEOF_WCHAR_T__ 4
// CHECK-DAG: #define __UINT64_TYPE__ long long unsigned int
// CHECK-DAG: #define __UINTPTR_TYPE__ unsigned int
// CHECK-DAG: #define __WCHAR_TYPE__ int
// CHECK-NOT: #define __CHAR_UNSIGNED__
// CHECK-NOT: #define __SIZEOF_INT128__
