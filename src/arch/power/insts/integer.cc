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

#include "arch/power/insts/integer.hh"

using namespace std;
using namespace PowerISA;

string
IntOp::generateDisassembly(
        Addr pc, const Loader::SymbolTable *symtab) const
{
    stringstream ss;
    bool printDest = true;
    bool printSrcs = true;
    bool printSecondSrc = true;

    // Generate the correct mnemonic
    string myMnemonic(mnemonic);

    // Special cases
    if (!myMnemonic.compare("or") && _srcRegIdx[0] == _srcRegIdx[1]) {
        myMnemonic = "mr";
        printSecondSrc = false;
    } else if (!myMnemonic.compare("mtlr")) {
        printDest = false;
    } else if (!myMnemonic.compare("mflr")) {
        printSrcs = false;
    }

    // Additional characters depending on isa bits being set
    if (oeSet) myMnemonic = myMnemonic + "o";
    if (rcSet) myMnemonic = myMnemonic + ".";
    ccprintf(ss, "%-10s ", myMnemonic);

    // Print the first destination only
    if (_numDestRegs > 0 && printDest) {
        printReg(ss, _destRegIdx[0]);
    }

    // Print the (possibly) two source registers
    if (_numSrcRegs > 0 && printSrcs) {
        if (_numDestRegs > 0 && printDest) {
            ss << ", ";
        }
        printReg(ss, _srcRegIdx[0]);
        if (_numSrcRegs > 1 && printSecondSrc) {
          ss << ", ";
          printReg(ss, _srcRegIdx[1]);
        }
    }

    return ss.str();
}


string
IntImmOp::generateDisassembly(
        Addr pc, const Loader::SymbolTable *symtab) const
{
    stringstream ss;

    ccprintf(ss, "%-10s ", mnemonic);

    // Print the first destination only
    if (_numDestRegs > 0) {
        printReg(ss, _destRegIdx[0]);
    }

    // Print the source register
    if (_numSrcRegs > 0) {
        if (_numDestRegs > 0) {
            ss << ", ";
        }
        printReg(ss, _srcRegIdx[0]);
    }

    // Print the immediate value last
    ss << ", " << (int32_t)imm;

    return ss.str();
}


string
IntArithOp::generateDisassembly(
        Addr pc,const Loader::SymbolTable *symtab) const
{
    stringstream ss;
    bool printSecondSrc = true;
    bool printThirdSrc = false;

    // Generate the correct mnemonic
    string myMnemonic(mnemonic);

    // Special cases
    if (!myMnemonic.compare("addme") ||
        !myMnemonic.compare("addze") ||
        !myMnemonic.compare("subfme") ||
        !myMnemonic.compare("subfze") ||
        !myMnemonic.compare("neg")){
        printSecondSrc = false;
    } else if (!myMnemonic.compare("maddhd") ||
               !myMnemonic.compare("maddhdu") ||
               !myMnemonic.compare("maddld")) {
        printThirdSrc = true;
    }

    // Additional characters depending on isa bits being set
    if (oeSet) myMnemonic = myMnemonic + "o";
    if (rcSet) myMnemonic = myMnemonic + ".";
    ccprintf(ss, "%-10s ", myMnemonic);

    // Print the first destination only
    if (_numDestRegs > 0) {
        printReg(ss, _destRegIdx[0]);
    }

    // Print the first source register
    if (_numSrcRegs > 0) {
        if (_numDestRegs > 0) {
            ss << ", ";
        }
        printReg(ss, _srcRegIdx[0]);

        // Print the second source register
        if (_numSrcRegs > 1 && printSecondSrc) {
            ss << ", ";
            printReg(ss, _srcRegIdx[1]);

            // Print the third source register
            if (_numSrcRegs > 2 && printThirdSrc) {
                ss << ", ";
                printReg(ss, _srcRegIdx[2]);
            }
        }
    }

    return ss.str();
}


string
IntImmArithOp::generateDisassembly(
        Addr pc, const Loader::SymbolTable *symtab) const
{
    stringstream ss;
    bool negateSimm = false;

    // Generate the correct mnemonic
    string myMnemonic(mnemonic);

    // Special cases
    if (!myMnemonic.compare("addi")) {
        if (_numSrcRegs == 0) {
            myMnemonic = "li";
        } else if (simm < 0) {
            myMnemonic = "subi";
            negateSimm = true;
        }
    } else if (!myMnemonic.compare("addis")) {
        if (_numSrcRegs == 0) {
            myMnemonic = "lis";
        } else if (simm < 0) {
            myMnemonic = "subis";
            negateSimm = true;
        }
    } else if (!myMnemonic.compare("addic") && simm < 0) {
        myMnemonic = "subic";
        negateSimm = true;
    } else if (!myMnemonic.compare("addic_")) {
        if (simm < 0) {
            myMnemonic = "subic.";
            negateSimm = true;
        } else {
            myMnemonic = "addic.";
        }
    }

    ccprintf(ss, "%-10s ", myMnemonic);

    // Print the first destination only
    if (_numDestRegs > 0) {
        printReg(ss, _destRegIdx[0]);
    }

    // Print the source register
    if (_numSrcRegs > 0) {
        if (_numDestRegs > 0) {
            ss << ", ";
        }
        printReg(ss, _srcRegIdx[0]);
    }

    // Print the immediate value
    if (negateSimm) {
        ss << ", " << -simm;
    } else {
        ss << ", " << simm;
    }

    return ss.str();
}


string
IntDispArithOp::generateDisassembly(
        Addr pc, const Loader::SymbolTable *symtab) const
{
    stringstream ss;
    bool printSrcs = true;
    bool printDisp = true;
    bool negateDisp = false;

    // Generate the correct mnemonic
    string myMnemonic(mnemonic);

    // Special cases
    if (!myMnemonic.compare("addpcis")) {
        printSrcs = false;
        if (disp == 0) {
            myMnemonic = "lnia";
            printDisp = false;
        } else if (disp < 0) {
            myMnemonic = "subpcis";
            negateDisp = true;
        }
    }

    ccprintf(ss, "%-10s ", myMnemonic);

    // Print the first destination only
    if (_numDestRegs > 0) {
        printReg(ss, _destRegIdx[0]);
    }

    // Print the source register
    if (_numSrcRegs > 0 && printSrcs) {
        if (_numDestRegs > 0) {
            ss << ", ";
        }
        printReg(ss, _srcRegIdx[0]);
    }

    // Print the displacement
    if (printDisp) {
        if (negateDisp) {
            ss << ", " << -disp;
        } else {
            ss << ", " << disp;
        }
    }

    return ss.str();
}


string
IntLogicOp::generateDisassembly(
        Addr pc, const Loader::SymbolTable *symtab) const
{
    stringstream ss;
    bool printSecondSrc = true;

    // Generate the correct mnemonic
    string myMnemonic(mnemonic);

    // Special cases
    if (!myMnemonic.compare("or") && _srcRegIdx[0] == _srcRegIdx[1]) {
        myMnemonic = "mr";
        printSecondSrc = false;
    } else if (!myMnemonic.compare("extsb") ||
               !myMnemonic.compare("extsh") ||
               !myMnemonic.compare("extsw") ||
               !myMnemonic.compare("cntlzw") ||
               !myMnemonic.compare("cntlzd") ||
               !myMnemonic.compare("cnttzw") ||
               !myMnemonic.compare("cnttzd")) {
        printSecondSrc = false;
    }

    // Additional characters depending on isa bits being set
    if (rcSet) myMnemonic = myMnemonic + ".";
    ccprintf(ss, "%-10s ", myMnemonic);

    // Print the first destination only
    if (_numDestRegs > 0) {
        printReg(ss, destRegIdx(0));
    }

    // Print the first source register
    if (_numSrcRegs > 0) {
        if (_numDestRegs > 0) {
            ss << ", ";
        }
        printReg(ss, _srcRegIdx[0]);

        // Print the second source register
        if (printSecondSrc) {

            // If the instruction updates the CR, the destination register
            // Ra is read and thus, it becomes the second source register
            // due to its higher precedence over Rb. In this case, it must
            // be skipped.
            if (rcSet) {
                if (_numSrcRegs > 2) {
                    ss << ", ";
                    printReg(ss, _srcRegIdx[2]);
                }
            } else {
                if (_numSrcRegs > 1) {
                    ss << ", ";
                    printReg(ss, _srcRegIdx[1]);
                }
            }
        }
    }

    return ss.str();
}


string
IntImmLogicOp::generateDisassembly(
        Addr pc, const Loader::SymbolTable *symtab) const
{
    stringstream ss;
    bool printRegs = true;

    // Generate the correct mnemonic
    string myMnemonic(mnemonic);

    // Special cases
    if (!myMnemonic.compare("ori") &&
        _destRegIdx[0].index() == 0 && _srcRegIdx[0].index() == 0) {
        myMnemonic = "nop";
        printRegs = false;
    } else if (!myMnemonic.compare("xori") &&
               _destRegIdx[0].index() == 0 && _srcRegIdx[0].index() == 0) {
        myMnemonic = "xnop";
        printRegs = false;
    } else if (!myMnemonic.compare("andi_")) {
        myMnemonic = "andi.";
    } else if (!myMnemonic.compare("andis_")) {
        myMnemonic = "andis.";
    }

    ccprintf(ss, "%-10s ", myMnemonic);

    if (printRegs) {

        // Print the first destination only
        if (_numDestRegs > 0) {
            printReg(ss, _destRegIdx[0]);
        }

        // Print the source register
        if (_numSrcRegs > 0) {
            if (_numDestRegs > 0) {
                ss << ", ";
            }
            printReg(ss, _srcRegIdx[0]);
        }

        // Print the immediate value
        ss << ", " << uimm;
    }

     return ss.str();
}


string
IntCompOp::generateDisassembly(
        Addr pc, const Loader::SymbolTable *symtab) const
{
    stringstream ss;
    bool printFieldPrefix = false;
    bool printLength = true;

    // Generate the correct mnemonic
    string myMnemonic(mnemonic);

    // Special cases
    if (!myMnemonic.compare("cmp")) {
        if (length) {
            myMnemonic = "cmpd";
        } else {
            myMnemonic = "cmpw";
        }
        printFieldPrefix = true;
        printLength = false;
    } else if (!myMnemonic.compare("cmpl")) {
        if (length) {
            myMnemonic = "cmpld";
        } else {
            myMnemonic = "cmplw";
        }
        printFieldPrefix = true;
        printLength = false;
    }

    ccprintf(ss, "%-10s ", myMnemonic);

    // Print the first destination only
    if (printFieldPrefix) {
        if (field > 0) {
            ss << "cr" << field;
        }
    } else {
        ss << field;
    }

    // Print the length
    if (printLength) {
        if (!printFieldPrefix || field > 0) {
            ss << ", ";
        }
        ss << length;
    }

    // Print the first source register
    if (_numSrcRegs > 0) {
        if (!printFieldPrefix || field > 0 || printLength) {
            ss << ", ";
        }
        printReg(ss, _srcRegIdx[0]);

        // Print the second source register
        if (_numSrcRegs > 1) {
            ss << ", ";
            printReg(ss, _srcRegIdx[1]);
        }
    }

    return ss.str();
}


string
IntImmCompOp::generateDisassembly(
        Addr pc, const Loader::SymbolTable *symtab) const
{
    stringstream ss;
    bool printFieldPrefix = false;
    bool printLength = true;

    // Generate the correct mnemonic
    string myMnemonic(mnemonic);

    // Special cases
    if (!myMnemonic.compare("cmpi")) {
        if (length) {
            myMnemonic = "cmpdi";
        } else {
            myMnemonic = "cmpwi";
        }
        printFieldPrefix = true;
        printLength = false;
    }

    ccprintf(ss, "%-10s ", myMnemonic);

    // Print the first destination only
    if (printFieldPrefix) {
        if (field > 0) {
            ss << "cr" << field;
        }
    } else {
        ss << field;
    }

    // Print the length
    if (printLength) {
        if (!printFieldPrefix || field > 0) {
            ss << ", ";
        }
        ss << length;
    }

    // Print the first source register
    if (_numSrcRegs > 0) {
        if (!printFieldPrefix || field > 0 || printLength) {
            ss << ", ";
        }
        printReg(ss, _srcRegIdx[0]);
    }

    // Print the immediate value
    ss << ", " << simm;

    return ss.str();
}


string
IntImmCompLogicOp::generateDisassembly(Addr pc,
        const Loader::SymbolTable *symtab) const
{
    stringstream ss;
    bool printFieldPrefix = false;
    bool printLength = true;

    // Generate the correct mnemonic
    string myMnemonic(mnemonic);

    // Special cases
    if (!myMnemonic.compare("cmpli")) {
        if (length) {
            myMnemonic = "cmpldi";
        } else {
            myMnemonic = "cmplwi";
        }
        printFieldPrefix = true;
        printLength = false;
    }

    ccprintf(ss, "%-10s ", myMnemonic);

    // Print the first destination only
    if (printFieldPrefix) {
        if (field > 0) {
            ss << "cr" << field;
        }
    } else {
        ss << field;
    }

    // Print the mode
    if (printLength) {
        if (!printFieldPrefix || field > 0) {
            ss << ", ";
        }
        ss << length;
    }

    // Print the first source register
    if (_numSrcRegs > 0) {
        if (!printFieldPrefix || field > 0 || printLength) {
            ss << ", ";
        }
        printReg(ss, _srcRegIdx[0]);
    }

    // Print the immediate value
    ss << ", " << uimm;

    return ss.str();
}


string
IntShiftOp::generateDisassembly(
        Addr pc, const Loader::SymbolTable *symtab) const
{
    stringstream ss;
    bool printSecondSrc = true;
    bool printShift = false;

    // Generate the correct mnemonic
    string myMnemonic(mnemonic);

    // Special cases
    if (!myMnemonic.compare("srawi")) {
        printSecondSrc = false;
        printShift = true;
    }

    // Additional characters depending on isa bits being set
    if (rcSet) myMnemonic = myMnemonic + ".";
    ccprintf(ss, "%-10s ", myMnemonic);

    // Print the first destination only
    if (_numDestRegs > 0) {
        printReg(ss, _destRegIdx[0]);
    }

    // Print the first source register
    if (_numSrcRegs > 0) {
        if (_numDestRegs > 0) {
            ss << ", ";
        }
        printReg(ss, _srcRegIdx[0]);

        // Print the second source register
        if (printSecondSrc) {

            // If the instruction updates the CR, the destination register
            // Ra is read and thus, it becomes the second source register
            // due to its higher precedence over Rb. In this case, it must
            // be skipped.
            if (rcSet) {
                if (_numSrcRegs > 2) {
                    ss << ", ";
                    printReg(ss, _srcRegIdx[2]);
                }
            } else {
                if (_numSrcRegs > 1) {
                    ss << ", ";
                    printReg(ss, _srcRegIdx[1]);
                }
            }
        }
    }

    // Print the shift value
    if (printShift) {
        ss << ", " << shift;
    }

    return ss.str();
}


string
IntConcatShiftOp::generateDisassembly(
        Addr pc,const Loader::SymbolTable *symtab) const
{
    stringstream ss;
    bool printSecondSrc = true;
    bool printShift = false;

    // Generate the correct mnemonic
    string myMnemonic(mnemonic);

    // Special cases
    if (!myMnemonic.compare("sradi") ||
        !myMnemonic.compare("extswsli")) {
        printSecondSrc = false;
        printShift = true;
    }

    // Additional characters depending on isa bits being set
    if (rcSet) myMnemonic = myMnemonic + ".";
    ccprintf(ss, "%-10s ", myMnemonic);


    // Print the first destination only
    if (_numDestRegs > 0) {
        printReg(ss, _destRegIdx[0]);
    }

    // Print the first source register
    if (_numSrcRegs > 0) {
        if (_numDestRegs > 0) {
            ss << ", ";
        }
        printReg(ss, _srcRegIdx[0]);

        // Print the second source register
        if (printSecondSrc) {

            // If the instruction updates the CR, the destination register
            // Ra is read and thus, it becomes the second source register
            // due to its higher precedence over Rb. In this case, it must
            // be skipped.
            if (rcSet) {
                if (_numSrcRegs > 2) {
                    ss << ", ";
                    printReg(ss, _srcRegIdx[2]);
                }
            } else {
                if (_numSrcRegs > 1) {
                    ss << ", ";
                    printReg(ss, _srcRegIdx[1]);
                }
            }
        }
    }

    // Print the shift value
    if (printShift) {
        ss << ", " << shift;
    }

    return ss.str();
}


string
IntRotateOp::generateDisassembly(
        Addr pc, const Loader::SymbolTable *symtab) const
{
    stringstream ss;
    bool printSecondSrc = true;
    bool printShift = true;
    bool printMaskBeg = true;
    bool printMaskEnd = true;

    // Generate the correct mnemonic
    string myMnemonic(mnemonic);

    // Special cases
    if (!myMnemonic.compare("rlwinm")) {
        if (maskBeg == 0 && maskEnd == 31) {
            myMnemonic = "rotlwi";
            printMaskBeg = false;
            printMaskEnd = false;
        } else if (shift == 0 && maskEnd == 31) {
            myMnemonic = "clrlwi";
            printShift = false;
            printMaskEnd = false;
        }
        printSecondSrc = false;
    } else if (!myMnemonic.compare("rlwnm")) {
        if (maskBeg == 0 && maskEnd == 31) {
            myMnemonic = "rotlw";
            printMaskBeg = false;
            printMaskEnd = false;
        }
        printShift = false;
    } else if (!myMnemonic.compare("rlwimi")) {
        printSecondSrc = false;
    }

    // Additional characters depending on isa bits being set
    if (rcSet) myMnemonic = myMnemonic + ".";
    ccprintf(ss, "%-10s ", myMnemonic);

    // Print the first destination only
    if (_numDestRegs > 0) {
        printReg(ss, _destRegIdx[0]);
    }

    // Print the first source register
    if (_numSrcRegs > 0) {
        if (_numDestRegs > 0) {
            ss << ", ";
        }
        printReg(ss, _srcRegIdx[0]);

        // Print the second source register
        if (printSecondSrc) {

            // If the instruction updates the CR, the destination register
            // Ra is read and thus, it becomes the second source register
            // due to its higher precedence over Rb. In this case, it must
            // be skipped.
            if (rcSet) {
                if (_numSrcRegs > 2) {
                    ss << ", ";
                    printReg(ss, _srcRegIdx[2]);
                }
            } else {
                if (_numSrcRegs > 1) {
                    ss << ", ";
                    printReg(ss, _srcRegIdx[1]);
                }
            }
        }
    }

    // Print the shift value
    if (printShift) {
        ss << ", " << shift;
    }

    // Print the mask bounds
    if (printMaskBeg) {
        ss << ", " << maskBeg;
    }
    if (printMaskEnd) {
        ss << ", " << maskEnd;
    }

    return ss.str();
}

string
IntConcatRotateOp::generateDisassembly(Addr pc,
                                       const Loader::SymbolTable *symtab) const
{
    stringstream ss;
    bool printSecondSrc = false;
    bool printShift = true;
    bool printMaskBeg = true;

    // Generate the correct mnemonic
    string myMnemonic(mnemonic);

    // Special cases
    if (!myMnemonic.compare("rldicl")) {
        if (maskBeg == 0) {
            myMnemonic = "rotldi";
            printMaskBeg = false;
        } else if (shift == 0) {
            myMnemonic = "clrldi";
            printShift = false;
        }
    } else if (!myMnemonic.compare("rldcl")) {
        if (maskBeg == 0) {
            myMnemonic = "rotld";
            printMaskBeg = false;
        }
        printSecondSrc = true;
        printShift = false;
    } else if (!myMnemonic.compare("rldcr")) {
        printSecondSrc = true;
        printShift = false;
    }

    // Additional characters depending on isa bits being set
    if (rcSet) myMnemonic = myMnemonic + ".";
    ccprintf(ss, "%-10s ", myMnemonic);

    // Print the first destination only
    if (_numDestRegs > 0) {
        printReg(ss, _destRegIdx[0]);
    }

    // Print the first source register
    if (_numSrcRegs > 0) {
        if (_numDestRegs > 0) {
            ss << ", ";
        }
        printReg(ss, _srcRegIdx[0]);

        // Print the second source register
        if (printSecondSrc) {

            // If the instruction updates the CR, the destination register
            // Ra is read and thus, it becomes the second source register
            // due to its higher precedence over Rb. In this case, it must
            // be skipped.
            if (rcSet) {
                if (_numSrcRegs > 2) {
                    ss << ", ";
                    printReg(ss, _srcRegIdx[2]);
                }
            } else {
                if (_numSrcRegs > 1) {
                    ss << ", ";
                    printReg(ss, _srcRegIdx[1]);
                }
            }
        }
    }

    // Print the shift amount
    if (printShift) {
        ss << ", " << shift;
    }

    // Print the mask bound
    if (printMaskBeg) {
        ss << ", " << maskBeg;
    }

    return ss.str();
}
