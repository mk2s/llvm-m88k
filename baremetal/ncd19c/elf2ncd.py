#!/usr/bin/env python3
"""elf2ncd.py ELF OUT: convert a linked big-endian ELF32 m88k image to NCD COFF/BCS."""
import struct, sys

TEXT_ADDR = 0x04020000
SHF_ALLOC = 2
SHT_NOBITS = 8


def main(elf, out):
    img = open(elf, "rb").read()
    if img[:4] != b"\x7fELF" or img[4] != 1 or img[5] != 2:
        sys.exit("not a big-endian ELF32 file")
    shoff, = struct.unpack(">I", img[0x20:0x24])
    shentsize, shnum, shstrndx = struct.unpack(">HHH", img[0x2e:0x34])
    shdrs = [struct.unpack(">IIIIIIIIII", img[shoff + i * shentsize:shoff + i * shentsize + 40])
             for i in range(shnum)]
    stroff = shdrs[shstrndx][4]
    secs = {}
    for name, typ, flags, addr, off, size, *_ in shdrs:
        n = img[stroff + name:img.index(b"\0", stroff + name)].decode()
        if not flags & SHF_ALLOC or size == 0:
            continue
        if n not in (".text", ".data", ".bss"):
            sys.exit("unexpected allocated section %s (size %#x)" % (n, size))
        secs[n] = (addr, off, size, typ)
    if ".text" not in secs:
        sys.exit("no .text")
    taddr, toff, tsize, _ = secs[".text"]
    if taddr != TEXT_ADDR:
        sys.exit(".text at %#x, expected %#x" % (taddr, TEXT_ADDR))
    text = img[toff:toff + tsize]
    if text[0x10:0x18] != b"Xncd19C\0":
        sys.exit("missing Xncd19C signature at .text+0x10")
    data = b""
    daddr = 0x08010000
    if ".data" in secs:
        daddr, doff, dsize, _ = secs[".data"]
        data = img[doff:doff + dsize]
    baddr, bsize = daddr + len(data), 0
    if ".bss" in secs:
        baddr, _, bsize, _ = secs[".bss"]
    entry = taddr  # the ROM launcher jumps to the start of .text (the NCD header branches to _start)
    toff_ = 20 + 28 + 3 * 44
    hdr = struct.pack(">HHIIIHH", 0x016d, 3, 0, 0, 0, 28, 0x020f)
    opt = struct.pack(">HHIIIIII", 0x010b, 0, len(text), len(data), bsize, entry, taddr, daddr)

    def sec(name, vaddr, size, ptr, flg):
        return struct.pack(">8sIIIIIIIII", name, vaddr, vaddr, size, ptr, 0, 0, 0, 0, flg)
    s = (sec(b".text", taddr, len(text), toff_, 0x20) +
         sec(b".data", daddr, len(data), toff_ + len(text), 0x40) +
         sec(b".bss", baddr, bsize, toff_ + len(text) + len(data), 0x80))
    open(out, "wb").write(hdr + opt + s + text + data)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(*sys.argv[1:])
