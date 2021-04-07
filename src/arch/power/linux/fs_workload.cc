/*
 * Copyright (c) 2010-2013, 2016 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2002-2006 The Regents of The University of Michigan
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

#include "arch/power/linux/fs_workload.hh"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "arch/generic/linux/threadinfo.hh"
#include "base/loader/dtb_file.hh"
#include "base/loader/object_file.hh"
#include "base/loader/symtab.hh"
#include "cpu/base.hh"
#include "cpu/pc_event.hh"
#include "cpu/thread_context.hh"
#include "debug/Loader.hh"
#include "kern/linux/events.hh"
#include "kern/linux/helpers.hh"
#include "kern/system_events.hh"
#include "mem/physical.hh"
#include "params/KernelWorkload.hh"
#include "sim/stat_control.hh"

using namespace Linux;

namespace PowerISA
{

FsLinux::FsLinux(const Params &p) : PowerISA::FsWorkload(p)
{

}

void
FsLinux::initState()
{
    PowerISA::FsWorkload::initState();

    //PowerSystem::initState();

    if (params()->early_kernel_symbols) {
        //ElfObject::loadGlobalSymbols(kernelSymtab, 0, 0, loadAddrMask);
        //ElfObject::loadGlobalSymbols(debugSymbolTable, 0, 0, loadAddrMask);
        auto phys_globals = kernelObj->symtab().globals()->mask(_loadAddrMask);
        kernelSymtab.insert(*phys_globals);
        Loader::debugSymbolTable.insert(*phys_globals);
    }

    // Setup boot data structure
    //Addr addr = 0;
    // Check if the kernel image has a symbol that tells us it supports
    // device trees.
    bool kernel_has_fdt_support =
        //kernelSymtab->findAddress("unflatten_device_tree", addr);
        kernelSymtab.find("unflatten_device_tree") != kernelSymtab.end();
    bool dtb_file_specified = params()->dtb_filename != "";
    kernel_has_fdt_support = true;
    if (kernel_has_fdt_support && dtb_file_specified) {
        // Kernel supports flattened device tree and dtb file specified.
        // Using Device Tree Blob to describe system configuration.
        inform("Loading DTB file: %s at address %#x\n", params()->dtb_filename,
                0x1800000 + _loadAddrOffset);

        auto *dtb_file = new ::Loader::DtbFile(params()->dtb_filename);
        if (!dtb_file) {
            fatal("couldn't load DTB file: %s\n", params()->dtb_filename);
        }

        if (!dtb_file->addBootCmdLine(
                    commandLine.c_str(), commandLine.size())) {
            warn("couldn't append bootargs to DTB file: %s\n",
                 params()->dtb_filename);
        }
        //dtb_file->setTextBase(0x1800000 +loadAddrOffset);
        //dtb_file->loadSections(physProxy);
        dtb_file->buildImage().
            offset(0x1800000 + _loadAddrOffset).
            write(system->physProxy);

        delete dtb_file;
    }
    if (kernelObj){
        inform("Loading kernel: %s at address %#x\n","kernel",
                        0x20000000);
        //kernelObj->setTextBase(0x20000000);
        //kernelObj->loadSections(physProxy);
        kernelObj->buildImage().
            offset(0x20000000).
            write(system->physProxy);
    }

    if (!params()->skiboot.empty()){
        inform("Loading skiboot: %s at address %#x\n",params()->skiboot,
                        0x0);
        Loader::ObjectFile *skiboot_file =
                Loader::createObjectFile(params()->skiboot,true);
        if (!skiboot_file){
                fatal("Could not load skiboot\n");
        }
        //skiboot_file->setTextBase(0x0);
        //skiboot_file->loadSections(physProxy);
        skiboot_file->buildImage().
            offset(0x0).
            write(system->physProxy);

    }
    if (!params()->initramfs.empty()){
        inform("Loading initramfs: %s at address %#x\n",params()->initramfs,
                        0x28000000);
        int fd = open(params()->initramfs.c_str(), O_RDONLY);
        if (fd < 0){
                fatal("Couldn't open %s file\n",params()->initramfs);
        }
        off_t off = lseek(fd, 0, SEEK_END);
        fatal_if(off < 0,
                "Failed to determine size of the object file %s\n",
                params()->initramfs);
        auto len = static_cast<size_t>(off);

        uint8_t *buffer = (uint8_t *)malloc(len + 1);
        if (!buffer){
                fatal("Malloc failed\n");
        }
        lseek(fd,0,SEEK_SET);
        int sz = read(fd, buffer, len);
        if (sz != len){
                fatal("Reading the initramfs file"
                        "failed len :: %d read :: %d\n",len,sz);
        }
        //buffer[len+1] = '\0';
        //const uint8_t *blob = (uint8_t *)malloc(len + 1);
        //strncpy()
        // load the file in memory
        //physProxy.memsetBlob(0x28000000,(const uint8_t *)buffer,len);
        system->physProxy.writeBlob(0x28000000, buffer, len);
        close(fd);

    }
}

FsLinux::~FsLinux()
{
}

} // namespace PowerISA
