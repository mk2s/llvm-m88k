// RUN: not %clang_cc1 -triple m88k-unknown-none-elf -target-cpu foo -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s
// CHECK: error: unknown target CPU 'foo'
// CHECK-NEXT: note: valid target CPU values are: generic, mc88000, mc88100, mc88110
int x;
