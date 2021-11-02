/*
 * Copyright (c) 2007-2008 The Florida State University
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

#ifndef __ARCH_ARM_REGS_VEC_HH__
#define __ARCH_ARM_REGS_VEC_HH__

#include "arch/power/types.hh"
#include "arch/generic/vec_pred_reg.hh"
#include "arch/generic/vec_reg.hh"

namespace gem5
{

namespace PowerISA
{

// Number of VecElem per Vector Register considering only pre-SVE
// Number of VecElem per Vector Register, computed based on the vector length
constexpr unsigned NumVecElemPerVecReg = 4;

using VecElem = uint32_t;
using VecRegContainer =
    gem5::VecRegContainer<NumVecElemPerVecReg * sizeof(VecElem)>;

using VecPredReg =
    gem5::VecPredRegT<VecElem, NumVecElemPerVecReg, false, false>;
using ConstVecPredReg =
    gem5::VecPredRegT<VecElem, NumVecElemPerVecReg, false, true>;
using VecPredRegContainer = VecPredReg::Container;

// Vec, PredVec
// NumFloatV7ArchRegs: This in theory should be 32.
// However in A32 gem5 is splitting double register accesses in two
// subsequent single register ones. This means we would use a index
// bigger than 31 when accessing D16-D31.

const int NumVecRegs = 64;
const int NumVecPredRegs = 64;  // P0-P15, FFR, UREG0

} // namespace PowerISA
} // namespace gem5

#endif
