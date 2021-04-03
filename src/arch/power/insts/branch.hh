/* Copyright (c) 2007-2008 The Florida State University
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

#ifndef __ARCH_POWER_INSTS_BRANCH_HH__
#define __ARCH_POWER_INSTS_BRANCH_HH__

#include "arch/power/insts/static_inst.hh"

namespace PowerISA
{

/**
 * Base class for instructions whose disassembly is not purely a
 * function of the machine instruction (i.e., it depends on the
 * PC).  This class overrides the disassemble() method to check
 * the PC and symbol table values before re-using a cached
 * disassembly string.  This is necessary for branches and jumps,
 * where the disassembly string includes the target address (which
 * may depend on the PC and/or symbol table).
 */
class PCDependentDisassembly : public PowerStaticInst
{
  protected:
    /// Cached program counter from last disassembly
    mutable Addr cachedPC;
    /// Cached symbol table pointer from last disassembly
    mutable const Loader::SymbolTable *cachedSymtab;

    /// Constructor
    PCDependentDisassembly(const char *mnem, ExtMachInst _machInst,
                           OpClass __opClass)
        : PowerStaticInst(mnem, _machInst, __opClass),
          cachedPC(0), cachedSymtab(0)
    {
    }

    const std::string &
    disassemble(Addr pc, const Loader::SymbolTable *symtab) const;
};


/**
 * Base class for unconditional, PC-relative or absolute address branches.
 */
class BranchOp : public PCDependentDisassembly
{
  protected:

    bool aaSet;
    bool lkSet;
    uint64_t disp;

    /// Constructor
    BranchOp(const char *mnem, MachInst _machInst, OpClass __opClass)
      : PCDependentDisassembly(mnem, _machInst, __opClass),
        aaSet(false),
        lkSet(false),
        disp(sext<26>(machInst.li << 2))
    {
    }

    PowerISA::PCState branchTarget(const PowerISA::PCState &pc) const override;

    /// Explicitly import the otherwise hidden branchTarget
    using StaticInst::branchTarget;

    std::string generateDisassembly(
            Addr pc, const Loader::SymbolTable *symtab) const override;
};


/**
 * Base class for conditional branches.
 */
class BranchCondOp : public PCDependentDisassembly
{
  protected:

    bool lkSet;
    uint32_t crBit;
    uint32_t opts;

    /// Constructor
    BranchCondOp(const char *mnem, MachInst _machInst, OpClass __opClass)
      : PCDependentDisassembly(mnem, _machInst, __opClass),
        lkSet(false),
        crBit(machInst.bi),
        opts(machInst.bo)
    {
    }

    inline bool
    checkCtr(uint64_t& ctr) const
    {
        bool ctrOk;
        if (opts & 0x4) {
            ctrOk = true;
        } else {
            ctr--;
            if (ctr != 0) {
                ctrOk = ((opts & 0x2) == 0);
            } else {
                ctrOk = ((opts & 0x2) != 0);
            }
        }
        return ctrOk;
    }

    inline bool
    checkCond(uint32_t cr) const
    {
        bool condOk;
        if (opts & 0x10) {
            condOk = true;
        } else {
            condOk = (((cr >> (31 - crBit)) & 0x1) == ((opts >> 3) & 0x1));
        }
        return condOk;
    }
};


/**
 * Base class for conditional, PC-relative or absolute address branches.
 */
class BranchDispCondOp : public BranchCondOp
{
  protected:

    bool aaSet;
    uint64_t disp;

    /// Constructor
    BranchDispCondOp(const char *mnem, MachInst _machInst, OpClass __opClass)
      : BranchCondOp(mnem, _machInst, __opClass),
        aaSet(false),
        disp(sext<16>(machInst.bd << 2))
    {
    }

    PowerISA::PCState branchTarget(const PowerISA::PCState &pc) const override;

    /// Explicitly import the otherwise hidden branchTarget
    using StaticInst::branchTarget;

    std::string generateDisassembly(
            Addr pc, const Loader::SymbolTable *symtab) const override;
};


/**
 * Base class for conditional, register-based branches.
 */
class BranchRegCondOp : public BranchCondOp
{
  protected:

    // TODO: For now, the hint value (BH) is ignored and always considered
    //       to be zero. Instruction flags should vary depending on this.
    uint32_t hint;

    /// Constructor.
    BranchRegCondOp(const char *mnem, MachInst _machInst, OpClass __opClass)
      : BranchCondOp(mnem, _machInst, __opClass),
        hint(machInst.bh)
    {
    }

    PowerISA::PCState branchTarget(ThreadContext *tc) const override;

    /// Explicitly import the otherwise hidden branchTarget
    using StaticInst::branchTarget;

    std::string generateDisassembly(
            Addr pc, const Loader::SymbolTable *symtab) const override;
};

} // namespace PowerISA

#endif //__ARCH_POWER_INSTS_BRANCH_HH__
