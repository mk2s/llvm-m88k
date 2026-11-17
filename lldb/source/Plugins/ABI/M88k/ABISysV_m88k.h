//===-- ABISysV_m88k.h ------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLDB_SOURCE_PLUGINS_ABI_M88K_ABISYSV_M88K_H
#define LLDB_SOURCE_PLUGINS_ABI_M88K_ABISYSV_M88K_H

#include "lldb/Target/ABI.h"
#include "lldb/lldb-private.h"

class ABISysV_m88k : public lldb_private::RegInfoBasedABI {
public:
  ~ABISysV_m88k() override = default;

  size_t GetRedZoneSize() const override { return 0; }

  bool PrepareTrivialCall(lldb_private::Thread &thread, lldb::addr_t sp,
                          lldb::addr_t functionAddress,
                          lldb::addr_t returnAddress,
                          llvm::ArrayRef<lldb::addr_t> args) const override;

  bool GetArgumentValues(lldb_private::Thread &thread,
                         lldb_private::ValueList &values) const override;

  lldb_private::Status
  SetReturnValueObject(lldb::StackFrameSP &frame_sp,
                       lldb::ValueObjectSP &new_value) override;

  lldb::ValueObjectSP
  GetReturnValueObjectImpl(lldb_private::Thread &thread,
                           lldb_private::CompilerType &type) const override;

  bool
  CreateFunctionEntryUnwindPlan(lldb_private::UnwindPlan &unwind_plan) override;

  bool CreateDefaultUnwindPlan(lldb_private::UnwindPlan &unwind_plan) override;

  bool RegisterIsVolatile(const lldb_private::RegisterInfo *reg_info) override;

  // The stack pointer is kept 16-byte aligned by the ABI, but the frame
  // pointer based CFA used by the default unwind plan is only 8-byte aligned.
  bool CallFrameAddressIsValid(lldb::addr_t cfa) override {
    return cfa != 0 && (cfa & 0x7) == 0;
  }

  // Instructions are 4 bytes and 4-byte aligned.
  bool CodeAddressIsValid(lldb::addr_t pc) override { return (pc & 0x3) == 0; }

  // The two low bits of r1 and of the exception-time instruction pointers are
  // not part of the address.
  lldb::addr_t FixCodeAddress(lldb::addr_t pc) override { return pc & ~3ull; }

  const lldb_private::RegisterInfo *
  GetRegisterInfoArray(uint32_t &count) override;

  bool GetPointerReturnRegister(const char *&name) override {
    name = "r2";
    return true;
  }

  // Static Functions

  static void Initialize();

  static void Terminate();

  static lldb::ABISP CreateInstance(lldb::ProcessSP process_sp,
                                    const lldb_private::ArchSpec &arch);

  static llvm::StringRef GetPluginNameStatic() { return "sysv-m88k"; }

  // PluginInterface protocol

  llvm::StringRef GetPluginName() override { return GetPluginNameStatic(); }

protected:
  lldb::ValueObjectSP
  GetReturnValueObjectSimple(lldb_private::Thread &thread,
                             lldb_private::CompilerType &ast_type) const;

private:
  using lldb_private::RegInfoBasedABI::RegInfoBasedABI;
};

#endif // LLDB_SOURCE_PLUGINS_ABI_M88K_ABISYSV_M88K_H
