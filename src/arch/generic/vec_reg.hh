/*
 * Copyright (c) 2015-2018 ARM Limited
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

/** \file arch/generic/vec_reg.hh
 * Vector Registers layout specification.
 *
 * This register type is to be used to model the SIMD registers.
 * It takes into account the possibility that different architectural names
 * may overlap (like for ARMv8 AArch32 for example).
 *
 * The design is having a basic vector register container that holds the
 * bytes, unaware of anything else. This is implemented by VecRegContainer.
 * As the (maximum) length of the physical vector register is a compile-time
 * constant, it is defined as a template parameter.
 *
 * This file also describe one view of the container that has semantic
 * information about the bytes, the VecRegT.
 *    A VecRegT is a view of a VecRegContainer (by reference). The VecRegT has
 *    a type (VecElem) to which bytes are casted, and the amount of such
 *    elements that the vector contains (NumElems). The size of a view,
 *    calculated as sizeof(VecElem) * NumElems must match the size of the
 *    underlying container. As VecRegT has some degree of type information it
 *    has vector semantics, and defines the index operator ([]) to get
 *    references to particular bytes understood as a VecElem.
 *
 * The intended usage is requesting views to the VecRegContainer via the
 * member 'as' for VecRegT.
 *
 * // We declare 512 bits vectors
 * using Vec512 = VecRegContainer<64>;
 * ...
 * // We implement the physical vector register file
 * Vec512 physicalVecRegFile[NUM_VREGS];
 * ...
 * // Usage example, for a macro op:
 * VecFloat8Add(ExecContext* xd) {
 *    // Request source vector register to the execution context (const as it
 *    // is read only).
 *    const Vec512& vsrc1raw = xc->readVecRegOperand(this, 0);
 *    // View it as a vector of floats (we could just specify the first
 *    // template parametre, the second has a default value that works, and the
 *    // last one is derived by the constness of vsrc1raw).
 *    VecRegT<float, 8, true>& vsrc1 = vsrc1raw->as<float, 8>();
 *
 *    // Second source and view
 *    const Vec512& vsrc2raw = xc->readVecRegOperand(this, 1);
 *    VecRegT<float, 8, true>& vsrc2 = vsrc2raw->as<float, 8>();
 *
 *    // Destination and view
 *    Vec512 vdstraw;
 *    VecRegT<float, 8, false>& vdst = vdstraw->as<float, 8>();
 *
 *    for (auto i = 0; i < 8; i++) {
 *        // This asignment sets the bits in the underlying Vec512: vdstraw
 *        vdst[i] = vsrc1[i] + vsrc2[i];
 *    }
 *    xc->setWriteRegOperand(this, 0, vdstraw);
 * }
 *
 */

#ifndef __ARCH_GENERIC_VEC_REG_HH__
#define __ARCH_GENERIC_VEC_REG_HH__

#include <array>
#include <cstdint>
#include <iostream>
#include <string>

#include "base/cprintf.hh"
#include "base/logging.hh"
#include "sim/serialize_handlers.hh"

namespace gem5
{

constexpr unsigned MaxVecRegLenInBytes = 4096;

template <size_t Sz>
class VecRegContainer;

/** Vector Register Abstraction
 * This generic class is a view in a particularization of MVC, to vector
 * registers. There is a VecRegContainer that implements the model, and
 * contains the data. To that model we can interpose different instantiations
 * of VecRegT to view the container as a vector of NumElems elems of type
 * VecElem.
 * @tparam VecElem Type of each element of the vector.
 * @tparam NumElems Amount of components of the vector.
 * @tparam Const Indicate if the underlying container can be modified through
 * the view.
 */
template <typename VecElem, size_t NumElems, bool Const>
class VecRegT
{
    /** Size of the register in bytes. */
    static constexpr inline size_t
    size()
    {
        return sizeof(VecElem) * NumElems;
    }
  public:
    /** Container type alias. */
    using Container = typename std::conditional<Const,
                                              const VecRegContainer<size()>,
                                              VecRegContainer<size()>>::type;
  private:
    /** My type alias. */
    using MyClass = VecRegT<VecElem, NumElems, Const>;
    /** Reference to container. */
    Container& container;

  public:
    /** Constructor. */
    VecRegT(Container& cnt) : container(cnt) {};

    /** Zero the container. */
    template<bool Condition = !Const>
    typename std::enable_if_t<Condition, void>
    zero() { container.zero(); }

    template<bool Condition = !Const>
    typename std::enable_if_t<Condition, MyClass&>
    operator=(const MyClass& that)
    {
        container = that.container;
        return *this;
    }

    /** Index operator. */
    const VecElem& operator[](size_t idx) const
    {
        return container.template raw_ptr<VecElem>()[idx];
    }

    /** Index operator. */
    template<bool Condition = !Const>
    typename std::enable_if_t<Condition, VecElem&>
    operator[](size_t idx)
    {
        return container.template raw_ptr<VecElem>()[idx];
    }

    /** Equality operator.
     * Required to compare thread contexts.
     */
    template<typename VE2, size_t NE2, bool C2>
    bool
    operator==(const VecRegT<VE2, NE2, C2>& that) const
    {
        return container == that.container;
    }
    /** Inequality operator.
     * Required to compare thread contexts.
     */
    template<typename VE2, size_t NE2, bool C2>
    bool
    operator!=(const VecRegT<VE2, NE2, C2>& that) const
    {
        return !operator==(that);
    }

    /** Output stream operator. */
    friend std::ostream&
    operator<<(std::ostream& os, const MyClass& vr)
    {
        /* 0-sized is not allowed */
        os << "[" << std::hex << (uint32_t)vr[0];
        for (uint32_t e = 1; e < vr.size(); e++)
            os << " " << std::hex << (uint32_t)vr[e];
        os << ']';
        return os;
    }

    const std::string print() const { return csprintf("%s", *this); }
    /**
     * Cast to VecRegContainer&
     * It is useful to get the reference to the container for ISA tricks,
     * because casting to reference prevents unnecessary copies.
     */
    operator Container&() { return container; }
};

/* Forward declaration. */
template <typename VecElem, bool Const>
class VecLaneT;

/**
 * Vector Register Abstraction
 * This generic class is the model in a particularization of MVC, to vector
 * registers. The model has functionality to create views of itself, or a
 * portion through the method 'as
 * @tparam Sz Size of the container in bytes.
 */
template <size_t SIZE>
class VecRegContainer
{
  private:
    static_assert(SIZE > 0,
            "Cannot create Vector Register Container of zero size");
    static_assert(SIZE <= MaxVecRegLenInBytes,
            "Vector Register size limit exceeded");
  public:
    static constexpr inline size_t size() { return SIZE; };
    using Container = std::array<uint8_t, SIZE>;
  private:
    // 16-byte aligned to support 128bit element view
    alignas(16) Container container;

  public:
    VecRegContainer() {}
    VecRegContainer(const VecRegContainer &) = default;

    /** Zero the container. */
    void zero() { memset(container.data(), 0, SIZE); }

    /** Assignment operators. */
    /** @{ */
    /** From VecRegContainer */
    VecRegContainer<SIZE>&
    operator=(const VecRegContainer<SIZE>& that)
    {
        if (&that != this)
            std::memcpy(container.data(), that.container.data(), SIZE);
        return *this;
    }
    /** @} */

    /** Equality operator.
     * Required to compare thread contexts.
     */
    template<size_t S2>
    inline bool
    operator==(const VecRegContainer<S2>& that) const
    {
        return SIZE == S2 &&
               !memcmp(container.data(), that.container.data(), SIZE);
    }
    /** Inequality operator.
     * Required to compare thread contexts.
     */
    template<size_t S2>
    bool
    operator!=(const VecRegContainer<S2>& that) const
    {
        return !operator==(that);
    }

    /**
     * View interposers.
     * Create a view of this container as a vector of VecElems with an
     * optional amount of elements. If the amount of elements is provided,
     * the size of the container is checked, to test bounds. If it is not
     * provided, the length is inferred from the container size and the
     * element size.
     * @tparam VecElem Type of each element of the vector for the view.
     * @tparam NumElem Amount of elements in the view.
     */
    /** @{ */
    template <typename VecElem>
    VecElem *
    as()
    {
        static_assert(SIZE % sizeof(VecElem) == 0,
                "VecElem does not evenly divide the register size");
        return (VecElem *)container.data();
    }

    template <typename VecElem>
    const VecElem *
    as() const
    {
        static_assert(SIZE % sizeof(VecElem) == 0,
                "VecElem does not evenly divide the register size");
        return (VecElem *)container.data();
    }

    friend std::ostream&
    operator<<(std::ostream& os, const VecRegContainer<SIZE>& v)
    {
        // When printing for human consumption, break into 4 byte chunks.
        ccprintf(os, "[");
        size_t count = 0;
        for (auto& b: v.container) {
            if (count && (count % 4) == 0)
                os << "_";
            ccprintf(os, "%02x", b);
            count++;
        }
        ccprintf(os, "]");
        return os;
    }

    /** @} */
    /**
     * Used for serialization.
     */
    friend ShowParam<VecRegContainer<SIZE>>;
};

/**
 * Calls required for serialization/deserialization
 */
/** @{ */
template <size_t Sz>
struct ParseParam<VecRegContainer<Sz>>
{
    static bool
    parse(const std::string &str, VecRegContainer<Sz> &value)
    {
        fatal_if(str.size() > 2 * Sz,
                 "Vector register value overflow at unserialize");

        for (int i = 0; i < Sz; i++) {
            uint8_t b = 0;
            if (2 * i < value.size())
                b = stoul(str.substr(i * 2, 2), nullptr, 16);
            value.template as<uint8_t>()[i] = b;
        }
        return true;
    }
};

template <size_t Sz>
struct ShowParam<VecRegContainer<Sz>>
{
    static void
    show(std::ostream &os, const VecRegContainer<Sz> &value)
    {
        for (auto& b: value.container)
            ccprintf(os, "%02x", b);
    }
};
/** @} */

/**
 * Dummy type aliases and constants for architectures that do not implement
 * vector registers.
 */
/** @{ */
using DummyVecElem = uint32_t;
constexpr unsigned DummyNumVecElemPerVecReg = 2;
using DummyVecRegContainer =
    VecRegContainer<DummyNumVecElemPerVecReg * sizeof(DummyVecElem)>;
/** @} */

} // namespace gem5

#endif /* __ARCH_GENERIC_VEC_REG_HH__ */
