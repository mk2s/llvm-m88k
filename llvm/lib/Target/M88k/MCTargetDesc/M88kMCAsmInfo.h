//====-- M88kMCAsmInfo.h - M88k asm properties ---------------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_M88K_MCTARGETDESC_M88KMCASMINFO_H
#define LLVM_LIB_TARGET_M88K_MCTARGETDESC_M88KMCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {
class Triple;

class M88kMCAsmInfo : public MCAsmInfoELF {
public:
  explicit M88kMCAsmInfo(const Triple &TT);
  void printSpecifierExpr(raw_ostream &OS,
                          const MCSpecifierExpr &Expr) const override;
  bool evaluateAsRelocatableImpl(const MCSpecifierExpr &Expr, MCValue &Res,
                                 const MCAssembler *Asm) const override;
};

namespace M88k {
using Specifier = uint8_t;
enum { S_None, S_ABS_HI, S_ABS_LO };
} // namespace M88k

} // end namespace llvm

#endif
