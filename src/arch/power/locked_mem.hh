/*
 * Copyright (c) 2006 The Regents of The University of Michigan
 * Copyright (c) 2007-2008 The Florida State University
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

#ifndef __ARCH_POWER_LOCKED_MEM_HH__
#define __ARCH_POWER_LOCKED_MEM_HH__

/**
 * @file
 *
 * ISA-specific helper functions for locked memory accesses.
 */

#include <cstdlib>

#include "arch/generic/locked_mem.hh"
#include "arch/power/miscregs.hh"
#include "arch/power/registers.hh"
#include "cpu/simple_thread.hh"
#include "cpu/thread_context.hh"
#include "debug/LLSC.hh"
#include "mem/packet.hh"
#include "mem/request.hh"

namespace PowerISA
{

template <class XC>
inline void
handleLockedSnoop(XC *xc, PacketPtr pkt, Addr cacheBlockMask)
{
    // If we see a snoop come into the CPU and we currently have an LLSC
    // operation pending we need to clear the lock flag if it is to the same
    // addr.
        ThreadContext *tc = ((SimpleThread *)xc)->getTC();
        if (!tc->readIntReg(INTREG_RSV))
        return;

    Addr locked_addr = tc->readIntReg(INTREG_RSV_ADDR);
    Addr snoop_addr = pkt->getAddr();

    if (locked_addr == snoop_addr)
        tc->setIntReg(INTREG_RSV, 0);
}

template <class XC>
inline void
handleLockedRead(XC *xc, const RequestPtr &req)
{
    ThreadContext *tc = ((SimpleThread *)xc)->getTC();
    tc->setIntReg(INTREG_RSV, 1);
    tc->setIntReg(INTREG_RSV_LEN, req->getSize());
    tc->setIntReg(INTREG_RSV_ADDR, req->getPaddr());
    DPRINTF(LLSC,"%s: Placing addr %#x in monitor\n", xc->getCpuPtr()->name(),
                 req->getPaddr());
}

template <class XC>
inline void
handleLockedSnoopHit(XC *xc)
{
}


template <class XC>
inline bool
handleLockedWrite(XC *xc, const RequestPtr &req, Addr cacheBlockMask)
{
    DPRINTF(LLSC,"%s: handling locked write for  address %#x in monitor\n",
            xc->getCpuPtr()->name(), req->getPaddr());

    ThreadContext *tc = ((SimpleThread *)xc)->getTC();
    int lock_flag = tc->readIntReg(INTREG_RSV);
    Addr lock_addr = tc->readIntReg(INTREG_RSV_ADDR);
    unsigned size = tc->readIntReg(INTREG_RSV_LEN);
    bool store_performed = false;
    bool undefined_case;

    if (lock_flag) {
        if (req->getSize() == size && req->getPaddr() == lock_addr)
            {
          undefined_case = false;
          store_performed = true;
            }
            else {
          //Taking smallest real page size supported as 64k
              int z = 64*1024;
              if (req->getPaddr()/z == lock_addr/z)
                    undefined_case = true;
              else {
                undefined_case = false;
                store_performed = false;
              }
            }
    }
    else {
      undefined_case = false;
      store_performed = false;
    }
    Xer xer = tc->readIntReg(INTREG_XER);
    Cr cr = tc->readIntReg(INTREG_CR);
    tc->setIntReg(INTREG_RSV, 0);

    if (undefined_case) {
            bool randombool = rand() % 1;
            if (randombool){
              xc->setStCondFailures(0);
            }
            bool secondrandombool = rand() % 1;
            cr.cr0 = ((secondrandombool ? 0x2 : 0x0) | xer.so);
            tc->setIntReg(INTREG_CR, cr);
    return randombool;
    }

    if (store_performed) {
        xc->setStCondFailures(0);
    }
      else {
      // Lock flag not set or addr mismatch in CPU;
      // the rest of this code is not architectural;
      // it's just a debugging aid to help detect
      // livelock by warning on long sequences of failed
      // store conditionals
      int stCondFailures = xc->readStCondFailures();
      stCondFailures++;
      xc->setStCondFailures(stCondFailures);
      if (stCondFailures % 100000 == 0) {
      warn("%i: context %d: %d consecutive "
             "store conditional failures\n",
             curTick(), xc->contextId(), stCondFailures);
      }

      if (!lock_flag){
        DPRINTF(LLSC, "[cid:%i]: Lock Flag Set, "
                "Store Conditional Failed.\n",
                 req->contextId());
      }
      else if (req->getPaddr() != lock_addr) {
             DPRINTF(LLSC, "[cid:%i]: Load-Link Address Mismatch, "
                              "Store Conditional Failed.\n",
                        req->contextId());
      }
      }
      cr.cr0 = ((store_performed ? 0x2 : 0x0) | xer.so);
      tc->setIntReg(INTREG_CR, cr);
    // store conditional failed already, so don't issue it to mem
    return store_performed;
}

template <class XC>
inline void
globalClearExclusive(XC *xc)
{
}

} // namespace PowerISA

namespace PowerISA
{

using namespace GenericISA::LockedMem;

} // namespace PowerISA

#endif // __ARCH_POWER_LOCKED_MEM_HH__
