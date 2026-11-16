//===- M88k.cpp ----------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ABIInfoImpl.h"
#include "TargetInfo.h"

using namespace clang;
using namespace clang::CodeGen;

//===----------------------------------------------------------------------===//
// M88k ABI Implementation
//
// This is a very simple ABI that relies a lot on DefaultABIInfo.
//===----------------------------------------------------------------------===//

namespace {
class M88kABIInfo final : public ABIInfo {

public:
  explicit M88kABIInfo(CodeGen::CodeGenTypes &CGT) : ABIInfo(CGT) {}

  ABIArgInfo classifyReturnType(QualType RetTy) const;
  ABIArgInfo classifyArgumentType(QualType RetTy) const;

  void computeInfo(CGFunctionInfo &FI) const override;

  RValue EmitVAArg(CodeGen::CodeGenFunction &CGF, CodeGen::Address VAListAddr,
                   QualType Ty, AggValueSlot Slot) const override;
};

class M88kTargetCodeGenInfo final : public TargetCodeGenInfo {
public:
  explicit M88kTargetCodeGenInfo(CodeGen::CodeGenTypes &CGT)
      : TargetCodeGenInfo(std::make_unique<M88kABIInfo>(CGT)) {}
};
} // namespace

ABIArgInfo M88kABIInfo::classifyArgumentType(QualType Ty) const {
  Ty = useFirstFieldIfTransparentUnion(Ty);

  if (isAggregateTypeForABI(Ty)) {
    // Records with non-trivial destructors/copy-constructors should not be
    // passed by value.
    if (CGCXXABI::RecordArgABI RAA = getRecordArgABI(Ty, getCXXABI()))
      return getNaturalAlignIndirect(Ty, RAA == CGCXXABI::RAA_DirectInMemory);

    return getNaturalAlignIndirect(Ty);
  }

  // Treat an enum type as its underlying type.
  if (const EnumType *EnumTy = Ty->getAs<EnumType>())
    Ty = EnumTy->getDecl()->getIntegerType();

  ASTContext &Context = getContext();
  if (const auto *EIT = Ty->getAs<BitIntType>())
    if (EIT->getNumBits() >
        Context.getTypeSize(Context.getTargetInfo().hasInt128Type()
                                ? Context.Int128Ty
                                : Context.LongLongTy))
      return getNaturalAlignIndirect(Ty);

  return (isPromotableIntegerTypeForABI(Ty) ? ABIArgInfo::getExtend(Ty)
                                            : ABIArgInfo::getDirect());
}

ABIArgInfo M88kABIInfo::classifyReturnType(QualType RetTy) const {
  if (RetTy->isVoidType())
    return ABIArgInfo::getIgnore();

  if (isAggregateTypeForABI(RetTy))
    return getNaturalAlignIndirect(RetTy);

  // Treat an enum type as its underlying type.
  if (const EnumType *EnumTy = RetTy->getAs<EnumType>())
    RetTy = EnumTy->getDecl()->getIntegerType();

  if (const auto *EIT = RetTy->getAs<BitIntType>())
    if (EIT->getNumBits() >
        getContext().getTypeSize(getContext().getTargetInfo().hasInt128Type()
                                     ? getContext().Int128Ty
                                     : getContext().LongLongTy))
      return getNaturalAlignIndirect(RetTy);

  return (isPromotableIntegerTypeForABI(RetTy) ? ABIArgInfo::getExtend(RetTy)
                                               : ABIArgInfo::getDirect());
}

void M88kABIInfo::computeInfo(CGFunctionInfo &FI) const {
  if (!getCXXABI().classifyReturnType(FI))
    FI.getReturnInfo() = classifyReturnType(FI.getReturnType());
  for (auto &I : FI.arguments())
    I.info = classifyArgumentType(I.type);
}

RValue M88kABIInfo::EmitVAArg(CodeGen::CodeGenFunction &CGF,
                              CodeGen::Address VAListAddr, QualType Ty,
                              AggValueSlot Slot) const {
  // The va_list type is (88open BCS):
  // struct {
  //    int __va_arg;   // argument word index of the next argument
  //    int *__va_stk;  // word N >= 8 is at __va_stk + 4 * N
  //    int *__va_reg;  // word N < 8 (r2-r9) is at __va_reg + 4 * N
  // };
  // Arguments occupy whole words. 8-byte aligned arguments start at an even
  // word index. An argument that does not fit into the remaining argument
  // registers is on the stack. Aggregates are passed indirectly.
  Ty = getContext().getCanonicalType(Ty);
  bool Indirect = isAggregateTypeForABI(Ty);
  auto TyInfo = getContext().getTypeInfoInChars(Ty);
  CharUnits Size = Indirect ? CharUnits::fromQuantity(4) : TyInfo.Width;
  bool Align8 = !Indirect && TyInfo.Align > CharUnits::fromQuantity(4);
  uint64_t Words = (Size.getQuantity() + 3) / 4;

  CGBuilderTy &B = CGF.Builder;
  llvm::Type *I32 = CGF.Int32Ty;
  Address VaArgPtr = B.CreateStructGEP(VAListAddr, 0, "__va_arg.addr");
  llvm::Value *Idx = B.CreateLoad(VaArgPtr, "__va_arg");
  if (Align8)
    Idx = B.CreateAnd(B.CreateAdd(Idx, llvm::ConstantInt::get(I32, 1)),
                      llvm::ConstantInt::get(I32, -2), "__va_arg.aligned");
  llvm::Value *Next =
      B.CreateAdd(Idx, llvm::ConstantInt::get(I32, Words), "__va_arg.next");
  B.CreateStore(Next, VaArgPtr);
  llvm::Value *InRegs =
      B.CreateICmpULE(Next, llvm::ConstantInt::get(I32, 8), "fits_in_regs");
  Address VaRegPtr = B.CreateStructGEP(VAListAddr, 2, "__va_reg.addr");
  Address VaStkPtr = B.CreateStructGEP(VAListAddr, 1, "__va_stk.addr");
  llvm::Value *Base =
      B.CreateSelect(InRegs, B.CreateLoad(VaRegPtr, "__va_reg"),
                     B.CreateLoad(VaStkPtr, "__va_stk"), "va_base");
  llvm::Value *Off = B.CreateMul(Idx, llvm::ConstantInt::get(I32, 4));
  // Big endian: values smaller than a word are in the low-order bytes.
  if (Size.getQuantity() < 4)
    Off = B.CreateAdd(Off,
                      llvm::ConstantInt::get(I32, 4 - Size.getQuantity()));
  llvm::Value *Ptr = B.CreateInBoundsGEP(CGF.Int8Ty, Base, Off, "va_arg.addr");
  CharUnits SlotAlign = CharUnits::fromQuantity(Align8 ? 8 : 4);
  if (Size.getQuantity() < 4)
    SlotAlign = Size;
  Address Addr(Ptr, CGF.ConvertTypeForMem(Indirect ? getContext().VoidPtrTy
                                                   : Ty),
               SlotAlign);
  if (Indirect)
    Addr = Address(B.CreateLoad(Addr, "va_arg.indirect"),
                   CGF.ConvertTypeForMem(Ty), TyInfo.Align);
  return CGF.EmitLoadOfAnyValue(CGF.MakeAddrLValue(Addr, Ty), Slot);
}
std::unique_ptr<TargetCodeGenInfo>
CodeGen::createM88kTargetCodeGenInfo(CodeGenModule &CGM) {
  return std::make_unique<M88kTargetCodeGenInfo>(CGM.getTypes());
}
