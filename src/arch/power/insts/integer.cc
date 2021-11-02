/*
 * Copyright (c) 2009 The University of Edinburgh
 * Copyright (c) 2021 IBM Corporation
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


bool inv_flag, lt_flag, gt_flag, eq_flag, ox_flag, sat_flag;              // wwf
bool vxsnan_flag, vximz_flag, vxidi_flag, vxisi_flag, vxzdz_flag, vxsqrt_flag, vxcvi_flag, vxvc_flag;
bool inc_flag, ux_flag, xx_flag, zx_flag;

namespace gem5
{

using namespace PowerISA;
using namespace std;

std::string
IntOp::generateDisassembly(Addr pc, const loader::SymbolTable *symtab) const
{
    std::stringstream ss;
    bool printDest = true;
    bool printSrcs = true;
    bool printSecondSrc = true;

    // Generate the correct mnemonic
    std::string myMnemonic(mnemonic);

    // Special cases
    if (myMnemonic == "mtcrf" ||
        myMnemonic == "mtxer" ||
        myMnemonic == "mtlr"  ||
        myMnemonic == "mtctr" ||
        myMnemonic == "mttar" ||
        myMnemonic == "mttb"  ||
        myMnemonic == "mttbu") {
        printDest = false;
    } else if (myMnemonic == "mfcr"  ||
               myMnemonic == "mfxer" ||
               myMnemonic == "mflr"  ||
               myMnemonic == "mfctr" ||
               myMnemonic == "mftar" ||
               myMnemonic == "mftb"  ||
               myMnemonic == "mftbu") {
        printSrcs = false;
    }

    // Additional characters depending on isa bits being set
    if (oe)
        myMnemonic = myMnemonic + "o";
    if (rc)
        myMnemonic = myMnemonic + ".";
    ccprintf(ss, "%-10s ", myMnemonic);

    // Print the first destination only
    if (_numDestRegs > 0 && printDest)
        printReg(ss, destRegIdx(0));

    // Print the (possibly) two source registers
    if (_numSrcRegs > 0 && printSrcs) {
        if (_numDestRegs > 0 && printDest)
            ss << ", ";
        printReg(ss, srcRegIdx(0));
        if (_numSrcRegs > 1 && printSecondSrc) {
          ss << ", ";
          printReg(ss, srcRegIdx(1));
        }
    }

    return ss.str();
}


std::string
IntImmOp::generateDisassembly(Addr pc, const loader::SymbolTable *symtab) const
{
    std::stringstream ss;

    ccprintf(ss, "%-10s ", mnemonic);

    // Print the first destination only
    if (_numDestRegs > 0)
        printReg(ss, destRegIdx(0));

    // Print the source register
    if (_numSrcRegs > 0) {
        if (_numDestRegs > 0)
            ss << ", ";
        printReg(ss, srcRegIdx(0));
    }

    // Print the immediate value last
    ss << ", " << (int32_t)si;

    return ss.str();
}


std::string
IntArithOp::generateDisassembly(
        Addr pc, const Loader::SymbolTable *symtab) const
{
    std::stringstream ss;
    bool printSecondSrc = true;
    bool printThirdSrc = false;

    // Generate the correct mnemonic
    std::string myMnemonic(mnemonic);

    // Special cases
    if (myMnemonic == "addme" ||
        myMnemonic == "addze" ||
        myMnemonic == "subfme" ||
        myMnemonic == "subfze" ||
        myMnemonic == "neg") {
        printSecondSrc = false;
    } else if (myMnemonic == "maddhd" ||
               myMnemonic == "maddhdu" ||
               myMnemonic == "maddld") {
        printThirdSrc = true;
    }

    // Additional characters depending on isa bits being set
    if (oe)
        myMnemonic = myMnemonic + "o";
    if (rc)
        myMnemonic = myMnemonic + ".";
    ccprintf(ss, "%-10s ", myMnemonic);

    // Print the first destination only
    if (_numDestRegs > 0)
        printReg(ss, destRegIdx(0));

    // Print the first source register
    if (_numSrcRegs > 0) {
        if (_numDestRegs > 0)
            ss << ", ";
        printReg(ss, srcRegIdx(0));

        // Print the second source register
        if (_numSrcRegs > 1 && printSecondSrc) {
            ss << ", ";
            printReg(ss, srcRegIdx(1));

            // Print the third source register
            if (_numSrcRegs > 2 && printThirdSrc) {
                ss << ", ";
                printReg(ss, srcRegIdx(2));
            }
        }
    }

    return ss.str();
}


std::string
IntImmArithOp::generateDisassembly(
        Addr pc, const Loader::SymbolTable *symtab) const
{
    std::stringstream ss;
    bool negateImm = false;

    // Generate the correct mnemonic
    std::string myMnemonic(mnemonic);

    // Special cases
    if (myMnemonic == "addi") {
        if (_numSrcRegs == 0) {
            myMnemonic = "li";
        } else if (si < 0) {
            myMnemonic = "subi";
            negateImm = true;
        }
    } else if (myMnemonic == "addis") {
        if (_numSrcRegs == 0) {
            myMnemonic = "lis";
        } else if (si < 0) {
            myMnemonic = "subis";
            negateImm = true;
        }
    } else if (myMnemonic == "addic" && si < 0) {
        myMnemonic = "subic";
        negateImm = true;
    } else if (myMnemonic == "addic_") {
        if (si < 0) {
            myMnemonic = "subic.";
            negateImm = true;
        } else {
            myMnemonic = "addic.";
        }
    }

    ccprintf(ss, "%-10s ", myMnemonic);

    // Print the first destination only
    if (_numDestRegs > 0)
        printReg(ss, destRegIdx(0));

    // Print the source register
    if (_numSrcRegs > 0) {
        if (_numDestRegs > 0)
            ss << ", ";
        printReg(ss, srcRegIdx(0));
    }

    // Print the immediate value
    if (negateImm)
        ss << ", " << -si;
    else
        ss << ", " << si;

    return ss.str();
}


std::string
IntDispArithOp::generateDisassembly(
        Addr pc, const Loader::SymbolTable *symtab) const
{
    std::stringstream ss;
    bool printSrcs = true;
    bool printDisp = true;
    bool negateDisp = false;

    // Generate the correct mnemonic
    std::string myMnemonic(mnemonic);

    // Special cases
    if (myMnemonic == "addpcis") {
        printSrcs = false;
        if (d == 0) {
            myMnemonic = "lnia";
            printDisp = false;
        } else if (d < 0) {
            myMnemonic = "subpcis";
            negateDisp = true;
        }
    }

    ccprintf(ss, "%-10s ", myMnemonic);

    // Print the first destination only
    if (_numDestRegs > 0)
        printReg(ss, destRegIdx(0));

    // Print the source register
    if (_numSrcRegs > 0 && printSrcs) {
        if (_numDestRegs > 0)
            ss << ", ";
        printReg(ss, srcRegIdx(0));
    }

    // Print the displacement
    if (printDisp)
        ss << ", " << (negateDisp ? -d : d);

    return ss.str();
}


std::string
IntLogicOp::generateDisassembly(
        Addr pc, const Loader::SymbolTable *symtab) const
{
    std::stringstream ss;
    bool printSecondSrc = true;

    // Generate the correct mnemonic
    std::string myMnemonic(mnemonic);

    // Special cases
    if (myMnemonic == "or" && srcRegIdx(0) == srcRegIdx(1)) {
        myMnemonic = "mr";
        printSecondSrc = false;
    } else if (myMnemonic == "extsb" ||
               myMnemonic == "extsh" ||
               myMnemonic == "extsw" ||
               myMnemonic == "cntlzw" ||
               myMnemonic == "cntlzd" ||
               myMnemonic == "cnttzw" ||
               myMnemonic == "cnttzd") {
        printSecondSrc = false;
    }

    // Additional characters depending on isa bits being set
    if (rc)
        myMnemonic = myMnemonic + ".";
    ccprintf(ss, "%-10s ", myMnemonic);

    // Print the first destination only
    if (_numDestRegs > 0)
        printReg(ss, destRegIdx(0));

    // Print the first source register
    if (_numSrcRegs > 0) {
        if (_numDestRegs > 0)
            ss << ", ";
        printReg(ss, srcRegIdx(0));

        // Print the second source register
        if (printSecondSrc) {

            // If the instruction updates the CR, the destination register
            // Ra is read and thus, it becomes the second source register
            // due to its higher precedence over Rb. In this case, it must
            // be skipped.
            if (rc) {
                if (_numSrcRegs > 2) {
                    ss << ", ";
                    printReg(ss, srcRegIdx(2));
                }
            } else {
                if (_numSrcRegs > 1) {
                    ss << ", ";
                    printReg(ss, srcRegIdx(1));
                }
            }
        }
    }

    return ss.str();
}


std::string
IntImmLogicOp::generateDisassembly(
        Addr pc, const Loader::SymbolTable *symtab) const
{
    std::stringstream ss;
    bool printRegs = true;

    // Generate the correct mnemonic
    std::string myMnemonic(mnemonic);

    // Special cases
    if (myMnemonic == "ori" &&
        destRegIdx(0).index() == 0 && srcRegIdx(0).index() == 0) {
        myMnemonic = "nop";
        printRegs = false;
    } else if (myMnemonic == "xori" &&
               destRegIdx(0).index() == 0 && srcRegIdx(0).index() == 0) {
        myMnemonic = "xnop";
        printRegs = false;
    } else if (myMnemonic == "andi_") {
        myMnemonic = "andi.";
    } else if (myMnemonic == "andis_") {
        myMnemonic = "andis.";
    }

    ccprintf(ss, "%-10s ", myMnemonic);

    if (printRegs) {

        // Print the first destination only
        if (_numDestRegs > 0)
            printReg(ss, destRegIdx(0));

        // Print the source register
        if (_numSrcRegs > 0) {
            if (_numDestRegs > 0)
                ss << ", ";
            printReg(ss, srcRegIdx(0));
        }

        // Print the immediate value
        ss << ", " << ui;
    }

     return ss.str();
}


std::string
IntCompOp::generateDisassembly(
        Addr pc, const Loader::SymbolTable *symtab) const
{
    std::stringstream ss;
    bool printFieldPrefix = false;
    bool printLength = true;

    // Generate the correct mnemonic
    std::string myMnemonic(mnemonic);

    // Special cases
    if (myMnemonic == "cmp" ||
        myMnemonic == "cmpl") {
        myMnemonic += l ? "d" : "w";
        printFieldPrefix = true;
        printLength = false;
    }

    ccprintf(ss, "%-10s ", myMnemonic);

    // Print the first destination only
    if (printFieldPrefix) {
        if (bf > 0)
            ss << "cr" << (int) bf;
    } else {
        ss << (int) bf;
    }

    // Print the length
    if (printLength) {
        if (!printFieldPrefix || bf > 0)
            ss << ", ";
        ss << (int) l;
    }

    // Print the first source register
    if (_numSrcRegs > 0) {
        if (!printFieldPrefix || bf > 0 || printLength)
            ss << ", ";
        printReg(ss, srcRegIdx(0));

        // Print the second source register
        if (_numSrcRegs > 1) {
            ss << ", ";
            printReg(ss, srcRegIdx(1));
        }
    }

    return ss.str();
}


std::string
IntImmCompOp::generateDisassembly(
        Addr pc, const Loader::SymbolTable *symtab) const
{
    std::stringstream ss;
    bool printFieldPrefix = false;
    bool printLength = true;

    // Generate the correct mnemonic
    std::string myMnemonic(mnemonic);

    // Special cases
    if (myMnemonic == "cmpi") {
        myMnemonic = l ? "cmpdi" : "cmpwi";
        printFieldPrefix = true;
        printLength = false;
    }

    ccprintf(ss, "%-10s ", myMnemonic);

    // Print the first destination only
    if (printFieldPrefix) {
        if (bf > 0)
            ss << "cr" << (int) bf;
    } else {
        ss << (int) bf;
    }

    // Print the length
    if (printLength) {
        if (!printFieldPrefix || bf > 0)
            ss << ", ";
        ss << (int) l;
    }

    // Print the first source register
    if (_numSrcRegs > 0) {
        if (!printFieldPrefix || bf > 0 || printLength)
            ss << ", ";
        printReg(ss, srcRegIdx(0));
    }

    // Print the immediate value
    ss << ", " << si;

    return ss.str();
}


std::string
IntImmCompLogicOp::generateDisassembly(
        Addr pc, const Loader::SymbolTable *symtab) const
{
    std::stringstream ss;
    bool printFieldPrefix = false;
    bool printLength = true;

    // Generate the correct mnemonic
    std::string myMnemonic(mnemonic);

    // Special cases
    if (myMnemonic == "cmpli") {
        myMnemonic = l ? "cmpldi" : "cmplwi";
        printFieldPrefix = true;
        printLength = false;
    }

    ccprintf(ss, "%-10s ", myMnemonic);

    // Print the first destination only
    if (printFieldPrefix) {
        if (bf > 0)
            ss << "cr" << (int) bf;
    } else {
        ss << (int) bf;
    }

    // Print the length
    if (printLength) {
        if (!printFieldPrefix || bf > 0)
            ss << ", ";
        ss << (int) l;
    }

    // Print the first source register
    if (_numSrcRegs > 0) {
        if (!printFieldPrefix || bf > 0 || printLength)
            ss << ", ";
        printReg(ss, srcRegIdx(0));
    }

    // Print the immediate value
    ss << ", " << ui;

    return ss.str();
}


std::string
IntShiftOp::generateDisassembly(
        Addr pc, const loader::SymbolTable *symtab) const
{
    std::stringstream ss;
    bool printSecondSrc = true;
    bool printShift = false;

    // Generate the correct mnemonic
    std::string myMnemonic(mnemonic);

    // Special cases
    if (myMnemonic == "srawi") {
        printSecondSrc = false;
        printShift = true;
    }

    // Additional characters depending on isa bits being set
    if (rc)
        myMnemonic = myMnemonic + ".";
    ccprintf(ss, "%-10s ", myMnemonic);

    // Print the first destination only
    if (_numDestRegs > 0)
        printReg(ss, destRegIdx(0));

    // Print the first source register
    if (_numSrcRegs > 0) {
        if (_numDestRegs > 0)
            ss << ", ";
        printReg(ss, srcRegIdx(0));

        // Print the second source register
        if (printSecondSrc) {

            // If the instruction updates the CR, the destination register
            // Ra is read and thus, it becomes the second source register
            // due to its higher precedence over Rb. In this case, it must
            // be skipped.
            if (rc) {
                if (_numSrcRegs > 2) {
                    ss << ", ";
                    printReg(ss, srcRegIdx(2));
                }
            } else {
                if (_numSrcRegs > 1) {
                    ss << ", ";
                    printReg(ss, srcRegIdx(1));
                }
            }
        }
    }

    // Print the shift value
    if (printShift)
        ss << ", " << (int) sh;

    return ss.str();
}


std::string
IntConcatShiftOp::generateDisassembly(
        Addr pc, const Loader::SymbolTable *symtab) const
{
    std::stringstream ss;
    bool printSecondSrc = true;
    bool printShift = false;

    // Generate the correct mnemonic
    std::string myMnemonic(mnemonic);

    // Special cases
    if (myMnemonic == "sradi" ||
        myMnemonic == "extswsli") {
        printSecondSrc = false;
        printShift = true;
    }

    // Additional characters depending on isa bits being set
    if (rc)
        myMnemonic = myMnemonic + ".";
    ccprintf(ss, "%-10s ", myMnemonic);

    // Print the first destination only
    if (_numDestRegs > 0)
        printReg(ss, destRegIdx(0));

    // Print the first source register
    if (_numSrcRegs > 0) {
        if (_numDestRegs > 0)
            ss << ", ";
        printReg(ss, srcRegIdx(0));

        // Print the second source register
        if (printSecondSrc) {

            // If the instruction updates the CR, the destination register
            // Ra is read and thus, it becomes the second source register
            // due to its higher precedence over Rb. In this case, it must
            // be skipped.
            if (rc) {
                if (_numSrcRegs > 2) {
                    ss << ", ";
                    printReg(ss, srcRegIdx(2));
                }
            } else {
                if (_numSrcRegs > 1) {
                    ss << ", ";
                    printReg(ss, srcRegIdx(1));
                }
            }
        }
    }

    // Print the shift value
    if (printShift)
        ss << ", " << (int) sh;

    return ss.str();
}


std::string
IntRotateOp::generateDisassembly(
        Addr pc, const loader::SymbolTable *symtab) const
{
    std::stringstream ss;
    bool printSecondSrc = true;
    bool printShift = true;
    bool printMaskBeg = true;
    bool printMaskEnd = true;

    // Generate the correct mnemonic
    std::string myMnemonic(mnemonic);

    // Special cases
    if (myMnemonic == "rlwinm") {
        if (mb == 0 && me == 31) {
            myMnemonic = "rotlwi";
            printMaskBeg = false;
            printMaskEnd = false;
        } else if (sh == 0 && me == 31) {
            myMnemonic = "clrlwi";
            printShift = false;
            printMaskEnd = false;
        }
        printSecondSrc = false;
    } else if (myMnemonic == "rlwnm") {
        if (mb == 0 && me == 31) {
            myMnemonic = "rotlw";
            printMaskBeg = false;
            printMaskEnd = false;
        }
        printShift = false;
    } else if (myMnemonic == "rlwimi") {
        printSecondSrc = false;
    }

    // Additional characters depending on isa bits being set
    if (rc)
        myMnemonic = myMnemonic + ".";
    ccprintf(ss, "%-10s ", myMnemonic);

    // Print the first destination only
    if (_numDestRegs > 0)
        printReg(ss, destRegIdx(0));

    // Print the first source register
    if (_numSrcRegs > 0) {
        if (_numDestRegs > 0)
            ss << ", ";
        printReg(ss, srcRegIdx(0));

        // Print the second source register
        if (printSecondSrc) {

            // If the instruction updates the CR, the destination register
            // Ra is read and thus, it becomes the second source register
            // due to its higher precedence over Rb. In this case, it must
            // be skipped.
            if (rc) {
                if (_numSrcRegs > 2) {
                    ss << ", ";
                    printReg(ss, srcRegIdx(2));
                }
            } else {
                if (_numSrcRegs > 1) {
                    ss << ", ";
                    printReg(ss, srcRegIdx(1));
                }
            }
        }
    }

    // Print the shift value
    if (printShift)
        ss << ", " << (int) sh;

    // Print the mask bounds
    if (printMaskBeg)
        ss << ", " << (int) mb;
    if (printMaskEnd)
        ss << ", " << (int) me;

    return ss.str();
}

std::string
IntConcatRotateOp::generateDisassembly(
        Addr pc, const Loader::SymbolTable *symtab) const
{
    std::stringstream ss;
    bool printSecondSrc = false;
    bool printShift = true;
    bool printMaskBeg = true;

    // Generate the correct mnemonic
    std::string myMnemonic(mnemonic);

    // Special cases
    if (myMnemonic == "rldicl") {
        if (mb == 0) {
            myMnemonic = "rotldi";
            printMaskBeg = false;
        } else if (sh == 0) {
            myMnemonic = "clrldi";
            printShift = false;
        }
    } else if (myMnemonic == "rldcl") {
        if (mb == 0) {
            myMnemonic = "rotld";
            printMaskBeg = false;
        }
        printSecondSrc = true;
        printShift = false;
    } else if (myMnemonic == "rldcr") {
        printSecondSrc = true;
        printShift = false;
    }

    // Additional characters depending on isa bits being set
    if (rc)
        myMnemonic = myMnemonic + ".";
    ccprintf(ss, "%-10s ", myMnemonic);

    // Print the first destination only
    if (_numDestRegs > 0)
        printReg(ss, destRegIdx(0));

    // Print the first source register
    if (_numSrcRegs > 0) {
        if (_numDestRegs > 0)
            ss << ", ";
        printReg(ss, srcRegIdx(0));

        // Print the second source register
        if (printSecondSrc) {

            // If the instruction updates the CR, the destination register
            // Ra is read and thus, it becomes the second source register
            // due to its higher precedence over Rb. In this case, it must
            // be skipped.
            if (rc) {
                if (_numSrcRegs > 2) {
                    ss << ", ";
                    printReg(ss, srcRegIdx(2));
                }
            } else {
                if (_numSrcRegs > 1) {
                    ss << ", ";
                    printReg(ss, srcRegIdx(1));
                }
            }
        }
    }

    // Print the shift amount
    if (printShift)
        ss << ", " << (int) sh;

    // Print the mask bound
    if (printMaskBeg)
        ss << ", " << (int) mb;

    return ss.str();
}


std::string
IntTrapOp::generateDisassembly(
        Addr pc, const Loader::SymbolTable *symtab) const
{
    std::string ext;
    std::stringstream ss;
    bool printSrcs = true;
    bool printCond = false;

    // Generate the correct mnemonic
    std::string myMnemonic(mnemonic);

    // Special cases
    if (myMnemonic == "tw" &&
        (srcRegIdx(0).index() == 0) && (srcRegIdx(1).index() == 0)) {
        myMnemonic = "trap";
        printSrcs = false;
    } else {
        ext = suffix();
        if (!ext.empty() &&
            (myMnemonic == "tw" || myMnemonic == "td")) {
            myMnemonic += ext;
        } else {
            printCond = true;
        }
    }

    ccprintf(ss, "%-10s ", myMnemonic);

    // Print the trap condition
    if (printCond)
        ss << (int) to;

    // Print the source registers
    if (printSrcs) {
        if (_numSrcRegs > 0) {
            if (printCond)
                ss << ", ";
            printReg(ss, srcRegIdx(0));
        }

        if (_numSrcRegs > 1) {
            ss << ", ";
            printReg(ss, srcRegIdx(1));
        }
    }

    return ss.str();
}


std::string
IntImmTrapOp::generateDisassembly(
        Addr pc, const Loader::SymbolTable *symtab) const
{
    std::string ext;
    std::stringstream ss;
    bool printCond = false;

    // Generate the correct mnemonic
    std::string myMnemonic(mnemonic);

    // Special cases
    ext = suffix();
    if (!ext.empty()) {
        if (myMnemonic == "twi") {
            myMnemonic = "tw" + ext + "i";
        } else if (myMnemonic == "tdi") {
            myMnemonic = "td" + ext + "i";
        } else {
            printCond = true;
        }
    } else {
        printCond = true;
    }

    ccprintf(ss, "%-10s ", myMnemonic);

    // Print the trap condition
    if (printCond)
        ss << (int) to;

    // Print the source registers
    if (_numSrcRegs > 0) {
        if (printCond)
            ss << ", ";
        printReg(ss, srcRegIdx(0));
    }

    // Print the immediate value
    ss << ", " << si;

    return ss.str();
}

} // namespace gem5

void bcd_ADD(uint8_t *p_t, uint8_t *p_a, uint8_t *p_b, bool PS)               // wwf
{
  bool sign_a, sign_b;
  sign_a = ((p_a[15] & 0x0f) == 0xB) | ((p_a[15] & 0x0f) == 0xD);
  sign_b = ((p_b[15] & 0x0f) == 0xB) | ((p_b[15] & 0x0f) == 0xD);
  
  // check if a or b are undefined.
  inv_flag = (p_a[15] & 0x0f) < 0xA | (p_b[15] & 0x0f) < 0xA; 
  for(int i=0; i<15; i++)
  {
    inv_flag = inv_flag | ((p_a[i]>>4) > 0x9) | ((p_b[i]>>4) > 0x9);
    inv_flag = inv_flag | ((p_a[i]&0x0F) > 0x9) | ((p_b[i]&0x0F) > 0x9);
  }
  inv_flag = inv_flag | ((p_a[15]>>4) > 0x9) | ((p_b[15]>>4) > 0x9);
  
  if(inv_flag)
  { lt_flag = false; gt_flag = false; eq_flag = false;    return;  }
  
  for(int i=0; i<16; i++)
    p_t[i] = 0x0;
  
  if(sign_a == sign_b)
  {
    uint8_t sum, offset=6, inc=0, t, k;
    for(int i=30; i >= 0; i--)
    {
      t = i % 2;
      k = i / 2;
      if(t == 0)
        sum = (p_a[k] >> 4) + (p_b[k] >> 4) + inc;
      else
        sum = (p_a[k] & 0x0F) + (p_b[k] & 0x0F) + inc;
      if(sum > 9)
      {
        inc = 1;
        sum = (sum + offset) & 0x0F;
      }
      else
        inc = 0;
      if(t == 0)
        p_t[k] = p_t[k] | ((sum << 4) & 0xFF);
      else
        p_t[k] = p_t[k] | (sum & 0x0F);
    }
    if(inc == 1)
      ox_flag = 1;
    else  ox_flag = 0;
    p_t[15] = p_t[15] | (p_a[15] & 0x0f);
  }
  else
  {
    if(!sign_a)   // a >= 0  and b < 0
    {
      uint8_t tmp[18], *p_tmpa;
      for(int i=0; i<16; i++)
        tmp[i] = p_b[i];
      tmp[15] = tmp[15] & 0xF0;
      tmp[15] = tmp[15] | 0x0C;        // -b
      p_tmpa = &tmp[0];
      
      printf("vector      0 - b :                          ");
      for(int i=0; i<16; i++)
        printf("%02x", p_tmpa[i]);
      printf("\n");
      printf("vector      start doing     a - (0 - b)\n");
      bcd_SUBTRACT(p_t, p_a, p_tmpa, PS);
    }
    else          // a < 0  and b >= 0
    {
      uint8_t tmp[18], *p_tmpa;
      for(int i=0; i<16; i++)
        tmp[i] = p_a[i];
      tmp[15] = tmp[15] & 0xF0;
      tmp[15] = tmp[15] | 0x0C;        // -a
      p_tmpa = &tmp[0];
      
      printf("vector      0 - a :                         ");
      for(int i=0; i<16; i++)
        printf("%02x", p_tmpa[i]);
      printf("\n");
      printf("vector      start doing     b - (0 - a)\n");
      bcd_SUBTRACT(p_t, p_b, p_tmpa, PS);
    }
  }
  
  // set control flags.
  bool src_sign = ((p_t[15] & 0x0f) == 0xB) | ((p_t[15] & 0x0f) == 0xD);
  eq_flag = true;
  for(int i=0; i<15; i++)
    eq_flag = eq_flag & (p_t[i] == 0x0);
  eq_flag = eq_flag & ((p_t[15]>>4) == 0x0);
  lt_flag = (!eq_flag) & src_sign;
  gt_flag = (!eq_flag) & (!src_sign);
  
  p_t[15] = p_t[15] & 0xF0;
  if(!lt_flag)
  {
    if(PS == 0)
      p_t[15] = p_t[15] | 0b1100;
    else
      p_t[15] = p_t[15] | 0b1111;
  }
  else
    p_t[15] = p_t[15] | 0b1101;
}

void bcd_SUBTRACT(uint8_t *p_t, uint8_t *p_a, uint8_t *p_b, bool PS)               // wwf  a - b
{
  bool sign_a, sign_b;
  sign_a = ((p_a[15] & 0x0f) == 0xB) | ((p_a[15] & 0x0f) == 0xD);
  sign_b = ((p_b[15] & 0x0f) == 0xB) | ((p_b[15] & 0x0f) == 0xD);
  
  // check if a or b are undefined.
  inv_flag = (p_a[15] & 0x0f) < 0xA | (p_b[15] & 0x0f) < 0xA; 
  for(int i=0; i<15; i++)
  {
    inv_flag = inv_flag | ((p_a[i]>>4) > 0x9) | ((p_b[i]>>4) > 0x9);
    inv_flag = inv_flag | ((p_a[i]&0x0F) > 0x9) | ((p_b[i]&0x0F) > 0x9);
  }
  inv_flag = inv_flag | ((p_a[15]>>4) > 0x9) | ((p_b[15]>>4) > 0x9);
  
  if(inv_flag)
  { lt_flag = false; gt_flag = false; eq_flag = false;    return;  }
  ox_flag = 0;
  
  for(int i=0; i<16; i++)
    p_t[i] = 0x0;
  
  if(sign_a == sign_b)
  {
    int8_t sub, offset=6, inc=0, t, k;
    
    // check if a > b not consider the sign.
    bool check_flag = true;
    uint8_t *p_tmp, tmp_a, tmp_b;    
    for(int i=0; i < 31; i++)
    {
      t = i % 2;
      k = i / 2;
      if(t == 0)
      {
        tmp_a = p_a[k] >> 4;
        tmp_b = p_b[k] >> 4;
      }
      else
      {
        tmp_a = p_a[k] & 0x0F;
        tmp_b = p_b[k] & 0x0F;
      }
      if(tmp_a < tmp_b)               // a < b , not consider the sign
      {
        check_flag = false;
        p_tmp = p_a;   p_a = p_b;    p_b = p_tmp;
        break;
      }
      else if(tmp_a > tmp_b)
        break;
    }
    
    for(int i=30; i >= 0; i--)
    {
      t = i % 2;
      k = i / 2;
      if(t == 0)
      {
        sub = (p_a[k] >> 4) - inc;
        if(sub < 0)
        {
          sub = sub + 16 - (p_b[k] >> 4);
          inc = 1;
        }
        else
        {
          inc = 0;                        // reset inc.
          sub = sub - (p_b[k] >> 4);
          if(sub < 0)
          {
            sub = sub + 16;
            inc = 1;
          }
        }
        
      }
      else
      {
        sub = (p_a[k] & 0x0F) - inc;
        if(sub < 0)
        {
          sub = sub + 16 - (p_b[k] & 0x0F);
          inc = 1;
        }
        else
        {
          inc = 0;                        // reset inc.
          sub = sub - (p_b[k] & 0x0F);
          if(sub < 0)
          {
            sub = sub + 16;
            inc = 1;
          }
        }
        
      }
        
      //printf("vector  1   i=%d k=%d t=%d  a=%x b=%x sub=%x  inc=%d   sign=%d\n", i, k, t, p_a[k], p_b[k], sub, inc, sign_a);
      if(inc == 1)
      {
        sub = (sub - offset) & 0x0F;
      }
      //printf("vector      i=%d k=%d t=%d  a=%x b=%x sub=%x  inc=%d   sign=%d\n", i, k, t, p_a[k], p_b[k], sub, inc, sign_a);
      if(t == 0)
        p_t[k] = p_t[k] | ((sub << 4) & 0xFF);
      else
        p_t[k] = p_t[k] | (sub & 0x0F);
    }
    
    printf("vector    check_flag=%d   inc=%d\n", check_flag, inc);
    if(!check_flag)
    {
      if(sign_a)
        p_t[15] = p_t[15] | 0b1100;
      else
        p_t[15] = p_t[15] | 0b1101;
    }
    else  
      p_t[15] = p_t[15] | (p_a[15] & 0x0f);
  }
  else
  {    
    if(!sign_a)   // a >= 0  and b < 0
    {
      uint8_t tmp[18], *p_tmpa;
      for(int i=0; i<16; i++)
        tmp[i] = p_b[i];
      tmp[15] = tmp[15] & 0xF0;
      tmp[15] = tmp[15] | 0x0C;        // -b
      p_tmpa = &tmp[0];
      
      printf("vector      0 - b :                          ");
      for(int i=0; i<16; i++)
        printf("%02x", p_tmpa[i]);
      printf("\n");
      printf("vector      start doing     a + (0 - b)\n");
      bcd_ADD(p_t, p_a, p_tmpa, PS);
    }
    else          // a < 0  and b >= 0
    {
      uint8_t tmp[18], *p_tmpa;
      for(int i=0; i<16; i++)
        tmp[i] = p_b[i];
      tmp[15] = tmp[15] & 0xF0;
      tmp[15] = tmp[15] | 0x0D;        // -b
      p_tmpa = &tmp[0];
      
      printf("vector      0 - b :                         ");
      for(int i=0; i<16; i++)
        printf("%02x", p_tmpa[i]);
      printf("\n");
      printf("vector      start doing     a + (0 - b)\n");
      bcd_ADD(p_t, p_a, p_tmpa, PS);
    }
  }
  
  // set control flags.
  bool src_sign = ((p_t[15] & 0x0f) == 0xB) | ((p_t[15] & 0x0f) == 0xD);
  eq_flag = true;
  for(int i=0; i<15; i++)
    eq_flag = eq_flag & (p_t[i] == 0x0);
  eq_flag = eq_flag & ((p_t[15]>>4) == 0x0);
  lt_flag = (!eq_flag) & src_sign;
  gt_flag = (!eq_flag) & (!src_sign);
  
  p_t[15] = p_t[15] & 0xF0;
  if(!lt_flag)
  {
    if(PS == 0)
      p_t[15] = p_t[15] | 0b1100;
    else
      p_t[15] = p_t[15] | 0b1111;
  }
  else
    p_t[15] = p_t[15] | 0b1101;
}


void print128(__int128_t x)  
{  
    if(x < 0)
    {
        x = -x;
        putchar('-');
    }
    if(!x)  
    {  
        puts("0");  
        return ;  
    }  
    std::string ret="";  
    while(x)  
    {  
        ret+=x%10+'0';  
        x/=10;  
    }  
    std::reverse(ret.begin(),ret.end());  
    std::cout<<ret<<std::endl;  
}  
void assign_128(__int128 &x, char num[])
{
    x = 0;
    int f = 1, i=0;
    char ch;
    int n = strlen(num);
    if(num[0] == '-') 
    { f = -f;  i = 1;  }
    for(; i < n; i++)
    {
      ch = num[i];
      x = x*10 + ch-'0';
    }
    x *= f;
}

//Let x be a signed integer quadword.
//Let y indicate the preferred sign code.
//Return the signed integer value x in packed decimal format.
void bcd_CONVERT_FROM_SI128(uint8_t *p_t, __int128_t x, bool y)
{
        __int128_t zero = 0x0;
	
	printf("vector:  bcd_CONVERT_FROM_SI128  data = ");
	print128(x);
        uint64_t *pu = (uint64_t *)&x;
        printf("vector:    x=    %llx    %llx\n", *pu, *(pu+1));
	
        uint8_t sign;
        uint8_t *p8 = (uint8_t *)&x;
        
        if( x < zero)
        {
	  x = ~x + 1;
	  printf("vector:  x = %x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x\n", \
		                                 p8[0],p8[1],p8[2],p8[3],p8[4],p8[5],p8[6],p8[7], \
		                                 p8[8],p8[9],p8[10],p8[11],p8[12],p8[13],p8[14],p8[15]);
	  sign = 0x000D;
	}
	else
	  sign = (y==0) ? 0x000C : 0x000F;
	printf("vector:     sign=%d\n", sign);
          
        printf("vector: start     vrb =  ");
        print128(x);
        pu = (uint64_t *)&x;
        printf("vector:        %llx    %llx\n", *pu, *(pu+1));
	
	for(int i=0; i<16; i++)
	  p_t[i] = 0x0;
	int8_t digit, i, j, t;
	for(i=30; i > -1; i--)
	{
	  if(x <= 0)
	    break;
	  t = i % 2;
	  j = i / 2;
	  digit = x % 10;
	  if(t == 0)
	    p_t[j] = p_t[j] | (digit << 4);
	  else
	    p_t[j] = p_t[j] | digit;	
	  printf("vector:  digit= %d   data= ", digit);  
          print128(x);
	  x = x / 10;
	}
	p_t[15] = p_t[15] | (sign & 0x0F);
}

void si128_CONVERT_FROM_BCD(uint8_t *p_t, uint8_t *p8)
{
        __int128_t result = 0x0;
	
	//printf("vector:    si128_CONVERT_FROM_BCD  data =  %x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x\n", \
		                                 p8[0],p8[1],p8[2],p8[3],p8[4],p8[5],p8[6],p8[7], \
		                                 p8[8],p8[9],p8[10],p8[11],p8[12],p8[13],p8[14],p8[15]);
	
        __int128_t scale = 1, digit;
        int sign = p8[15] & 0x0F;
        int t, j;
        for(int i=30; i>-1; i--)
        {
	  t = i % 2;
	  j = i / 2;
	  if(t == 0)
	    digit = (p8[j] >> 4) & 0x0F;
	  else
	    digit = p8[j] & 0x0F;
	  result = result + digit * scale;
	  scale = scale * 10;
	}
	
	//printf("vector: result =  ");
        //print128(result);
        
	if (sign == 0x0B | sign == 0x0D)
	 result = ~result + 1;
	
	//printf("vector: result =  ");
        //print128(result);
	
	uint8_t *ppp = (uint8_t*)&result;
	for(int i = 0; i < 16; i++)
	  p_t[15-i] = ppp[i];
	
	//printf("vector:    return data =  %x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x\n", \
		                                 p_t[0],p_t[1],p_t[2],p_t[3],p_t[4],p_t[5],p_t[6],p_t[7], \
		                                 p_t[8],p_t[9],p_t[10],p_t[11],p_t[12],p_t[13],p_t[14],p_t[15]);
}
