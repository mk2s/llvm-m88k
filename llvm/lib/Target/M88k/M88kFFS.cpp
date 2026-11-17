//===-- M88kFFS.cpp - Identify and rewrite FFS for M88k -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// clang lowers a call to __builtin_ffs() in the following way:
//
//   ffs(x) -> x ? cttz(x) + 1 : 0
//
// This pass detects this instruction sequence, and reqrites it using the
// intrinsic llvm.m88k.ff1. It results in more compact and more performant code
// on m88k.
//
// The IR is replaced with:
//
//   %neg = sub i32 0, %x
//   %and = and i32 %x, %neg                  ; isolate the lowest set bit
//   %ff1 = tail call i32 @llvm.m88k.ff1(i32 %and)
//   %add = add i32 %ff1, 1
//   %nz = lshr i32 %ff1, 5                   ; ff1 is 32 only if %x is 0
//   %mask = add i32 %nz, -1                  ; all ones if %x != 0, else 0
//   %res = and i32 %add, %mask
//
// ff1 returns the bit number of the most significant set bit, or 32 if no bit
// is set. A previous version computed ff1(2 * (x & -x) + (x != 0)), which
// is wrong for x == 0 (ff1(0) == 32, not 0) and for x == 0x80000000 (the
// multiplication by 2 overflows, giving ff1(1) == 0 instead of 32).
//
//===----------------------------------------------------------------------===//

#include "M88k.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicsM88k.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"
#include <cassert>

#define DEBUG_TYPE "m88k-ffs"

using namespace llvm;
using namespace llvm::PatternMatch;

STATISTIC(RewrittenFFS, "Number of rewritten FFS patterns");

namespace {
class M88kFFS : public FunctionPass {

public:
  static char ID;

  M88kFFS();

  bool runOnFunction(Function &MF) override;

  bool runOnBasicBlock(BasicBlock &BB);
};
} // end anonymous namespace

M88kFFS::M88kFFS() : FunctionPass(ID) {
  initializeM88kFFSPass(*PassRegistry::getPassRegistry());
}

bool M88kFFS::runOnFunction(Function &MF) {
  bool Changed = false;
  for (BasicBlock &BB : MF)
    Changed |= runOnBasicBlock(BB);

  return Changed;
}

// Iterate the def-use chain, and add all instructions to the list.
static void addToEraseList(Instruction *I,
                           SmallVectorImpl<Instruction *> &EraseList) {
  for (User *U : I->users()) {
    if (Instruction *Inst = dyn_cast<Instruction>(U)) {
      addToEraseList(Inst, EraseList);
    }
  }
  EraseList.push_back(I);
}

bool M88kFFS::runOnBasicBlock(BasicBlock &BB) {
  bool Changed = false;

  Module &M = *BB.getParent()->getParent();
  auto &Ctx = M.getContext();
  IRBuilder<> Builder(Ctx);

  Type *Int32Ty = Type::getInt32Ty(Ctx);

  Value *Input;
  Instruction *Cttz = nullptr;

  SmallVector<Instruction *, 4> EraseList;

  for (BasicBlock::iterator I = BB.begin(); I != BB.end(); ++I) {
    // Check if instruction is llvm.cttz.32(i32 %x, i1 true).
    if (match(&(*I), m_Intrinsic<Intrinsic::cttz>(m_Value(Input), m_One())) &&
        Input->getType()->isIntegerTy(32)) {
      Cttz = &(*I);
    }

    // Do not check the pattern without a call to llvm.cttz.32.
    if (!Cttz)
      continue;

    CmpPredicate Pred = CmpPredicate(ICmpInst::ICMP_EQ);
    if (match(&(*I),
              m_Select(m_OneUse(m_c_ICmp(Pred, m_Specific(Input), m_ZeroInt())),
                       m_ZeroInt(),
                       m_OneUse(m_Add(m_OneUse(m_Specific(Cttz)), m_One())))) &&
        Pred == ICmpInst::ICMP_EQ) {

      Builder.SetInsertPoint(Cttz);

      auto *Neg = Builder.CreateNeg(Input);
      auto *And = Builder.CreateAnd(Input, Neg);
      auto *FF1 = Builder.CreateIntrinsic(Int32Ty, Intrinsic::m88k_ff1, {And});
      auto *Plus1 = Builder.CreateAdd(FF1, ConstantInt::get(Int32Ty, 1));
      auto *NotFound = Builder.CreateLShr(FF1, ConstantInt::get(Int32Ty, 5));
      auto *Mask =
          Builder.CreateAdd(NotFound, Constant::getAllOnesValue(Int32Ty));
      auto *Res = Builder.CreateAnd(Plus1, Mask);
      I->replaceAllUsesWith(Res);

      addToEraseList(Cttz, EraseList);
      Cttz = nullptr;

      ++RewrittenFFS;
      Changed = true;
    }
  }

  for (Instruction *I : EraseList)
    I->eraseFromParent();

  return Changed;
}

char M88kFFS::ID = 0;
INITIALIZE_PASS(M88kFFS, DEBUG_TYPE, "Rewrite FFS patterns", false, false)

namespace llvm {
FunctionPass *createM88kFFS() { return new M88kFFS(); }
} // end namespace llvm
