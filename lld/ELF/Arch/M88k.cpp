//===- M88k.cpp -----------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// The Motorola 88000 (m88k) is a big-endian 32-bit RISC architecture.  All
// instructions are 4 bytes wide.  This file implements the subset of the
// SysV/m88k ABI relocations needed to statically link objects produced by the
// LLVM m88k backend, plus what is needed to link non-PIE dynamic executables
// for OpenBSD (see libexec/ld.so/m88k/rtld_machine.c):
//
//   R_88K_NONE     no-op
//   R_88K_32       S + A                    (absolute 32-bit word)
//   R_88K_16H      (S + A) >> 16            (upper 16 bits, e.g. or.u %hi16)
//   R_88K_16L      (S + A) & 0xffff         (lower 16 bits, e.g. or  %lo16)
//   R_88K_DISP16   (S + A - P) >> 2         (16-bit word displacement)
//   R_88K_DISP26   (S + A - P) >> 2         (26-bit word displacement, br/bsr)
//   R_88K_PLT_DISP26 (L + A - P) >> 2       (same as DISP26)
//   R_88K_ABDIFF_16H/16L  halves of GOT - (S + A)
//   R_88K_GOT_16H/16L     halves of (offset of the GOT entry of S from the GOT
//                         base); 16L must fit in 16 bits, the GOT is small
//
// The GOT base is .got.plt (DT_PLTGOT, _GLOBAL_OFFSET_TABLE_), as with GNU ld
// where .got.plt and .got are one section. lld places .got right after
// .got.plt so GOT entries have offsets in [0, 0xffff] from the base.
// ABDIFF is used by the GETGOT prologue:  or.u %r25,%r0,hi16(.Lpc#abdiff) ;
// bsr.n .Lpc ; or %r25,%r25,lo16(.Lpc#abdiff) ; .Lpc: addu %r25,%r25,%r1.
//
// DISP26 and PLT_DISP26 go through the PLT when the target is in a DSO.
//
// Dynamic relocations: R_88K_32 (symbolic and GOT), R_88K_BBASED_32 (load
// base relative), R_88K_COPY, and R_88K_GOTP_ENT for PLT slots. A GOTP_ENT
// relocation points at the .got.plt slot; its addend is the address of the
// PLT entry, whose or.u/ld immediates ld.so rewrites to the slot address. So
// the PLT must be writable. The PLT layout is the one GNU ld uses on OpenBSD:
//
//   PLT0:  subu  r31,r31,16 ; st r1,r31,12 ; st r11,r31,8 ; bsr 1f
//      1:  or.u  r11,r0,hi16(.got.plt - 1b) ; or r11,r11,lo16(.got.plt - 1b)
//          addu  r11,r11,r1 ; ld r1,r11,4 ; st r1,r31,4 ; ld r11,r11,8
//          st    r0,r31,0 ; jsr r11 ; 6 x nop
//   PLTn:  or.u  r11,r0,hi16(slot) ; ld r11,r11,lo16(slot) ; jmp r11
//          or.u  r11,r0,hi16(reloff) ; or r11,r11,lo16(reloff) ; br PLT0
//
// .got.plt[0] is _DYNAMIC, [1] and [2] are set by ld.so (object, resolver);
// each slot initially points at the or.u after the jmp of its PLT entry.
//
// The 16-bit immediate fields live in the low half of the instruction word
// (bits 15-0); the 26-bit displacement field lives in bits 25-0.
//
//===----------------------------------------------------------------------===//

#include "Symbols.h"
#include "SyntheticSections.h"
#include "Target.h"
#include "lld/Common/ErrorHandler.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Support/Endian.h"

using namespace llvm;
using namespace llvm::support::endian;
using namespace llvm::ELF;
using namespace lld;
using namespace lld::elf;

namespace {
class M88k final : public TargetInfo {
public:
  M88k(Ctx &);
  RelExpr getRelExpr(RelType type, const Symbol &s,
                     const uint8_t *loc) const override;
  RelType getDynRel(RelType type) const override;
  void writeGotPltHeader(uint8_t *buf) const override;
  void writeGotPlt(uint8_t *buf, const Symbol &s) const override;
  void writePltHeader(uint8_t *buf) const override;
  void writePlt(uint8_t *buf, const Symbol &sym,
                uint64_t pltEntryAddr) const override;
  void relocate(uint8_t *loc, const Relocation &rel,
                uint64_t val) const override;
};
} // namespace

M88k::M88k(Ctx &ctx) : TargetInfo(ctx) {
  // "or %r0, %r0, %r0" is the canonical m88k no-op; use it as trap/pad filler
  // so that gaps in executable sections stay valid instructions.
  trapInstr = {0xf4, 0x00, 0x58, 0x00};

  defaultCommonPageSize = 4096;
  defaultMaxPageSize = 4096;
  defaultImageBase = 0x1000;

  copyRel = R_88K_COPY;
  gotRel = R_88K_32;
  pltRel = R_88K_GOTP_ENT;
  relativeRel = R_88K_BBASED_32;
  symbolicRel = R_88K_32;
  gotPltHeaderEntriesNum = 3;
  gotBaseSymInGotPlt = true;
  pltHeaderSize = 72;
  pltEntrySize = 24;
}

RelExpr M88k::getRelExpr(RelType type, const Symbol &s,
                         const uint8_t *loc) const {
  switch (type) {
  case R_88K_NONE:
    return R_NONE;
  case R_88K_32:
  case R_88K_16H:
  case R_88K_16L:
    return R_ABS;
  case R_88K_DISP16:
    return R_PC;
  // The 26-bit displacement only exists in br/bsr, so a DISP26 relocation is
  // always a branch and may go through the PLT, as with GNU ld (GCC uses
  // DISP26, not PLT_DISP26, for calls to external functions).
  case R_88K_DISP26:
  case R_88K_PLT_DISP26:
    return R_PLT_PC;
  case R_88K_ABDIFF_16H:
  case R_88K_ABDIFF_16L:
    return R_GOTPLTREL;
  case R_88K_GOT_16H:
  case R_88K_GOT_16L:
    return R_GOTPLT;
  default:
    Err(ctx) << getErrorLoc(ctx, loc) << "unknown relocation (" << type.v
             << ") against symbol " << &s;
    return R_NONE;
  }
}

void M88k::relocate(uint8_t *loc, const Relocation &rel, uint64_t val) const {
  switch (rel.type) {
  case R_88K_NONE:
    break;
  case R_88K_32:
    checkIntUInt(ctx, loc, val, 32, rel);
    write32be(loc, val);
    break;
  case R_88K_16H:
    // The relocation offset already points at the 16-bit immediate field
    // (the low half-word of the instruction), so write directly at loc.
    write16be(loc, (val >> 16) & 0xffff);
    break;
  case R_88K_16L:
    write16be(loc, val & 0xffff);
    break;
  case R_88K_ABDIFF_16H:
    // val is S + A - GOT; the relocation computes GOT - (S + A).
    write16be(loc, (-val >> 16) & 0xffff);
    break;
  case R_88K_ABDIFF_16L:
    write16be(loc, -val & 0xffff);
    break;
  case R_88K_GOT_16H:
    write16be(loc, (val >> 16) & 0xffff);
    break;
  case R_88K_GOT_16L:
    checkUInt(ctx, loc, val, 16, rel);
    write16be(loc, val & 0xffff);
    break;
  case R_88K_DISP16: {
    // PC-relative, word-scaled, 16-bit signed displacement.  The relocation
    // offset points at the instruction word; the field is in bits 15-0.
    if (val & 0x3)
      Err(ctx) << getErrorLoc(ctx, loc) << "improper alignment for R_88K_DISP16";
    int64_t disp = SignExtend64<32>(val) >> 2;
    checkInt(ctx, loc, disp, 16, rel);
    write16be(loc + 2, disp & 0xffff);
    break;
  }
  case R_88K_DISP26:
  case R_88K_PLT_DISP26: {
    // PC-relative, word-scaled, 26-bit signed displacement in bits 25-0.
    if (val & 0x3)
      Err(ctx) << getErrorLoc(ctx, loc) << "improper alignment for R_88K_DISP26";
    int64_t disp = SignExtend64<32>(val) >> 2;
    checkInt(ctx, loc, disp, 26, rel);
    write32be(loc, (read32be(loc) & ~0x03ffffff) | (disp & 0x03ffffff));
    break;
  }
  default:
    Err(ctx) << getErrorLoc(ctx, loc) << "unrecognized relocation " << rel.type;
  }
}

RelType M88k::getDynRel(RelType type) const {
  if (type == R_88K_32)
    return type;
  return R_88K_NONE;
}

void M88k::writeGotPltHeader(uint8_t *buf) const {
  write32be(buf, ctx.mainPart->dynamic->getVA());
}

void M88k::writeGotPlt(uint8_t *buf, const Symbol &s) const {
  // Lazy binding: the slot points at the "or.u r11" that loads the relocation
  // offset, right after the indirect jump.
  write32be(buf, s.getPltVA(ctx) + 12);
}

void M88k::writePltHeader(uint8_t *buf) const {
  static const uint32_t insns[] = {
      0x67ff0010, // subu  %r31, %r31, 16
      0x243f000c, // st    %r1, %r31, 12
      0x257f0008, // st    %r11, %r31, 8
      0xc8000001, // bsr   1f
      0x5d600000, // 1: or.u %r11, %r0, hi16(.got.plt - 1b)
      0x596b0000, // or    %r11, %r11, lo16(.got.plt - 1b)
      0xf56b6001, // addu  %r11, %r11, %r1
      0x142b0004, // ld    %r1, %r11, 4
      0x243f0004, // st    %r1, %r31, 4
      0x156b0008, // ld    %r11, %r11, 8
      0x241f0000, // st    %r0, %r31, 0
      0xf400c80b, // jsr   %r11
      0xf4005800, 0xf4005800, 0xf4005800, // nop
      0xf4005800, 0xf4005800, 0xf4005800, // nop
  };
  static_assert(sizeof(insns) == 72, "PLT header size");
  for (size_t i = 0; i < std::size(insns); ++i)
    write32be(buf + 4 * i, insns[i]);
  uint32_t off = ctx.in.gotPlt->getVA() - (ctx.in.plt->getVA() + 16);
  write16be(buf + 18, off >> 16);
  write16be(buf + 22, off & 0xffff);
}

void M88k::writePlt(uint8_t *buf, const Symbol &sym,
                    uint64_t pltEntryAddr) const {
  uint32_t slot = sym.getGotPltVA(ctx);
  uint32_t relOff = sym.getPltIdx(ctx) * sizeof(llvm::ELF::Elf32_Rela);
  int64_t disp =
      (int64_t)ctx.in.plt->getVA() - (int64_t)(pltEntryAddr + 20);
  write32be(buf, 0x5d600000 | (slot >> 16));          // or.u %r11, %r0, hi16
  write32be(buf + 4, 0x156b0000 | (slot & 0xffff));   // ld %r11, %r11, lo16
  write32be(buf + 8, 0xf400c00b);                     // jmp %r11
  write32be(buf + 12, 0x5d600000 | (relOff >> 16));   // or.u %r11, %r0, hi16
  write32be(buf + 16, 0x596b0000 | (relOff & 0xffff)); // or %r11, %r11, lo16
  write32be(buf + 20, 0xc0000000 | ((disp >> 2) & 0x03ffffff)); // br PLT0
}

void elf::setM88kTargetInfo(Ctx &ctx) { ctx.target.reset(new M88k(ctx)); }
