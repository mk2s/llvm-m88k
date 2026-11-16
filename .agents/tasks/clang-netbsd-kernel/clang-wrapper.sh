#!/bin/sh
# clang front end for the NetBSD luna88k kernel build (CC=this script).
# -std=gnu89 matches GCC 3.3's default (K&R definitions need it).
# -fcommon: GCC 3.3 default; the kernel has tentative definitions in several headers/files.
# The -Wno-* list (FEAT-001) must come AFTER "$@" so it overrides the makefile's -Wstrict-prototypes.
# genassym.sh emits "XYZZY name value" via __asm and compiles with -S; the integrated assembler
# would parse and reject it, so pass inline asm through as text when only emitting assembly.
X=
for a in "$@"; do [ "$a" = -S ] && X=-fno-integrated-as; done
exec /home/mk2s/Documents/llvm-m88k/build/bin/clang --target=m88k-unknown-netbsd -mcpu=mc88100 -fno-pie -std=gnu89 -fcommon $X "$@" \
 -Wno-deprecated-non-prototype -Wno-strict-prototypes -Wno-header-guard -Wno-gnu-folding-constant -Wno-pointer-sign \
 -Wno-unused-but-set-variable -Wno-knr-promoted-parameter -Wno-format-security -Wno-shift-negative-value \
 -Wno-constant-conversion -Wno-incompatible-pointer-types-discards-qualifiers -Wno-pointer-compare -Wno-unused-function \
 -Wno-enum-conversion -Wno-parentheses-equality -Wno-array-parameter -Wno-misleading-indentation \
 -Wno-constant-logical-operand -Wno-tautological-constant-compare
