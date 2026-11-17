#!/usr/bin/env python3
"""Build the debug demo: demo.c (clang -g) + the RSP stub -> out/demo.elf, out/demo.img.

  build.py [-O0|-O1|-O2|-Os]      (default -O0)

out/demo.elf has the symbols and DWARF for the debugger; out/demo.img is the NCD COFF
image that MAME (gdbstub/tools/mame_harness.py) or the TFTP boot loads.

The stub (gdbstub/stub/{start.s,stub.c}) is built with the luna88k GNU toolchain like
gdbstub/tools/build.py does; everything is linked with ld.lld at 0x04020000.
"""
import os
import re
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "out")
LLVM = os.environ.get("LLVM", "/home/mk2s/Documents/llvm-m88k")
LB = os.path.join(LLVM, "build", "bin")
GS = os.environ.get("GDBSTUB", "/home/mk2s/Documents/ncd-88k-xterm/gdbstub")
TB = os.path.expanduser("~/tools-luna88k/bin/m88k--netbsdelf3-")
BASE = 0x04020000

sys.path.insert(0, os.path.join(GS, "tools"))
from coff import Coff  # noqa: E402

# same flags as gdbstub/tools/build.py
STUB_CFLAGS = ["-O2", "-ffreestanding", "-fno-builtin", "-nostdlib", "-fomit-frame-pointer", "-Wall"]
CFLAGS = ["--target=m88k-unknown-none-elf", "-mcpu=mc88100", "-g", "-ffreestanding",
          "-fno-unwind-tables", "-fno-asynchronous-unwind-tables", "-Wall"]


def run(cmd):
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode:
        raise SystemExit("FAILED: %s\n%s%s" % (" ".join(cmd), r.stdout, r.stderr))
    return r.stdout


def main():
    opt = sys.argv[1] if len(sys.argv) > 1 else "-O0"
    os.makedirs(OUT, exist_ok=True)
    o = lambda name: os.path.join(OUT, name)

    # the stub
    run([TB + "as", os.path.join(GS, "stub", "start.s"), "-o", o("stub_start.o")])
    run([TB + "gcc"] + STUB_CFLAGS + ["-c", os.path.join(GS, "stub", "stub.c"), "-o", o("stub.o")])
    # the program
    clang = os.path.join(LB, "clang")
    run([clang] + CFLAGS + ["-c", os.path.join(HERE, "demo_start.S"), "-o", o("demo_start.o")])
    run([clang] + CFLAGS + [opt, "-c", os.path.join(HERE, "demo.c"), "-o", o("demo.o")])

    # link: header first, then stub, then the program (stub.ld layout, base 0x04020000)
    ld = open(os.path.join(GS, "stub", "stub.ld")).read()
    ld = ld.replace(". = BASE;", ". = 0x%x;" % BASE).replace('OUTPUT_FORMAT("elf32-m88k")', "")
    open(o("demo.ld"), "w").write(ld)
    elf = o("demo.elf")
    run([os.path.join(LB, "ld.lld"), "-T", o("demo.ld"), "-o", elf,
         o("demo_start.o"), o("stub_start.o"), o("stub.o"), o("demo.o")])

    syms = {}
    for line in run([os.path.join(LB, "llvm-nm"), elf]).splitlines():
        m = re.match(r"([0-9a-f]+) \w (\S+)$", line.strip())
        if m:
            syms[m.group(2)] = int(m.group(1), 16)

    # flat blob covering everything up to _end (bss and the stub's vector table)
    binf = o("demo.bin")
    run([os.path.join(LB, "llvm-objcopy"), "-O", "binary", elf, binf])
    blob = bytearray(open(binf, "rb").read())
    need = syms["_end"] - BASE
    if len(blob) < need:
        blob += bytes(need - len(blob))
    # the stub resumes through `stub_cont_insn`: aim it at test_main
    at = syms["stub_cont_insn"]
    struct.pack_into(">I", blob, at - BASE,
                     0xc0000000 | (((syms["test_main"] - at) >> 2) & 0x03ffffff))
    img = o("demo.img")
    open(img, "wb").write(Coff(bytes(blob), bytes(16), 16).pack())
    print("%s (%s): %d bytes, main %08x, fib %08x" % (img, opt, len(blob), syms["main"], syms["fib"]))


if __name__ == "__main__":
    main()
