// An unknown CPU name is rejected with the list of valid names.
// RUN: not %clang --target=m88k-unknown-none-elf -mcpu=foo -c %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s
// RUN: not %clang --target=m88k-unknown-none-elf -mtune=foo -c %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s --check-prefix=TUNE
// CHECK: error: unknown target CPU 'foo'
// CHECK-NEXT: note: valid target CPU values are: generic, mc88000, mc88100, mc88110
// TUNE: error: unknown target CPU 'foo'
// TUNE-NEXT: note: valid target CPU values are: generic, mc88000, mc88100, mc88110
int x;
