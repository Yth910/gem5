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

#include "arch/power/insts/branch.hh"

#include "base/loader/symtab.hh"
#include "cpu/thread_context.hh"

using namespace PowerISA;

const std::string &
PCDependentDisassembly::disassemble(
        Addr pc, const Loader::SymbolTable *symtab) const
{
    if (!cachedDisassembly ||
        pc != cachedPC || symtab != cachedSymtab)
    {
        if (cachedDisassembly)
            delete cachedDisassembly;

        cachedDisassembly =
            new std::string(generateDisassembly(pc, symtab));
        cachedPC = pc;
        cachedSymtab = symtab;
    }

    return *cachedDisassembly;
}


PowerISA::PCState
BranchOp::branchTarget(const PowerISA::PCState &pc) const
{
    if (aaSet) {
        return disp;
    } else {
        return pc.pc() + disp;
    }
}


std::string
BranchOp::generateDisassembly(Addr pc, const Loader::SymbolTable *symtab) const
{
    std::stringstream ss;
    Addr target;

    // Generate correct mnemonic
    std::string myMnemonic(mnemonic);

    // Additional characters depending on isa bits being set
    if (lkSet) myMnemonic = myMnemonic + "l";
    if (aaSet) myMnemonic = myMnemonic + "a";
    ccprintf(ss, "%-10s ", myMnemonic);

    if (aaSet) {
        target = disp;
    } else {
        target = pc + disp;
    }

    Loader::SymbolTable::const_iterator it;
    if (symtab && (it = symtab->find(target)) != symtab->end())
        ss << it->name;
    else
        ccprintf(ss, "0x%x", target);

    return ss.str();
}


PowerISA::PCState
BranchDispCondOp::branchTarget(const PowerISA::PCState &pc) const
{
    if (aaSet) {
        return disp;
    } else {
        return pc.pc() + disp;
    }
}


std::string
BranchDispCondOp::generateDisassembly(
    Addr pc,
    const Loader::SymbolTable *symtab
    ) const
{
    std::stringstream ss;
    Addr target;

    // Generate the correct mnemonic
    std::string myMnemonic(mnemonic);

    // Additional characters depending on isa bits being set
    if (lkSet) myMnemonic = myMnemonic + "l";
    if (aaSet) myMnemonic = myMnemonic + "a";
    ccprintf(ss, "%-10s ", myMnemonic);

    // Print BI and BO fields
    ss << crBit << ", " << opts << ", ";

    if (aaSet) {
        target = disp;
    } else {
        target = pc + disp;
    }

    Loader::SymbolTable::const_iterator it;
    if (symtab && (it = symtab->find(target)) != symtab->end())
        ss << it->name;
    else
        ccprintf(ss, "0x%x", target);

    return ss.str();
}


PowerISA::PCState
BranchRegCondOp::branchTarget(ThreadContext *tc) const
{
    Addr addr = tc->readIntReg(_srcRegIdx[_numSrcRegs - 1].index());
    return addr & -4ULL;
}


std::string
BranchRegCondOp::generateDisassembly(
    Addr pc,
    const Loader::SymbolTable *symtab
    ) const
{
    std::stringstream ss;

    // Generate the correct mnemonic
    std::string myMnemonic(mnemonic);

    // Additional characters depending on isa bits being set
    if (lkSet) myMnemonic = myMnemonic + "l";
    ccprintf(ss, "%-10s ", mnemonic);

    // Print the BI and BO fields
    ss << crBit << ", " << opts;

    return ss.str();
}
