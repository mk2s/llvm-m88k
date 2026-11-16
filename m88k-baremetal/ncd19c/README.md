# NCD 19c bare-metal BSP (MAME ncd19c)

Files: `crt0.S` (NCD header + entry, bss clear, stack), `ncd19c.ld` (linker script),
`ncd19c_console.[ch]` (serial + screen console), `elf2ncd.py` (ELF -> NCD COFF),
`build.sh`, `run-ncd19c.py` (MAME runner), `ncd19c-load.lua` (MAME autoboot script), `tests/`.

## Build
    ./build.sh boot-hello        # tests/boot-hello.c -> out/boot-hello.elf, out/boot-hello.coff
    ./build.sh libc-hello        # tests named libc-* / val* link picolibc + compiler-rt
picolibc: branch `m88k` of /home/mk2s/Documents/picolibc, installed to `install-m88k`
(`PICOLIBC` overrides). `ncd19c_stdio.c` gives tinystdio stdin/stdout/stderr on the console
(output: serial + screen, input: serial). The heap is picolibc's sbrk between `__heap_start`
and `__heap_end` from `ncd19c.ld`. Rebuild picolibc:

    cd /home/mk2s/Documents/picolibc
    meson setup build-m88k --cross-file scripts/cross-clang-m88k-unknown-none-elf.txt \
      -Dmultilib=false -Dpicocrt=false -Dsemihost=false -Dtests=false -Dsingle-thread=true \
      -Dthread-local-storage=false -Datomic-ungetc=false -Dio-long-long=true \
      -Dnewlib-global-errno=true -Dspecsdir=none --buildtype=minsize --prefix=$PWD/install-m88k
    ninja -C build-m88k -j2 && ninja -C build-m88k install

The CPU is set by `-mcpu=` in the cross file (mc88100; mc88110 later).
Env: `LLVM` (default /home/mk2s/Documents/llvm-m88k, uses build/bin/clang and ld.lld),
`M88K_CPU` (default mc88100). Target: `--target=m88k-unknown-none-elf`.

## Run in MAME (headless output capture)
    ./run-ncd19c.py out/boot-hello.coff --mode inject --expect "boot ok" --shot out/shot.png
    ./run-ncd19c.py out/boot-hello.coff --mode netboot
The runner opens a TCP listener itself (default 127.0.0.1:2340, `--port`) and starts MAME
(/home/mk2s/Documents/mame/mame ncd19c) with serial channel B connected to it via
`-serial1 null_modem -bitb socket.127.0.0.1:PORT`. Program serial output is printed to stdout.
Choose a port other than 2159/2160/2260/2323/2324.

- MAME is always started with `-window` (never fullscreen); it still needs a display.
- Exit code 0 only if EOT was seen (and `--expect` text found). The program ends output with
  byte 0x04 (EOT); the runner then lets the Lua script take the screenshot (`--shot`) and exit.
- Only the MAME process started by the runner is ever killed.
- `inject` (default): the Lua script waits `--wait` emulated seconds for the ROM monitor, writes
  the image into emulated RAM and jumps to it. Uses `-networkprovider none`; no root needed.
- `netboot`: copies the image over `~/Documents/mame/tftpboot/0A000205` and lets the firmware
  boot it via TFTP. A tftpd must already be running (no sudo is used). The original file is
  backed up to `0A000205.baremetal.bak` and restored afterwards with a sha256 check; the runner
  refuses if another mame is running or a differing backup already exists.
- A private copy of cfg/ncd19c.cfg is used so MAME's own config is not rewritten.

## Console
Output goes to the SCC2681 channel B (AUX, 0x0100002C) and, unconditionally by default, to the
screen through the ROM `out1Char` routine (`ncd_console_mask`), since the real unit has no serial
console option.

## Memory map (ncd19c.ld)
- 0x01000000 DUART (SR_B +0x24, THR_B +0x2C)
- .text/.rodata at 0x04020000 (code RAM; ROM at 0x04000000)
- .data at 0x08010000, .bss after it (NOLOAD); stack at top of RAM, `__ram_top` default 0x08200000,
  `__stack_size` 0x20000. Override with `--defsym`. Real unit has 2 MB data RAM at 0x08000000;
  MAME ncd19c has 8 MB data and 4 MB code.
- The boot ROM accepts only .text, .data, .bss sections.
