//===-- M88kMCAsmBackend.cpp - M88k assembler backend ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/M88kMCFixups.h"
#include "MCTargetDesc/M88kMCTargetDesc.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/Twine.h"
#include "llvm/ADT/bit.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>

using namespace llvm;

namespace {
// Value is a fully-resolved relocation value: Symbol + Addend [- Pivot].
// Return the bits that should be installed in a relocation field for
// fixup kind Kind.
uint64_t extractBitsForFixup(MCFixupKind Kind, uint64_t Value,
                             const MCFixup &Fixup, MCContext &Ctx) {
  if (Kind < FirstTargetFixupKind)
    return Value;

  auto CheckFixupInRange = [&](int64_t Min, int64_t Max) -> bool {
    int64_t SVal = int64_t(Value);
    if (SVal < Min || SVal > Max) {
      Ctx.reportError(Fixup.getLoc(), "operand out of range (" + Twine(SVal) +
                                          " not between " + Twine(Min) +
                                          " and " + Twine(Max) + ")");
      return false;
    }
    return true;
  };

  auto HandlePCRelFixupValue = [&](unsigned W) -> uint64_t {
    if (Value % 4 != 0)
      Ctx.reportError(Fixup.getLoc(), "Non-even PC relative offset.");
    if (!CheckFixupInRange(minIntN(W) * 2, maxIntN(W) * 2))
      return 0;
    return (int64_t)Value >> 2;
  };

  switch (unsigned(Kind)) {
  case M88k::FK_88K_DISP16:
    return HandlePCRelFixupValue(16);
  case M88k::FK_88K_DISP26:
    return HandlePCRelFixupValue(26);

  case M88k::FK_88K_HI:
    // %hi16: the upper 16 bits of the (32-bit) target value.
    return (Value >> 16) & 0xffff;
  case M88k::FK_88K_LO:
    // %lo16: the lower 16 bits of the (32-bit) target value.
    return Value & 0xffff;

  case M88k::FK_88K_GOT_HI:
  case M88k::FK_88K_GOT_LO:
  case M88k::FK_88K_ABDIFF_HI:
  case M88k::FK_88K_ABDIFF_LO:
    // The value is only known to the linker; the field stays zero.
    return 0;
  case M88k::FK_88K_NONE:
    return 0;
  }

  llvm_unreachable("Unknown fixup kind!");
}

class M88kMCAsmBackend : public MCAsmBackend {
  uint8_t OSABI;

public:
  M88kMCAsmBackend(uint8_t OSABI)
      : MCAsmBackend(endianness::big), OSABI(OSABI) {}

  // Override MCAsmBackend
  MCFixupKindInfo getFixupKindInfo(MCFixupKind Kind) const override;
  std::optional<MCFixupKind> getFixupKind(StringRef Name) const override;

  void applyFixup(const MCFragment &F, const MCFixup &Fixup,
                  const MCValue &Target, uint8_t *Data, uint64_t Value,
                  bool IsResolved) override;
  bool fixupNeedsRelaxation(const MCFixup &Fixup,
                            uint64_t Value) const override;
  bool writeNopData(raw_ostream &OS, uint64_t Count,
                    const MCSubtargetInfo *STI) const override;

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override {
    return createM88kObjectWriter(OSABI);
  }
};
} // end anonymous namespace

std::optional<MCFixupKind>
M88kMCAsmBackend::getFixupKind(StringRef Name) const {
  unsigned Type = llvm::StringSwitch<unsigned>(Name)
#define ELF_RELOC(X, Y) .Case(#X, Y)
#include "llvm/BinaryFormat/ELFRelocs/M88k.def"
#undef ELF_RELOC
                      .Default(-1u);
  if (Type != -1u)
    return static_cast<MCFixupKind>(FirstLiteralRelocationKind + Type);
  return std::nullopt;
}

MCFixupKindInfo M88kMCAsmBackend::getFixupKindInfo(MCFixupKind Kind) const {
  static const MCFixupKindInfo Infos[M88k::NumTargetFixupKinds] = {
      // This table *must* be in same the order of fixup_* kinds in
      // M88kMCFixups.h.
      // name    offset bits flags
      // The HI/LO fixups are emitted at byte offset 2 within the 4-byte
      // instruction (see M88kMCCodeEmitter::getMachineOpValue), so their
      // in-field bit offset is 0 -- the 16 value bits occupy that whole
      // 2-byte slot.  DISP16/DISP26 are emitted at byte offset 0.
      {"FK_88K_NONE", 0, 32, 0},
      {"FK_88K_DISP16", 16, 16, 0},
      {"FK_88K_DISP26", 6, 26, 0},
      {"FK_88K_HI", 0, 16, 0},
      {"FK_88K_LO", 0, 16, 0},
      {"FK_88K_GOT_HI", 0, 16, 0},
      {"FK_88K_GOT_LO", 0, 16, 0},
      {"FK_88K_ABDIFF_HI", 0, 16, 0},
      {"FK_88K_ABDIFF_LO", 0, 16, 0}};

  if (Kind < FirstTargetFixupKind)
    return MCAsmBackend::getFixupKindInfo(Kind);

  assert(unsigned(Kind - FirstTargetFixupKind) < M88k::NumTargetFixupKinds &&
         "Invalid kind!");
  return Infos[Kind - FirstTargetFixupKind];
}

void M88kMCAsmBackend::applyFixup(const MCFragment &F, const MCFixup &Fixup,
                                  const MCValue &Target, uint8_t *Data,
                                  uint64_t Value, bool IsResolved) {
  MCFixupKind Kind = Fixup.getKind();
  // Branch displacements always get a relocation.
  if (Kind == M88k::FK_88K_DISP16 || Kind == M88k::FK_88K_DISP26)
    IsResolved = false;
  // GOT-relative relocations are always resolved by the linker.
  if (Kind == M88k::FK_88K_GOT_HI || Kind == M88k::FK_88K_GOT_LO ||
      Kind == M88k::FK_88K_ABDIFF_HI || Kind == M88k::FK_88K_ABDIFF_LO)
    IsResolved = false;
  maybeAddReloc(F, Fixup, Target, Value, IsResolved);
  if (mc::isRelocation(Kind))
    return;
  unsigned BitSize = getFixupKindInfo(Kind).TargetSize;
  unsigned Size = (BitSize + 7) / 8;

  // Big-endian insertion of Size bytes.
  Value = extractBitsForFixup(Kind, Value, Fixup, getContext());
  if (BitSize < 64)
    Value &= ((uint64_t)1 << BitSize) - 1;
  unsigned ShiftValue = (Size * 8) - 8;
  for (unsigned I = 0; I != Size; ++I) {
    Data[I] |= uint8_t(Value >> ShiftValue);
    ShiftValue -= 8;
  }
}

bool M88kMCAsmBackend::fixupNeedsRelaxation(const MCFixup &Fixup,
                                            uint64_t Value) const {
  return false;
}

bool M88kMCAsmBackend::writeNopData(raw_ostream &OS, uint64_t Count,
                                    const MCSubtargetInfo *STI) const {
  if ((Count % 4) != 0)
    return false;

  // or %r0,%r0,%r0.  Write whole words; streaming a C string would stop at
  // the embedded NUL.
  for (uint64_t I = 0; I != Count / 4; ++I)
    OS.write("\xf4\x00\x58\x00", 4);

  return true;
}

MCAsmBackend *llvm::createM88kMCAsmBackend(const Target &T,
                                           const MCSubtargetInfo &STI,
                                           const MCRegisterInfo &MRI,
                                           const MCTargetOptions &Options) {
  // Every OpenBSD m88k binary uses ELFOSABI_NONE; OpenBSD is identified by
  // .note.openbsd.ident instead.
  return new M88kMCAsmBackend(ELF::ELFOSABI_NONE);
}
