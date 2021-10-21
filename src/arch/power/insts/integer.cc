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

bool inv_flag, lt_flag, gt_flag, eq_flag, ox_flag, sat_flag;              // wwf
bool vxsnan_flag, vximz_flag, vxidi_flag, vxisi_flag, vxzdz_flag, vxsqrt_flag, vxcvi_flag, vxvc_flag;
bool inc_flag, ux_flag, xx_flag, zx_flag;


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
    if (!myMnemonic.compare("or") && srcRegIdx(0) == srcRegIdx(1)) {
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
        printReg(ss, destRegIdx(0));
    }

    // Print the (possibly) two source registers
    if (_numSrcRegs > 0 && printSrcs) {
        if (_numDestRegs > 0 && printDest) {
            ss << ", ";
        }
        printReg(ss, srcRegIdx(0));
        if (_numSrcRegs > 1 && printSecondSrc) {
          ss << ", ";
          printReg(ss, srcRegIdx(1));
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
        printReg(ss, destRegIdx(0));
    }

    // Print the source register
    if (_numSrcRegs > 0) {
        if (_numDestRegs > 0) {
            ss << ", ";
        }
        printReg(ss, srcRegIdx(0));
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
        printReg(ss, destRegIdx(0));
    }

    // Print the first source register
    if (_numSrcRegs > 0) {
        if (_numDestRegs > 0) {
            ss << ", ";
        }
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
        printReg(ss, destRegIdx(0));
    }

    // Print the source register
    if (_numSrcRegs > 0) {
        if (_numDestRegs > 0) {
            ss << ", ";
        }
        printReg(ss, srcRegIdx(0));
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
        printReg(ss, destRegIdx(0));
    }

    // Print the source register
    if (_numSrcRegs > 0 && printSrcs) {
        if (_numDestRegs > 0) {
            ss << ", ";
        }
        printReg(ss, srcRegIdx(0));
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
    if (!myMnemonic.compare("or") && srcRegIdx(0) == srcRegIdx(1)) {
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
        printReg(ss, srcRegIdx(0));

        // Print the second source register
        if (printSecondSrc) {

            // If the instruction updates the CR, the destination register
            // Ra is read and thus, it becomes the second source register
            // due to its higher precedence over Rb. In this case, it must
            // be skipped.
            if (rcSet) {
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
        destRegIdx(0).index() == 0 && srcRegIdx(0).index() == 0) {
        myMnemonic = "nop";
        printRegs = false;
    } else if (!myMnemonic.compare("xori") &&
               destRegIdx(0).index() == 0 && srcRegIdx(0).index() == 0) {
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
            printReg(ss, destRegIdx(0));
        }

        // Print the source register
        if (_numSrcRegs > 0) {
            if (_numDestRegs > 0) {
                ss << ", ";
            }
            printReg(ss, srcRegIdx(0));
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
        printReg(ss, srcRegIdx(0));

        // Print the second source register
        if (_numSrcRegs > 1) {
            ss << ", ";
            printReg(ss, srcRegIdx(1));
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
        printReg(ss, srcRegIdx(0));
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
        printReg(ss, srcRegIdx(0));
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
        printReg(ss, destRegIdx(0));
    }

    // Print the first source register
    if (_numSrcRegs > 0) {
        if (_numDestRegs > 0) {
            ss << ", ";
        }
        printReg(ss, srcRegIdx(0));

        // Print the second source register
        if (printSecondSrc) {

            // If the instruction updates the CR, the destination register
            // Ra is read and thus, it becomes the second source register
            // due to its higher precedence over Rb. In this case, it must
            // be skipped.
            if (rcSet) {
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
        printReg(ss, destRegIdx(0));
    }

    // Print the first source register
    if (_numSrcRegs > 0) {
        if (_numDestRegs > 0) {
            ss << ", ";
        }
        printReg(ss, srcRegIdx(0));

        // Print the second source register
        if (printSecondSrc) {

            // If the instruction updates the CR, the destination register
            // Ra is read and thus, it becomes the second source register
            // due to its higher precedence over Rb. In this case, it must
            // be skipped.
            if (rcSet) {
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
        printReg(ss, destRegIdx(0));
    }

    // Print the first source register
    if (_numSrcRegs > 0) {
        if (_numDestRegs > 0) {
            ss << ", ";
        }
        printReg(ss, srcRegIdx(0));

        // Print the second source register
        if (printSecondSrc) {

            // If the instruction updates the CR, the destination register
            // Ra is read and thus, it becomes the second source register
            // due to its higher precedence over Rb. In this case, it must
            // be skipped.
            if (rcSet) {
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
        printReg(ss, destRegIdx(0));
    }

    // Print the first source register
    if (_numSrcRegs > 0) {
        if (_numDestRegs > 0) {
            ss << ", ";
        }
        printReg(ss, srcRegIdx(0));

        // Print the second source register
        if (printSecondSrc) {

            // If the instruction updates the CR, the destination register
            // Ra is read and thus, it becomes the second source register
            // due to its higher precedence over Rb. In this case, it must
            // be skipped.
            if (rcSet) {
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
    if (printShift) {
        ss << ", " << shift;
    }

    // Print the mask bound
    if (printMaskBeg) {
        ss << ", " << maskBeg;
    }

    return ss.str();
}

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
    string ret="";  
    while(x)  
    {  
        ret+=x%10+'0';  
        x/=10;  
    }  
    reverse(ret.begin(),ret.end());  
    cout<<ret<<endl;  
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
