#include "arch/power/radixwalk.hh"

#include <memory>

#include "arch/power/faults.hh"
#include "arch/power/miscregs.hh"
#include "arch/power/tlb.hh"
#include "base/bitfield.hh"
#include "cpu/base.hh"
#include "cpu/thread_context.hh"
#include "debug/RadixWalk.hh"
#include "mem/packet_access.hh"
#include "mem/request.hh"

#define PRTB_SHIFT     12
#define PRTB_MASK      0x0ffffffffffff
#define PRTB_ALIGN     4
#define TABLE_BASE_ALIGN     PRTB_SHIFT
#define DSISR_MASK    0x00000000ffffffff

#define RPDB_SHIFT     8
#define RPDB_MASK      0x0fffffffffffff

#define RPDS_SHIFT     0
#define RPDS_MASK      0x1f

#define NLB_SHIFT      RPDB_SHIFT
#define NLB_MASK       RPDB_MASK

#define NLS_SHIFT      RPDS_SHIFT
#define NLS_MASK       RPDS_MASK

#define DIR_BASE_ALIGN  RPDB_SHIFT

#define RTS1_SHIFT     61
#define RTS1_MASK      0x3
#define RTS2_BITS      3
#define RTS2_SHIFT     5
#define RTS2_MASK      ((1 << RTS2_BITS) - 1)

#define   NOHPTE      0x0000000040000000
            /*Bit-33 Acc to ISA: no translation found */
#define  PROTFAULT   0x0000000008000000
           /* Bit-36 Acc to ISA:protection fault */
#define  ISSTORE     0x0000000002000000
           /* Bit-38 Acc to ISA:access was a store */
#define  UNSUPP_MMU  0x0000000000080000
           /*Bit-44 P9: Unsupported MMU config */
#define  PRTABLE_FAULT 0x0000000000020000
          /*Bit-46  P9: Fault on process table */

#define RPN_MASK      0x01fffffffffff000

#define QUADRANT_MASK 0xc000000000000000
#define QUADRANT00   0x0000000000000000
#define QUADRANT01   0x4000000000000000
#define QUADRANT10   0x8000000000000000
#define QUADRANT11   0xc000000000000000

#define extract(x, shift, mask)   ((x >> shift) & mask)
#define align(x, bits) (x << bits)
#define setBitMask(shift) ( (uint64_t)1 << shift)
#define unsetMask(start ,end)(~((setBitMask(start))-1) | ((setBitMask(end))-1))

#define getRTS(x)      ((extract(x, RTS1_SHIFT, RTS1_MASK) << RTS2_BITS) | \
                        (extract(x, RTS2_SHIFT, RTS2_MASK)))

namespace PowerISA {

uint64_t
RadixWalk::readPhysMem(uint64_t addr, uint64_t dataSize)
{
    uint64_t ret;
    Request::Flags flags = Request::PHYSICAL;

    RequestPtr request = std::make_shared<Request>(
            addr, dataSize, flags, this->requestorId);

    Packet *read = new Packet(request, MemCmd::ReadReq);
    read->allocate();
    this->port.sendAtomic(read);
    ret = read->getBE<uint64_t>();

    read->deleteData();
    delete &read->req;
    delete read;

    return ret;
}

uint64_t
RadixWalk::writePhysMem(uint64_t addr, uint64_t dataSize)
{
    uint64_t ret;
    Request::Flags flags = Request::PHYSICAL;

    RequestPtr request = std::make_shared<Request>(
            addr, dataSize, flags, this->requestorId);

    Packet *write = new Packet(request, MemCmd::WriteReq);
    write->allocate();
    this->port.sendAtomic(write);
    ret = write->getBE<uint64_t>();

    write->deleteData();
    delete &write->req;
    delete write;

    return ret;
}

Fault
RadixWalk::prepareDSI(ThreadContext * tc, RequestPtr req,
                    BaseTLB::Mode mode, uint64_t BitMask)
{
    uint64_t dsisr = tc->readIntReg(INTREG_DSISR);
    dsisr = (dsisr & (~(DSISR_MASK))) | BitMask;
    if (mode == BaseTLB::Write)
      dsisr = dsisr | ISSTORE;
    tc->setIntReg(INTREG_DSISR, dsisr);
    tc->setIntReg(INTREG_DAR, req->getVaddr());
    return std::make_shared<DataStorageInterrupt>();
}

Fault
RadixWalk::prepareISI(ThreadContext * tc, RequestPtr req,
                     uint64_t BitMask)
{
    Msr msr = tc->readIntReg(INTREG_MSR);
    //here unsetting SRR1 bits 33-36 and 42-47 according to ISA
    uint64_t srr1 = ((msr & unsetMask(31, 27)) & unsetMask(22,16)) | BitMask;
    tc->setIntReg(INTREG_SRR1, srr1);
    return std::make_shared<InstrStorageInterrupt>();
}

Fault
RadixWalk::prepareSI(ThreadContext * tc, RequestPtr req,
                    BaseTLB::Mode mode, uint64_t BitMask)
{
    if (mode != BaseTLB::Execute)
      return prepareDSI(tc, req, mode, BitMask);
    else
      return prepareISI(tc, req, BitMask);
}

uint32_t geteffLPID(ThreadContext *tc)
{
    Msr msr = tc->readIntReg(INTREG_MSR);

    if (msr.hv)
        return 0;

    return tc->readIntReg(INTREG_LPIDR);
}

uint32_t geteffPID(ThreadContext *tc, Addr vaddr)
{
    Msr msr = tc->readIntReg(INTREG_MSR);
    uint64_t quadrant = vaddr & QUADRANT_MASK;

    if (msr.hv && !msr.pr) { //Hypervisor Kernel
        switch(quadrant) {
        case QUADRANT11:
            return 0;
        case QUADRANT10:
            return 0;
        default:
            return tc->readIntReg(INTREG_PIDR);
        }
    }

    //Hypervisor Userspace or Guest
    switch(quadrant) {
    case QUADRANT11:
        return 0;
    case QUADRANT00:
       return tc->readIntReg(INTREG_PIDR);
    default:
       if (msr.hv)
           panic("Errorenous hypervisor Userspace Quadrant : %lx\n", quadrant);
       else
           panic("Errorenous Guest Quadrant : %lx\n", quadrant);
    }

    return tc->readIntReg(INTREG_PIDR);
}

Fault
RadixWalk::start(ThreadContext * tc, RequestPtr req, BaseTLB::Mode mode)
{
    Addr vaddr = req->getVaddr();
    // prte0 ---> Process Table Entry
    DPRINTF(RadixWalk,"Translating vaddr = 0x%016lx\n", vaddr);
    uint64_t prte0 = getRPDEntry(tc, vaddr);
    // rpdb, rpds --->  Root Page Directory Base and Size
    uint64_t rpdb = extract(prte0, RPDB_SHIFT, RPDB_MASK);
    uint64_t rpds = extract(prte0, RPDS_SHIFT, RPDS_MASK);
    // rts = Radix Tree Size - 31.
    uint64_t rts = getRTS(prte0);

    uint64_t nextLevelBase = align(rpdb, DIR_BASE_ALIGN);
    uint64_t nextLevelSize = rpds;

    // usefulBits ---> radix tree size + 31
    // These are the useful lower bits of vaddr used for
    // address translation.
    uint64_t usefulBits = rts + 31; //Typically is 52.

    DPRINTF(RadixWalk,"RPDB: 0x%lx RPDS: 0x%lx usefulBits: %ld\n"
            ,rpdb,rpds,usefulBits);

    std::pair<Addr, Fault> AddrTran;

    Lpcr lpcr = tc->readIntReg(INTREG_LPCR);
    Msr msr = tc->readIntReg(INTREG_MSR);
    AddrTran.first = vaddr;

    if (( lpcr.hr == 0 && lpcr.vc <= 3 ) ||
            ( msr.hv == 1 && msr.pr == 0 && msr.dr == 0 )) {
        AddrTran.second = prepareSI(tc, req, mode,
                       PRTABLE_FAULT);
      DPRINTF(RadixWalk,"Fault Generated due to Process table fault\n");
      return AddrTran.second;
    }

    AddrTran = this->walkTree(vaddr, nextLevelBase, tc, mode, req,
                                nextLevelSize, usefulBits);
    req->setPaddr(AddrTran.first);
    if (AddrTran.second == NoFault) {

      DPRINTF(RadixWalk,"Radix Translated 0x%016lx -> 0x%016lx\n",
      vaddr,AddrTran.first);
    }
    return AddrTran.second;
}

uint64_t
RadixWalk::getRPDEntry(ThreadContext * tc, Addr vaddr)
{
    Ptcr ptcr = tc->readIntReg(INTREG_PTCR);
    uint32_t efflpid = geteffLPID(tc);

    DPRINTF(RadixWalk,"PTCR:0x%016lx\n",(uint64_t)ptcr);
    DPRINTF(RadixWalk,"effLPID: %d\n",efflpid);

    //Accessing 2nd double word of partition table (pate1)
    //Ref: Power ISA Manual v3.0B, Book-III, section 5.7.6.1
    //           PTCR Layout
    // ====================================================
    // -----------------------------------------------
    // | /// |     PATB                | /// | PATS  |
    // -----------------------------------------------
    // 0     4                       51 52 58 59    63
    // PATB[4:51] holds the base address of the Partition Table,
    // right shifted by 12 bits.
    // This is because the address of the Partition base is
    // 4k aligned. Hence, the lower 12bits, which are always
    // 0 are ommitted from the PTCR.
    //
    // Thus, The Partition Table Base is obtained by (PATB << 12)
    //
    // PATS represents the partition table size right-shifted by 12 bits.
    // The minimal size of the partition table is 4k.
    // Thus partition table size = (1 << PATS + 12).
    //
    //        Partition Table
    //  ====================================================
    //  0    PATE0            63  PATE1             127
    //  |----------------------|----------------------|
    //  |                      |                      |
    //  |----------------------|----------------------|
    //  |                      |                      |
    //  |----------------------|----------------------|
    //  |                      |                      | <-- effLPID
    //  |----------------------|----------------------|
    //           .
    //           .
    //           .
    //  |----------------------|----------------------|
    //  |                      |                      |
    //  |----------------------|----------------------|
    //
    // The effective LPID  forms the index into the Partition Table.
    //
    // Each entry in the partition table contains 2 double words, PATE0, PATE1,
    // corresponding to that partition.
    //
    // In case of Radix, The structure of PATE0 and PATE1 is as follows.
    //
    //     PATE0 Layout
    // -----------------------------------------------
    // |1|RTS1|/|     RPDB          | RTS2 |  RPDS   |
    // -----------------------------------------------
    //  0 1  2 3 4                55 56  58 59      63
    //
    // HR[0] : For Radix Page table, first bit should be 1.
    // RTS1[1:2] : Gives one fragment of the Radix treesize
    // RTS2[56:58] : Gives the second fragment of the Radix Tree size.
    // RTS = (RTS1 << 3 + RTS2) + 31.
    //
    // RPDB[4:55] = Root Page Directory Base.
    // RPDS = Logarithm of Root Page Directory Size right shifted by 3.
    //        Thus, Root page directory size = 1 << (RPDS + 3).
    //        Note: RPDS >= 5.
    //
    //   PATE1 Layout
    // -----------------------------------------------
    // |///|       PRTB             |  //  |  PRTS   |
    // -----------------------------------------------
    // 0  3 4                     51 52  58 59     63
    //
    // PRTB[4:51]   = Process Table Base. This is aligned to size.
    // PRTS[59: 63] = Process Table Size right shifted by 12.
    //                Minimal size of the process table is 4k.
    //                Process Table Size = (1 << PRTS + 12).
    //                Note: PRTS <= 24.
    //
    //                Computing the size aligned Process Table Base:
    //                   table_base = (PRTB  & ~((1 << PRTS) - 1)) << 12
    //                Thus, the lower 12+PRTS bits of table_base will
    //                be zero.

    uint64_t pate1Addr = align(ptcr.patb, TABLE_BASE_ALIGN) +
            (efflpid * sizeof(uint64_t) * 2) + 8;
    uint64_t dataSize = 8;
    uint64_t pate1 = htog<uint64_t>(
            this->readPhysMem(pate1Addr, dataSize), ByteOrder::big);

    DPRINTF(RadixWalk,"2nd Double word of partition table entry: 0x%016lx\n",
            pate1);

    uint64_t prtb = extract(pate1, PRTB_SHIFT, PRTB_MASK);
    prtb = align(prtb, TABLE_BASE_ALIGN);

    //Ref: Power ISA Manual v3.0B, Book-III, section 5.7.6.2
    //
    //        Process Table
    // ==========================
    //  0    PRTE0            63  PRTE1             127
    //  |----------------------|----------------------|
    //  |                      |                      |
    //  |----------------------|----------------------|
    //  |                      |                      |
    //  |----------------------|----------------------|
    //  |                      |                      | <-- effPID
    //  |----------------------|----------------------|
    //           .
    //           .
    //           .
    //  |----------------------|----------------------|
    //  |                      |                      |
    //  |----------------------|----------------------|
    //
    // The effective Process id (PID) forms the index into the Process Table.
    //
    // Each entry in the partition table contains 2 double words, PRTE0, PRTE1,
    // corresponding to that process
    //
    // In case of Radix, The structure of PRTE0 and PRTE1 is as follows.
    //
    //     PRTE0 Layout
    // -----------------------------------------------
    // |/|RTS1|/|     RPDB          | RTS2 |  RPDS   |
    // -----------------------------------------------
    //  0 1  2 3 4                55 56  58 59      63
    //
    // RTS1[1:2] : Gives one fragment of the Radix treesize
    // RTS2[56:58] : Gives the second fragment of the Radix Tree size.
    // RTS = (RTS1 << 3 + RTS2) << 31,
    //        since minimal Radix Tree size is 4G.
    //
    // RPDB = Root Page Directory Base.
    // RPDS = Root Page Directory Size right shifted by 3.
    //        Thus, Root page directory size = RPDS << 3.
    //        Note: RPDS >= 5.
    //
    //   PRTE1 Layout
    // -----------------------------------------------
    // |                      ///                    |
    // -----------------------------------------------
    // 0                                            63
    // All bits are reserved.

    uint32_t effPID = geteffPID(tc, vaddr);
    DPRINTF(RadixWalk,"effPID=%d\n", effPID);
    uint64_t prte0Addr = prtb + effPID * sizeof(prtb) * 2 ;
    DPRINTF(RadixWalk,"Process table base: 0x%016lx\n",prtb);
    uint64_t prte0 = htog<uint64_t>(
            this->readPhysMem(prte0Addr, dataSize), ByteOrder::big);

    //prte0 ---> Process Table Entry

    DPRINTF(RadixWalk,"process table entry: 0x%016lx\n",prte0);
    return prte0;
}

std::pair<Addr, Fault>
RadixWalk::walkTree(Addr vaddr ,uint64_t curBase ,ThreadContext * tc ,
    BaseTLB::Mode mode , RequestPtr req, uint64_t curSize ,uint64_t usefulBits)
{
        uint64_t dataSize = 8;
        std::pair<Addr, Fault> AddrTran;

        if (curSize < 5) {
          DPRINTF(RadixWalk,"[PANIC] vaddr = %lx,Radix RPDS = %lx,< 5\n",
                       vaddr, curSize);
          AddrTran.first = vaddr;
          AddrTran.second = prepareSI(tc, req, mode,
                          UNSUPP_MMU);
          DPRINTF(RadixWalk,"Fault due to unsupported Radix Tree config\n");
        return AddrTran;
        }
        // vaddr                    64 Bit
        // vaddr |-----------------------------------------------------|
        //       | Unused    |  Used                                   |
        //       |-----------|-----------------------------------------|
        //       | 0000000   | usefulBits = X bits (typically 52)      |
        //       |-----------|-----------------------------------------|
        //       |           |<--Cursize---->|                         |
        //       |           |    Index      |                         |
        //       |           |    into Page  |                         |
        //       |           |    Directory  |                         |
        //       |-----------------------------------------------------|
        //                        |                       |
        //                        V                       |
        // PDE  |---------------------------|             |
        //      |V|L|//|  NLB       |///|NLS|             |
        //      |---------------------------|             |
        // PDE = Page Directory Entry                     |
        // [0] = V = Valid Bit                            |
        // [1] = L = Leaf bit. If 0, then                 |
        // [4:55] = NLB = Next Level Base                 |
        //                right shifted by 8              |
        // [59:63] = NLS = Next Level Size                |
        //            |    NLS >= 5                       |
        //            |                                   V
        //            |                     |--------------------------|
        //            |                     |   usfulBits = X-Cursize  |
        //            |                     |--------------------------|
        //            |---------------------><--NLS-->|                |
        //                                  | Index   |                |
        //                                  | into    |                |
        //                                  | PDE     |                |
        //                                  |--------------------------|
        //                                                    |
        // If the next PDE obtained by                        |
        // (NLB << 8 + 8 * index) is a                        |
        // nonleaf, then repeat the above.                    |
        //                                                    |
        // If the next PDE is a leaf,                         |
        // then Leaf PDE structure is as                      |
        // follows                                            |
        //                                                    |
        //                                                    |
        // Leaf PDE                                           |
        // |------------------------------|           |----------------|
        // |V|L|sw|//|RPN|sw|R|C|/|ATT|EAA|           | usefulBits     |
        // |------------------------------|           |----------------|
        // [0] = V = Valid Bit                                 |
        // [1] = L = Leaf Bit = 1 if leaf                      |
        //                      PDE                            |
        // [2] = Sw = Sw bit 0.                                |
        // [7:51] = RPN = Real Page Number,                    V
        //          real_page = RPN << 12 ------------->  Logical OR
        // [52:54] = Sw Bits 1:3                               |
        // [55] = R = Reference                                |
        // [56] = C = Change                                   V
        // [58:59] = Att =                                Physical Address
        //           0b00 = Normal Memory
        //           0b01 = SAO
        //           0b10 = Non Idenmpotent
        //           0b11 = Tolerant I/O
        // [60:63] = Encoded Access
        //           Authority
        //
        uint64_t shift = usefulBits - curSize;
        uint64_t mask = (1UL << curSize) - 1;
        uint64_t index = extract(vaddr, shift, mask);

        uint64_t entryAddr = curBase + (index * sizeof(uint64_t));
        Rpde rpde = htog<uint64_t>(
                this->readPhysMem(entryAddr, dataSize), ByteOrder::big);
        DPRINTF(RadixWalk,"rpde:0x%016lx\n",(uint64_t)rpde);
        usefulBits = usefulBits - curSize;
        Msr msr = tc->readIntReg(INTREG_MSR);

        if (rpde.valid == 0) {
            AddrTran.first = vaddr;
            Lpcr lpcr = tc->readIntReg(INTREG_LPCR);

            if (lpcr.hr == 0) {
              AddrTran.second = prepareSI(tc, req, mode,
                        PRTABLE_FAULT);
              DPRINTF(RadixWalk,"Fault generated due to invalid pt entry\n");
            }
            else if (msr.dr == 1 || msr.ir == 1) {
              AddrTran.second = prepareSI(tc, req, mode, NOHPTE);
              DPRINTF(RadixWalk,"Fault due to translation not found\n");
            }
        return AddrTran;
           }

        //Ref: Power ISA Manual v3.0B, Book-III, section 5. 7.10.2
        if (rpde.leaf == 1) {
                Rpte  rpte = (uint64_t)rpde;
                uint64_t realpn = rpde & RPN_MASK;
                uint64_t pageMask = (1UL << usefulBits) - 1;
                Addr paddr = (realpn & ~pageMask) | (vaddr & pageMask);
                DPRINTF(RadixWalk,"paddr:0x%016lx\n",paddr);
                AddrTran.second = NoFault;
                AddrTran.first = paddr;


                //Conditions for checking privileges and permissions
            if (mode == BaseTLB::Execute &&
                    (!rpte.exe || ( rpte.pri && msr.pr ))) {
              AddrTran.second = prepareISI(tc, req,
                            PROTFAULT);
              DPRINTF(RadixWalk,"Fault is due to protection violation\n");
              }

            else if ( ( mode == BaseTLB::Read && !rpte.read ) ||
                      ( mode == BaseTLB::Write && !rpte.r_w ) ||
                      (( mode != BaseTLB::Execute)
                                && (rpte.pri && msr.pr ))) {
              AddrTran.second = prepareDSI(tc, req, mode,
                                  PROTFAULT);
              DPRINTF(RadixWalk,"Fault is due to protection violation\n");
              }

            rpte.ref = 1;
            if (mode == BaseTLB::Write) {
                rpte.c = 1;
            }
            gtoh<uint64_t>(rpte, ByteOrder::big);
            this->writePhysMem(rpte, dataSize);

          return AddrTran;
        }

        uint64_t nextLevelBase = align(rpde.NLB, DIR_BASE_ALIGN);
        uint64_t nextLevelSize = rpde.NLS;
        DPRINTF(RadixWalk,"NLB: 0x%lx\n",(uint64_t)nextLevelBase);
        DPRINTF(RadixWalk,"NLS: 0x%lx\n",(uint64_t)nextLevelSize);
        DPRINTF(RadixWalk,"usefulBits: %lx",(uint64_t)usefulBits);
        return walkTree(vaddr, nextLevelBase, tc ,
                              mode, req, nextLevelSize, usefulBits);
}

void
RadixWalk::RadixPort::recvReqRetry()
{

}

bool
RadixWalk::RadixPort::recvTimingResp(PacketPtr pkt)
{
    return true;
}

Port &
RadixWalk::getRequestPort(const std::string &if_name, PortID idx)
{
    if (if_name == "port")
        return port;
    else{
        return ClockedObject::getPort(if_name, idx);
    }
}

}/* end namespace PowerISA */
