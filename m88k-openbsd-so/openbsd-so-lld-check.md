# lld m88k shared objects: structural check against libc.so.96.1

Tools: `out/build-so-tools/bin` (`ld.lld`, `llvm-readelf`, `llvm-mc`). Inputs from
the guest filesystem: `L=/mnt/obsd/usr/lib`.

## Commands

```
# DSO from GCC's crt files and an LLVM-style PIC object (ABDIFF, GOT_16L, DISP26,
# PLT_DISP26 of crtbeginS.o are all resolved by lld)
ld.lld -shared -z now -soname=libt.so $L/crtbeginS.o a.o $L/crtendS.o -o libt.so
llvm-readelf -r -d -l libt.so

# executable that links a DSO and libc (there is no unversioned libc.so)
ld.lld -nopie -rpath /work -dynamic-linker /usr/libexec/ld.so \
  $L/crt0.o $L/crtbegin.o main.o libfoo.so $L/libc.so.96.1 $L/crtend.o -o prog
run88k --root /mnt/obsd --mount /work=$PWD --cwd /work run prog
```

The last command prints the DSO's output and the exit status of `main`; the
real ld.so loads the lld-built `libfoo.so` (BIND_NOW), resolves the GOTP_ENT
slots in `libfoo.so` and a R_88K_COPY of a DSO variable in the executable.

## Result versus `libc.so.96.1`

Same:
- ET_DYN, OSABI 0, ABI version 0, flags 0.
- Dynamic tags: SONAME, INIT_ARRAY/SZ, HASH, GNU_HASH, STRTAB, SYMTAB, STRSZ,
  SYMENT, PLTGOT (= `.got.plt` start), PLTRELSZ, PLTREL=RELA, JMPREL,
  0x70000002/0x70000003 (`.plt` start and end), RELA, RELASZ (without
  `.rela.plt`), RELAENT=12, BIND_NOW (tag 0x18), FLAGS_1=NOW, RELACOUNT.
  No DT_DEBUG, no DT_TEXTREL. INIT/FINI appear only when `crtbeginS.o` brings
  `.init`/`.fini`.
- `.rela.dyn`: RELACOUNT `R_88K_BBASED_32` entries first (symbol 0), then
  `R_88K_32` against symbols. `.rela.plt`: `R_88K_GOTP_ENT`, addend = address of
  the PLT entry, same order as the `.got.plt` slots.
- LOAD kinds R E, RWE (`.plt` alone, 72 + 24n bytes), RW; PT_DYNAMIC, PT_NOTE,
  PT_OPENBSD_RANDOMIZE, PT_GNU_RELRO. `p_offset == p_vaddr (mod 0x1000)`.
- Executable: PT_INTERP, `DT_NEEDED`, `DT_DEBUG`, 0x70000002/3, COPY relocation,
  no BIND_NOW (lazy, GOT pre-filled).

Different (expected, harmless for ld.so):
- lld starts with an R LOAD (headers, `.hash`, `.dynsym`, ...) before the R E
  text LOAD; libc has text first. Segment kinds and alignment are the same.
- lld ends RELRO on a page boundary and starts `.data` in a second RW LOAD
  (libc has one RW LOAD). `.got.plt` and `.got` are separate sections that
  are adjacent; `.got` always directly follows `.got.plt`.
- PT_PHDR and PT_GNU_STACK are present (libc has neither); ld.so ignores both.
- FLAGS_1 is NOW only (libc: NOW|INITFIRST, a libc-specific link option).
- Without `-z now` a DSO's `.got.plt` and `.got` are not in RELRO; with it both are.
- GOT_16L above 0xffff is an error (documented limit of 64 KiB for `.got.plt`
  plus `.got`), so GCC large-model objects with a `GOT_16H/16L` pair beyond 64 KiB
  do not link.
