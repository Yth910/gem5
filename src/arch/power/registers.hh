/*
 * Copyright (c) 2009 The University of Edinburgh
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __ARCH_POWER_REGISTERS_HH__
#define __ARCH_POWER_REGISTERS_HH__

#include "arch/generic/vec_pred_reg.hh"
#include "arch/generic/vec_reg.hh"
#include "arch/power/miscregs.hh"
#include "base/types.hh"

namespace PowerISA
{

// Not applicable to Power
constexpr unsigned NumVecElemPerVecReg = 4;

using VecElem = uint32_t;
using VecReg = ::VecRegT<VecElem, NumVecElemPerVecReg, false>;
using ConstVecReg = ::VecRegT<VecElem, NumVecElemPerVecReg, true>;
using VecRegContainer = VecReg::Container;
//constexpr unsigned NumVecElemPerVecReg = ::DummyNumVecElemPerVecReg;
constexpr size_t VecRegSizeBytes = 16;

constexpr size_t VecPredRegSizeBits = ::DummyVecPredRegSizeBits;
constexpr bool VecPredRegHasPackedRepr = ::DummyVecPredRegHasPackedRepr;
using VecPredReg = ::VecPredRegT<VecElem, NumVecElemPerVecReg,
                                 VecPredRegHasPackedRepr, false>;
using ConstVecPredReg = ::VecPredRegT<VecElem, NumVecElemPerVecReg,
                                      VecPredRegHasPackedRepr, true>;
using VecPredRegContainer = VecPredReg::Container;

// Constants Related to the number of registers
const int NumIntArchRegs = 32;

// CR, XER, LR, CTR, TAR, FPSCR, RSV, RSV-LEN, RSV-ADDR
// and zero register, which doesn't actually exist but needs a number
const int NumIntSpecialRegs = 89;
const int NumFloatArchRegs = 32;

const int NumIntRegs = NumIntArchRegs + NumIntSpecialRegs;
const int NumFloatRegs = NumFloatArchRegs;
const int NumVecRegs = 64;
const int NumVecScalarRegs = 64;
const int NumVecPredRegs = 1;  // Not applicable to Power
                               // (1 to prevent warnings)
const int NumCCRegs = 0;
const int NumMiscRegs = NUM_MISCREGS;

// Semantically meaningful register indices
const int ReturnValueReg = 3;
const int StackPointerReg = 1;

// There isn't one in Power, but we need to define one somewhere
const int ZeroReg = NumIntRegs - 1;

enum MiscIntRegNums {
    INTREG_CR = NumIntArchRegs,
    INTREG_XER,
    INTREG_LR,
    INTREG_CTR,
    INTREG_TAR,
    INTREG_FPSCR,
    INTREG_RSV,
    INTREG_RSV_LEN,
    INTREG_RSV_ADDR,
    INTREG_DSCR,
    INTREG_DSISR,
    INTREG_DAR,
    INTREG_DEC,
    INTREG_SRR0,
    INTREG_SRR1,
    INTREG_CFAR,
    INTREG_CTRL,
    INTREG_VRSAVE,
    INTREG_TB,
    INTREG_TBL,
    INTREG_TBU,
    INTREG_PVR,
    INTREG_PPR,
    INTREG_PPR32,
    INTREG_MSR,
    INTREG_SPRG0,
    INTREG_SPRG1,
    INTREG_SPRG2,
    INTREG_SPRG3,
    INTREG_LPCR,
    INTREG_PTCR,
    INTREG_FSCR,
    INTREG_MMCRA,
    INTREG_MMCR0,
    INTREG_MMCR1,
    INTREG_MMCR2,
    INTREG_PSSCR,
    INTREG_LPIDR,
    INTREG_PIDR,
    INTREG_HFSCR,
    INTREG_HSPRG0,
    INTREG_HIR0,
    INTREG_MMCRC,
    INTREG_AMOR,
    INTREG_IAMR,
    INTREG_HSRR0,
    INTREG_HSRR1,
    INTREG_HDEC,
    INTREG_PCR,
    INTREG_AMR,
    INTREG_TFHAR,
    INTREG_TFIAR,
    INTREG_TEXASR,
    INTREG_TIDR,
    INTREG_UAMOR,
    INTREG_DPDES,
    INTREG_DAWR0,
    INTREG_RPR,
    INTREG_CIABR,
    INTREG_CIR,
    INTREG_TBU40,
    INTREG_SPURR,
    INTREG_PURR,
    INTREG_HDSISR,
    INTREG_HDAR,
    INTREG_HRMOR,
    INTREG_HMER,
    INTREG_HMEER,
    INTREG_SIER,
    INTREG_PMC1,
    INTREG_PMC2,
    INTREG_PMC3,
    INTREG_PMC4,
    INTREG_PMC5,
    INTREG_PMC6,
    INTREG_SDAR,
    INTREG_BESCRS,
    INTREG_BESCRSU,
    INTREG_BESCRR,
    INTREG_BESCRRU,
    INTREG_EBBHR,
    INTREG_EBBRR,
    INTREG_BESCR,
    INTREG_ASDR,
    INTREG_IC,
    INTREG_VTB,
    INTREG_HSPRG1,
    INTREG_PIR,
    INTREG_VSCR,
    INTREG_DUMMY
};

} // namespace PowerISA

#endif // __ARCH_POWER_REGISTERS_HH__
