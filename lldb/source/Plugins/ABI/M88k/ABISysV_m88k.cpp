//===-- ABISysV_m88k.cpp --------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// ABI for the Motorola 88000 (MC88100), as implemented by the M88k LLVM
// backend:
// - r0 is hard-wired to zero.
// - r1 holds the return address.
// - r2..r9 pass the first 8 words of arguments. 64-bit values are passed in an
//   even/odd register pair, the even register holding the high word.
// - r2 (and r3 for 64-bit values) hold the return value. r12 holds the
//   address of a returned structure.
// - r14..r25 and r30 are callee-saved. r30 is the frame pointer, r31 the
//   stack pointer.
// - A frame established by the prologue stores the caller's r30 at [r30] and
//   the return address at [r30 + 4].
//
// The register numbering of the DWARF info matches the backend: r0..r31 are
// DWARF registers 0..31.
//
//===----------------------------------------------------------------------===//

#include "ABISysV_m88k.h"

#include "lldb/Core/PluginManager.h"
#include "lldb/Core/Value.h"
#include "lldb/Symbol/UnwindPlan.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/RegisterContext.h"
#include "lldb/Target/StackFrame.h"
#include "lldb/Target/Target.h"
#include "lldb/Target/Thread.h"
#include "lldb/Utility/ConstString.h"
#include "lldb/Utility/DataExtractor.h"
#include "lldb/Utility/LLDBLog.h"
#include "lldb/Utility/Log.h"
#include "lldb/Utility/RegisterValue.h"
#include "lldb/Utility/Status.h"
#include "lldb/ValueObject/ValueObjectConstResult.h"

#include "llvm/TargetParser/Triple.h"

#include <cstring>

using namespace lldb;
using namespace lldb_private;

LLDB_PLUGIN_DEFINE_ADV(ABISysV_m88k, ABIM88k)

namespace {
enum dwarf_regnums : uint32_t {
  dwarf_r0 = 0,
  dwarf_r1 = 1,
  dwarf_r2 = 2,
  dwarf_r3 = 3,
  dwarf_r12 = 12,
  dwarf_r30 = 30,
  dwarf_r31 = 31,
};
} // namespace

#define GPR(n, alt, generic)                                                   \
  {                                                                            \
    "r" #n, alt, 4, 0, eEncodingUint, eFormatHex,                              \
        {n, n, generic, LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM}, nullptr,    \
        nullptr, nullptr,                                                      \
  }

#define CTL(name, alt, generic)                                                \
  {                                                                            \
    name, alt, 4, 0, eEncodingUint, eFormatHex,                                \
        {LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM, generic,                    \
         LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM},                            \
        nullptr, nullptr, nullptr,                                             \
  }

// The register names match the fallback register layout used by the
// gdb-remote plugin when a stub does not send a target description.
static const RegisterInfo g_register_infos[] = {
    GPR(0, nullptr, LLDB_INVALID_REGNUM),
    GPR(1, "ra", LLDB_REGNUM_GENERIC_RA),
    GPR(2, nullptr, LLDB_REGNUM_GENERIC_ARG1),
    GPR(3, nullptr, LLDB_REGNUM_GENERIC_ARG2),
    GPR(4, nullptr, LLDB_REGNUM_GENERIC_ARG3),
    GPR(5, nullptr, LLDB_REGNUM_GENERIC_ARG4),
    GPR(6, nullptr, LLDB_REGNUM_GENERIC_ARG5),
    GPR(7, nullptr, LLDB_REGNUM_GENERIC_ARG6),
    GPR(8, nullptr, LLDB_REGNUM_GENERIC_ARG7),
    GPR(9, nullptr, LLDB_REGNUM_GENERIC_ARG8),
    GPR(10, nullptr, LLDB_INVALID_REGNUM),
    GPR(11, nullptr, LLDB_INVALID_REGNUM),
    GPR(12, nullptr, LLDB_INVALID_REGNUM),
    GPR(13, nullptr, LLDB_INVALID_REGNUM),
    GPR(14, nullptr, LLDB_INVALID_REGNUM),
    GPR(15, nullptr, LLDB_INVALID_REGNUM),
    GPR(16, nullptr, LLDB_INVALID_REGNUM),
    GPR(17, nullptr, LLDB_INVALID_REGNUM),
    GPR(18, nullptr, LLDB_INVALID_REGNUM),
    GPR(19, nullptr, LLDB_INVALID_REGNUM),
    GPR(20, nullptr, LLDB_INVALID_REGNUM),
    GPR(21, nullptr, LLDB_INVALID_REGNUM),
    GPR(22, nullptr, LLDB_INVALID_REGNUM),
    GPR(23, nullptr, LLDB_INVALID_REGNUM),
    GPR(24, nullptr, LLDB_INVALID_REGNUM),
    GPR(25, nullptr, LLDB_INVALID_REGNUM),
    GPR(26, nullptr, LLDB_INVALID_REGNUM),
    GPR(27, nullptr, LLDB_INVALID_REGNUM),
    GPR(28, nullptr, LLDB_INVALID_REGNUM),
    GPR(29, nullptr, LLDB_INVALID_REGNUM),
    GPR(30, "fp", LLDB_REGNUM_GENERIC_FP),
    GPR(31, "sp", LLDB_REGNUM_GENERIC_SP),
    CTL("pc", "sxip", LLDB_REGNUM_GENERIC_PC),
    CTL("epsr", nullptr, LLDB_REGNUM_GENERIC_FLAGS),
    CTL("ssbr", nullptr, LLDB_INVALID_REGNUM),
    CTL("snip", nullptr, LLDB_INVALID_REGNUM),
    CTL("sfip", nullptr, LLDB_INVALID_REGNUM),
    CTL("vbr", nullptr, LLDB_INVALID_REGNUM),
};

#undef GPR
#undef CTL

const RegisterInfo *ABISysV_m88k::GetRegisterInfoArray(uint32_t &count) {
  count = std::size(g_register_infos);
  return g_register_infos;
}

ABISP ABISysV_m88k::CreateInstance(ProcessSP process_sp, const ArchSpec &arch) {
  if (arch.GetTriple().getArch() == llvm::Triple::m88k)
    return ABISP(
        new ABISysV_m88k(std::move(process_sp), MakeMCRegisterInfo(arch)));
  return ABISP();
}

bool ABISysV_m88k::PrepareTrivialCall(Thread &thread, addr_t sp,
                                      addr_t func_addr, addr_t return_addr,
                                      llvm::ArrayRef<addr_t> args) const {
  RegisterContext *reg_ctx = thread.GetRegisterContext().get();
  if (!reg_ctx)
    return false;

  // Only register arguments are supported.
  if (args.size() > 8)
    return false;

  for (size_t i = 0; i < args.size(); ++i) {
    const RegisterInfo *reg_info = reg_ctx->GetRegisterInfo(
        eRegisterKindGeneric, LLDB_REGNUM_GENERIC_ARG1 + i);
    if (!reg_info || !reg_ctx->WriteRegisterFromUnsigned(reg_info, args[i]))
      return false;
  }

  // The stack pointer must be 16-byte aligned. The callee does not need an
  // argument area because all arguments are passed in registers.
  sp &= ~addr_t(0xf);

  const RegisterInfo *sp_info =
      reg_ctx->GetRegisterInfo(eRegisterKindGeneric, LLDB_REGNUM_GENERIC_SP);
  const RegisterInfo *ra_info =
      reg_ctx->GetRegisterInfo(eRegisterKindGeneric, LLDB_REGNUM_GENERIC_RA);
  const RegisterInfo *pc_info =
      reg_ctx->GetRegisterInfo(eRegisterKindGeneric, LLDB_REGNUM_GENERIC_PC);
  if (!sp_info || !ra_info || !pc_info)
    return false;

  return reg_ctx->WriteRegisterFromUnsigned(sp_info, sp) &&
         reg_ctx->WriteRegisterFromUnsigned(ra_info, return_addr) &&
         reg_ctx->WriteRegisterFromUnsigned(pc_info, func_addr);
}

bool ABISysV_m88k::GetArgumentValues(Thread &thread, ValueList &values) const {
  RegisterContext *reg_ctx = thread.GetRegisterContext().get();
  if (!reg_ctx)
    return false;

  // Integer and pointer arguments of up to 32 bits passed in r2..r9.
  uint32_t next_arg = LLDB_REGNUM_GENERIC_ARG1;
  for (uint32_t i = 0; i < values.GetSize(); ++i) {
    Value *value = values.GetValueAtIndex(i);
    if (!value)
      return false;
    CompilerType type = value->GetCompilerType();
    llvm::Expected<uint64_t> size = type.GetByteSize(&thread);
    if (!size) {
      llvm::consumeError(size.takeError());
      return false;
    }
    if (*size > 4)
      return false;
    bool is_signed = false;
    if (!type.IsIntegerOrEnumerationType(is_signed) && !type.IsPointerType())
      return false;
    if (next_arg > LLDB_REGNUM_GENERIC_ARG8)
      return false;
    const RegisterInfo *reg_info =
        reg_ctx->GetRegisterInfo(eRegisterKindGeneric, next_arg++);
    if (!reg_info)
      return false;
    uint64_t raw = reg_ctx->ReadRegisterAsUnsigned(reg_info, 0);
    Scalar &scalar = value->GetScalar();
    scalar = static_cast<uint32_t>(raw);
    if (!scalar.ExtractBitfield(*size * 8, 0))
      return false;
    if (is_signed)
      scalar.SignExtend(*size * 8);
  }
  return true;
}

Status ABISysV_m88k::SetReturnValueObject(StackFrameSP &frame_sp,
                                          ValueObjectSP &new_value_sp) {
  if (!new_value_sp)
    return Status::FromErrorString("Empty value object for return value.");

  CompilerType compiler_type = new_value_sp->GetCompilerType();
  if (!compiler_type)
    return Status::FromErrorString("Null clang type for return value.");

  bool is_signed = false;
  bool is_complex = false;
  if (!compiler_type.IsIntegerOrEnumerationType(is_signed) &&
      !compiler_type.IsPointerType() &&
      !(compiler_type.IsFloatingPointType(is_complex) && !is_complex))
    return Status::FromErrorString(
        "Returning this type is not supported on m88k.");

  Status error;
  DataExtractor data;
  size_t num_bytes = new_value_sp->GetData(data, error);
  if (error.Fail())
    return Status::FromErrorStringWithFormat(
        "Couldn't convert return value to raw data: %s", error.AsCString());

  if (num_bytes > 8)
    return Status::FromErrorString(
        "Returning values larger than 8 bytes is not supported on m88k.");

  RegisterContext &reg_ctx = *frame_sp->GetThread()->GetRegisterContext();
  const RegisterInfo *r2 = reg_ctx.GetRegisterInfo(eRegisterKindDWARF, dwarf_r2);
  const RegisterInfo *r3 = reg_ctx.GetRegisterInfo(eRegisterKindDWARF, dwarf_r3);
  if (!r2 || !r3)
    return Status::FromErrorString("Couldn't find the return registers.");

  offset_t offset = 0;
  uint64_t raw = data.GetMaxU64(&offset, num_bytes);
  if (num_bytes <= 4) {
    if (!reg_ctx.WriteRegisterFromUnsigned(r2, raw & 0xffffffff))
      return Status::FromErrorString("Couldn't write r2.");
    return Status();
  }
  // 64-bit value: high word in r2, low word in r3.
  if (!reg_ctx.WriteRegisterFromUnsigned(r2, raw >> 32) ||
      !reg_ctx.WriteRegisterFromUnsigned(r3, raw & 0xffffffff))
    return Status::FromErrorString("Couldn't write r2/r3.");
  return Status();
}

ValueObjectSP
ABISysV_m88k::GetReturnValueObjectSimple(Thread &thread,
                                         CompilerType &compiler_type) const {
  if (!compiler_type)
    return ValueObjectSP();

  RegisterContextSP reg_ctx = thread.GetRegisterContext();
  if (!reg_ctx)
    return ValueObjectSP();

  const RegisterInfo *r2 = reg_ctx->GetRegisterInfo(eRegisterKindDWARF, dwarf_r2);
  const RegisterInfo *r3 = reg_ctx->GetRegisterInfo(eRegisterKindDWARF, dwarf_r3);
  if (!r2 || !r3)
    return ValueObjectSP();

  llvm::Expected<uint64_t> byte_size = compiler_type.GetByteSize(&thread);
  if (!byte_size) {
    llvm::consumeError(byte_size.takeError());
    return ValueObjectSP();
  }

  Value value;
  value.SetCompilerType(compiler_type);
  value.SetValueType(Value::ValueType::Scalar);

  const uint32_t type_flags = compiler_type.GetTypeInfo();
  uint64_t hi = reg_ctx->ReadRegisterAsUnsigned(r2, 0) & 0xffffffff;
  uint64_t raw = hi;
  if (*byte_size == 8)
    raw = (hi << 32) | (reg_ctx->ReadRegisterAsUnsigned(r3, 0) & 0xffffffff);
  else if (*byte_size > 8)
    return ValueObjectSP();

  if (type_flags & (eTypeIsInteger | eTypeIsEnumeration)) {
    const bool is_signed = (type_flags & eTypeIsSigned) != 0;
    Scalar &scalar = value.GetScalar();
    switch (*byte_size) {
    case 1:
      scalar = is_signed ? Scalar(int8_t(raw)) : Scalar(uint8_t(raw));
      break;
    case 2:
      scalar = is_signed ? Scalar(int16_t(raw)) : Scalar(uint16_t(raw));
      break;
    case 4:
      scalar = is_signed ? Scalar(int32_t(raw)) : Scalar(uint32_t(raw));
      break;
    case 8:
      scalar = is_signed ? Scalar(int64_t(raw)) : Scalar(uint64_t(raw));
      break;
    default:
      return ValueObjectSP();
    }
  } else if (type_flags & eTypeIsPointer) {
    value.GetScalar() = uint32_t(raw);
  } else if (type_flags & eTypeIsFloat) {
    bool is_complex = false;
    if (!compiler_type.IsFloatingPointType(is_complex) || is_complex)
      return ValueObjectSP();
    if (*byte_size == 4) {
      uint32_t bits = uint32_t(raw);
      float f;
      std::memcpy(&f, &bits, sizeof(f));
      value.GetScalar() = f;
    } else if (*byte_size == 8) {
      double d;
      std::memcpy(&d, &raw, sizeof(d));
      value.GetScalar() = d;
    } else {
      return ValueObjectSP();
    }
  } else {
    return ValueObjectSP();
  }

  return ValueObjectConstResult::Create(thread.GetStackFrameAtIndex(0).get(),
                                        value, ConstString(""));
}

ValueObjectSP
ABISysV_m88k::GetReturnValueObjectImpl(Thread &thread,
                                       CompilerType &return_compiler_type) const {
  if (!return_compiler_type)
    return ValueObjectSP();
  return GetReturnValueObjectSimple(thread, return_compiler_type);
}

// At the first instruction of a function the CFA is the stack pointer and the
// return address is in r1.
UnwindPlanSP ABISysV_m88k::CreateFunctionEntryUnwindPlan() {
  UnwindPlan::Row row;
  row.GetCFAValue().SetIsRegisterPlusOffset(dwarf_r31, 0);
  row.SetRegisterLocationToIsCFAPlusOffset(dwarf_r31, 0, true);
  row.SetRegisterLocationToSame(dwarf_r1, false);
  row.SetRegisterLocationToSame(dwarf_r30, false);
  auto plan_sp = std::make_shared<UnwindPlan>(eRegisterKindDWARF);
  plan_sp->AppendRow(std::move(row));
  plan_sp->SetReturnAddressRegister(dwarf_r1);
  plan_sp->SetSourceName("m88k at-func-entry default");
  plan_sp->SetSourcedFromCompiler(eLazyBoolNo);
  plan_sp->SetUnwindPlanValidAtAllInstructions(eLazyBoolNo);
  plan_sp->SetUnwindPlanForSignalTrap(eLazyBoolNo);
  return plan_sp;
}

// Frame pointer based unwinding after the prologue. The prologue stores the
// caller's r30 at [r30] and the return address at [r30 + 4]. The distance
// between r30 and the incoming stack pointer depends on the frame size, so
// the CFA used here is r30 itself; the caller's stack pointer is unknown.
UnwindPlanSP ABISysV_m88k::CreateDefaultUnwindPlan() {
  UnwindPlan::Row row;
  row.GetCFAValue().SetIsRegisterPlusOffset(dwarf_r30, 0);
  row.SetRegisterLocationToAtCFAPlusOffset(dwarf_r30, 0, true);
  row.SetRegisterLocationToAtCFAPlusOffset(dwarf_r1, 4, true);
  auto plan_sp = std::make_shared<UnwindPlan>(eRegisterKindDWARF);
  plan_sp->AppendRow(std::move(row));
  plan_sp->SetReturnAddressRegister(dwarf_r1);
  plan_sp->SetSourceName("m88k default unwind plan");
  plan_sp->SetSourcedFromCompiler(eLazyBoolNo);
  plan_sp->SetUnwindPlanValidAtAllInstructions(eLazyBoolNo);
  plan_sp->SetUnwindPlanForSignalTrap(eLazyBoolNo);
  return plan_sp;
}

bool ABISysV_m88k::RegisterIsVolatile(const RegisterInfo *reg_info) {
  if (!reg_info)
    return true;
  uint32_t reg = reg_info->kinds[eRegisterKindDWARF];
  if (reg == LLDB_INVALID_REGNUM)
    return true;
  // r14..r25, r30 and r31 are preserved across calls.
  if ((reg >= 14 && reg <= 25) || reg == dwarf_r30 || reg == dwarf_r31)
    return false;
  return true;
}

void ABISysV_m88k::Initialize() {
  PluginManager::RegisterPlugin(GetPluginNameStatic(),
                                "System V ABI for m88k targets", CreateInstance);
}

void ABISysV_m88k::Terminate() {
  PluginManager::UnregisterPlugin(CreateInstance);
}
