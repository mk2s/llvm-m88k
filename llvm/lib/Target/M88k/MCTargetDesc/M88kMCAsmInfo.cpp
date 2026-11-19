//===-- M88kMCAsmInfo.cpp - M88k asm properties ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "M88kMCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

M88kMCAsmInfo::M88kMCAsmInfo(const Triple &TT) {
  // TODO: Check!
  CodePointerSize = 4;
  CalleeSaveStackSlotSize = 4;
  IsLittleEndian = false;
  MinInstAlignment = 4;

  CommentString = "|"; // # as comment delimiter is only allowed at first column
  ZeroDirective = "\t.space\t";
  Data64bitsDirective = "\t.quad\t";
  UsesELFSectionDirectiveForBSS = true;
  SupportsDebugInformation = true;
  ExceptionsType = ExceptionHandling::SjLj;
  // SjLj EH does not use CFI. Emit CFI into .debug_frame for debuggers.
  UsesCFIForDebug = true;
}

void M88kMCAsmInfo::printSpecifierExpr(raw_ostream &OS,
                                       const MCSpecifierExpr &Expr) const {
  if (Expr.getSpecifier() == M88k::S_None) {
    printExpr(OS, *Expr.getSubExpr());
    return;
  }
  switch (Expr.getSpecifier()) {
  default:
    llvm_unreachable("Invalid kind!");
  case M88k::S_ABS_HI:
    OS << "%hi16";
    break;
  case M88k::S_ABS_LO:
    OS << "%lo16";
    break;
  case M88k::S_GOT_HI:
    OS << "%got_hi16";
    break;
  case M88k::S_GOT_LO:
    OS << "%got_lo16";
    break;
  case M88k::S_ABDIFF_HI:
    OS << "%abdiff_hi16";
    break;
  case M88k::S_ABDIFF_LO:
    OS << "%abdiff_lo16";
    break;
  }
  OS << '(';
  printExpr(OS, *Expr.getSubExpr());
  OS << ')';
}

bool M88kMCAsmInfo::evaluateAsRelocatableImpl(const MCSpecifierExpr &Expr,
                                              MCValue &Res,
                                              const MCAssembler *Asm) const {
  // Like the former M88kMCExpr: keep both symbols of the sub expression.
  if (!Expr.getSubExpr()->evaluateAsRelocatable(Res, Asm))
    return false;
  Res.setSpecifier(Expr.getSpecifier());
  return true;
}
