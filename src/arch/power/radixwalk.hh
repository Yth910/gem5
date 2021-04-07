#ifndef __ARCH_POWER_RADIX_WALK_HH__
#define __ARCH_POWER_RADIX_WALK_HH__

#include "arch/power/tlb.hh"
#include "base/bitunion.hh"
#include "base/types.hh"
#include "mem/mem_object.hh"
#include "mem/packet.hh"
#include "params/PowerRadixWalk.hh"
#include "sim/faults.hh"
#include "sim/system.hh"

class ThreadContext;

namespace PowerISA
{
    class RadixWalk : public ClockedObject
    {
      protected:
        // Port for accessing memory
        class RadixPort : public RequestPort
        {
          public:
            RadixPort(const std::string &_name, RadixWalk * _rwalk) :
                  RequestPort(_name, _rwalk), rwalk(_rwalk)
            {}

          protected:
            RadixWalk *rwalk;
            bool recvTimingResp(PacketPtr pkt);
            void recvReqRetry();
        };

        friend class RadixPort;
        RadixPort port;
        System * sys;
        RequestorID requestorId;

      public:
        uint64_t readPhysMem(uint64_t addr, uint64_t dataSize);
        uint64_t writePhysMem(uint64_t addr, uint64_t dataSize);

        BitUnion64(Rpde)
                Bitfield<63> valid;
                Bitfield<62> leaf;
                Bitfield<59, 8>  NLB;
                Bitfield<4, 0> NLS;
        EndBitUnion(Rpde)

        BitUnion64(Rpte)
                Bitfield<63> valid;
                Bitfield<62> leaf;
                Bitfield<61> sw1;
                Bitfield<56,12> rpn;
                Bitfield<11,9> sw2;
                Bitfield<8> ref;
                Bitfield<7> c;
                Bitfield<5,4> att;
                Bitfield<3> pri;
                Bitfield<2> read;
                Bitfield<1> r_w;
                Bitfield<0> exe;
        EndBitUnion(Rpte)

        Fault start(ThreadContext * _tc, RequestPtr req, BaseTLB::Mode mode);
        Port &getRequestPort(const std::string &if_name,
                                      PortID idx = InvalidPortID);

        Addr getRPDEntry(ThreadContext * tc, Addr vaddr);

        std::pair<Addr,Fault> walkTree(Addr vaddr ,uint64_t curBase ,
            ThreadContext * tc ,BaseTLB::Mode mode , RequestPtr req,
                         uint64_t curSize ,uint64_t usefulBits);
        Fault prepareSI(ThreadContext * tc,
              RequestPtr req, BaseTLB::Mode mode, uint64_t BitMask);

        Fault prepareISI(ThreadContext * tc,
              RequestPtr req, uint64_t BitMask);

        Fault prepareDSI(ThreadContext * tc, RequestPtr req,
                         BaseTLB::Mode mode,uint64_t BitMask);

        typedef PowerRadixWalkParams Params;

        RadixWalk(const Params &params) :
            ClockedObject(params), port(name() + ".port", this),
            sys(params.system),
            requestorId(sys->getRequestorId(this, name()))
        {
        }
    };
}
#endif // __ARCH_POWER_RADIX_WALK_HH__
