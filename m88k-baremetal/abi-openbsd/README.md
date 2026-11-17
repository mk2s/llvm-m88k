# OpenBSD 7.1 luna88k GCC 3.3.6 reference output (via run88k)
Generated with:
  ~/Documents/run88k/zig-out/bin/x86_64-linux/run88k --root /mnt/obsd --mount /work=DIR --cwd /work run /mnt/obsd/usr/bin/cc -O2 -S -o X.obsd.s X.c
Compare with `clang --target=m88k-unknown-openbsd -O2 -S`. The NetBSD cross GCC (m88k--netbsdelf3-gcc)
uses a different, older ABI (stack arguments at r31+32, structs consume argument words) and is not a reference.
Findings: fixed-argument calls (struct by value on the stack at r31+0 without using register words, sret in r12,
stack arguments from r31+0) match. va_list handling differs; see the llvm/lib/Target/M88k/KNOWN_ISSUES.md.
