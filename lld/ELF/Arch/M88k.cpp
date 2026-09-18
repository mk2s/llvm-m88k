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
// LLVM m88k backend:
//
//   R_88K_NONE     no-op
//   R_88K_32       S + A                    (absolute 32-bit word)
//   R_88K_16H      (S + A) >> 16            (upper 16 bits, e.g. or.u %hi16)
//   R_88K_16L      (S + A) & 0xffff         (lower 16 bits, e.g. or  %lo16)
//   R_88K_DISP16   (S + A - P) >> 2         (16-bit word displacement)
//   R_88K_DISP26   (S + A - P) >> 2         (26-bit word displacement, br/bsr)
//
// The 16-bit immediate fields live in the low half of the instruction word
// (bits 15-0); the 26-bit displacement field lives in bits 25-0.
//
//===----------------------------------------------------------------------===//

#include "Symbols.h"
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
  case R_88K_DISP26:
    return R_PC;
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
  case R_88K_DISP26: {
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

void elf::setM88kTargetInfo(Ctx &ctx) { ctx.target.reset(new M88k(ctx)); }
