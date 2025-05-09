//===-- ARMBaseRegisterInfo.h - ARM Register Information Impl ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the base ARM implementation of TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARM_ARMBASEREGISTERINFO_H
#define LLVM_LIB_TARGET_ARM_ARMBASEREGISTERINFO_H

#include "MCTargetDesc/ARMBaseInfo.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/MC/MCRegisterInfo.h"
#include <cstdint>

#define GET_REGINFO_HEADER
#include "ARMGenRegisterInfo.inc"

namespace llvm {

class LiveIntervals;

/// Register allocation hints.
namespace ARMRI {

  enum {
    // Used for LDRD register pairs
    RegPairOdd  = 1,
    RegPairEven = 2,
    // Used to hint for lr in t2DoLoopStart
    RegLR = 3
  };

} // end namespace ARMRI

static inline bool isCalleeSavedRegister(MCRegister Reg,
                                         const MCPhysReg *CSRegs) {
  for (unsigned i = 0; CSRegs[i]; ++i)
    if (Reg == CSRegs[i])
      return true;
  return false;
}

class ARMBaseRegisterInfo : public ARMGenRegisterInfo {
protected:
  /// BasePtr - ARM physical register used as a base ptr in complex stack
  /// frames. I.e., when we need a 3rd base, not just SP and FP, due to
  /// variable size stack objects.
  unsigned BasePtr = ARM::R6;

  // Can be only subclassed.
  explicit ARMBaseRegisterInfo();

public:
  /// Code Generation virtual methods...
  const MCPhysReg *getCalleeSavedRegs(const MachineFunction *MF) const override;
  const MCPhysReg *
  getCalleeSavedRegsViaCopy(const MachineFunction *MF) const;
  const uint32_t *getCallPreservedMask(const MachineFunction &MF,
                                       CallingConv::ID) const override;
  const uint32_t *getNoPreservedMask() const override;
  const uint32_t *getTLSCallPreservedMask(const MachineFunction &MF) const;
  const uint32_t *getSjLjDispatchPreservedMask(const MachineFunction &MF) const;

  /// getThisReturnPreservedMask - Returns a call preserved mask specific to the
  /// case that 'returned' is on an i32 first argument if the calling convention
  /// is one that can (partially) model this attribute with a preserved mask
  /// (i.e. it is a calling convention that uses the same register for the first
  /// i32 argument and an i32 return value)
  ///
  /// Should return NULL in the case that the calling convention does not have
  /// this property
  const uint32_t *getThisReturnPreservedMask(const MachineFunction &MF,
                                             CallingConv::ID) const;

  ArrayRef<MCPhysReg>
  getIntraCallClobberedRegs(const MachineFunction *MF) const override;

  BitVector getReservedRegs(const MachineFunction &MF) const override;
  bool isAsmClobberable(const MachineFunction &MF,
                       MCRegister PhysReg) const override;
  bool isInlineAsmReadOnlyReg(const MachineFunction &MF,
                              MCRegister PhysReg) const override;

  const TargetRegisterClass *
  getPointerRegClass(const MachineFunction &MF,
                     unsigned Kind = 0) const override;
  const TargetRegisterClass *
  getCrossCopyRegClass(const TargetRegisterClass *RC) const override;

  const TargetRegisterClass *
  getLargestLegalSuperClass(const TargetRegisterClass *RC,
                            const MachineFunction &MF) const override;

  unsigned getRegPressureLimit(const TargetRegisterClass *RC,
                               MachineFunction &MF) const override;

  bool getRegAllocationHints(Register VirtReg, ArrayRef<MCPhysReg> Order,
                             SmallVectorImpl<MCPhysReg> &Hints,
                             const MachineFunction &MF, const VirtRegMap *VRM,
                             const LiveRegMatrix *Matrix) const override;

  void updateRegAllocHint(Register Reg, Register NewReg,
                          MachineFunction &MF) const override;

  bool hasBasePointer(const MachineFunction &MF) const;

  bool canRealignStack(const MachineFunction &MF) const override;
  int64_t getFrameIndexInstrOffset(const MachineInstr *MI,
                                   int Idx) const override;
  bool needsFrameBaseReg(MachineInstr *MI, int64_t Offset) const override;
  Register materializeFrameBaseRegister(MachineBasicBlock *MBB, int FrameIdx,
                                        int64_t Offset) const override;
  void resolveFrameIndex(MachineInstr &MI, Register BaseReg,
                         int64_t Offset) const override;
  bool isFrameOffsetLegal(const MachineInstr *MI, Register BaseReg,
                          int64_t Offset) const override;

  bool cannotEliminateFrame(const MachineFunction &MF) const;

  // Debug information queries.
  Register getFrameRegister(const MachineFunction &MF) const override;
  Register getBaseRegister() const { return BasePtr; }

  /// emitLoadConstPool - Emits a load from constpool to materialize the
  /// specified immediate.
  virtual void
  emitLoadConstPool(MachineBasicBlock &MBB, MachineBasicBlock::iterator &MBBI,
                    const DebugLoc &dl, Register DestReg, unsigned SubIdx,
                    int Val, ARMCC::CondCodes Pred = ARMCC::AL,
                    Register PredReg = Register(),
                    unsigned MIFlags = MachineInstr::NoFlags) const;

  /// Code Generation virtual methods...
  bool requiresRegisterScavenging(const MachineFunction &MF) const override;

  bool requiresFrameIndexScavenging(const MachineFunction &MF) const override;

  bool requiresVirtualBaseRegisters(const MachineFunction &MF) const override;

  bool eliminateFrameIndex(MachineBasicBlock::iterator II,
                           int SPAdj, unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override;

  /// SrcRC and DstRC will be morphed into NewRC if this returns true
  bool shouldCoalesce(MachineInstr *MI,
                      const TargetRegisterClass *SrcRC,
                      unsigned SubReg,
                      const TargetRegisterClass *DstRC,
                      unsigned DstSubReg,
                      const TargetRegisterClass *NewRC,
                      LiveIntervals &LIS) const override;

  int getSEHRegNum(unsigned i) const { return getEncodingValue(i); }
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_ARM_ARMBASEREGISTERINFO_H
