//===------ M88k.cpp - Emit LLVM Code for builtins ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This contains code to emit Builtin calls as LLVM code.
//
//===----------------------------------------------------------------------===//
#include "CodeGenFunction.h"
#include "clang/Basic/TargetBuiltins.h"
#include "llvm/IR/IntrinsicsM88k.h"
using namespace clang;
using namespace CodeGen;
using namespace llvm;

Value *CodeGenFunction::EmitM88kBuiltinExpr(unsigned BuiltinID,
                                            const CallExpr *E) {
  switch (BuiltinID) {
  default:
    return nullptr;

  case M88k::BI__builtin_m88k_ff1:
  case M88k::BI__builtin_m88k_ff0: {
    unsigned IID = BuiltinID == M88k::BI__builtin_m88k_ff1
                       ? llvm::Intrinsic::m88k_ff1
                       : llvm::Intrinsic::m88k_ff0;
    llvm::Value *Arg = EmitScalarExpr(E->getArg(0));
    Function *Intr = CGM.getIntrinsic(IID);
    llvm::CallInst *Call = Builder.CreateCall(Intr, Arg);
    Call->setDoesNotAccessMemory();
    return Call;
  }
  }
}
