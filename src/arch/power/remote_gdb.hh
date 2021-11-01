/*
 * Copyright (c) 2015 LabWare
 * Copyright (c) 2002-2005 The Regents of The University of Michigan
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

#ifndef __ARCH_POWER_REMOTE_GDB_HH__
#define __ARCH_POWER_REMOTE_GDB_HH__

#include "arch/power/registers.hh"
#include "arch/power/remote_gdb.hh"
#include "base/remote_gdb.hh"

namespace PowerISA
{


class RemoteGDB : public BaseRemoteGDB
{
  protected:
    bool acc(Addr addr, size_t len);

    class PowerGdbRegCache : public BaseGdbRegCache
    {
      using BaseGdbRegCache::BaseGdbRegCache;
      private:
        struct {
            uint64_t gpr[NumIntArchRegs];

            uint64_t pc;
            uint64_t msr;
            uint32_t cr;
            uint64_t lr;
            uint64_t ctr;
            uint32_t xer;

            uint64_t fpr[NumFloatArchRegs];
            __uint128_t vpr[NumVecRegs];
            uint32_t fpscr;
            uint32_t vscr;


            /* Remote target is expected to have 174 registers in the 'g'
               packet with a total size of 1076 bytes */
            //uint8_t __padding[524];
        } __attribute__((packed)) r;
      public:
        char *data() const { return (char *)&r; }
        size_t size() const { return sizeof(r); }
        void getRegs(ThreadContext*);
        void setRegs(ThreadContext*) const;
        const std::string
        name() const
        {
            return gdb->name() + ".PowerGdbRegCache";
        }
    };

    PowerGdbRegCache regCache;

  public:
    RemoteGDB(System *_system, ThreadContext *tc, int _port);
    BaseGdbRegCache *gdbRegs();
    std::vector<std::string>
    availableFeatures() const
    {
        return {"qXfer:features:read+"};
    };
    bool getXferFeaturesRead(const std::string &annex, std::string &output);
};

} // namespace PowerISA

#endif /* __ARCH_POWER_REMOTE_GDB_H__ */
