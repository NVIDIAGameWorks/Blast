// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2022 NVIDIA Corporation. All rights reserved.

#pragma once

#include <utility>
#include <stdio.h>


inline static constexpr uint32_t
instSetCode(uint8_t fn, uint8_t bitset, uint8_t bit)
{
    return (uint32_t)fn << 16 | (uint32_t)bitset << 8 | (uint32_t)bit;
}

inline static void
extractInstSetBitsetAndBit(int& fn, int& bitset, int& bit, uint32_t code)
{
    fn = (int)(code >> 16);
    bitset = (int)(code >> 8)&0xff;
    bit = (int)(code & 0xff);
}

struct InstructionSet
{
    enum Enum
    {
        MMX = instSetCode(1, 3, 23),
        SSE = instSetCode(1, 3, 25),
        SSE2 = instSetCode(1, 3, 26),
        SSE3 = instSetCode(1, 2, 0),
        SSSE3 = instSetCode(1, 2, 9),
        SSE4_1 = instSetCode(1, 2, 19),
        SSE4_2 = instSetCode(1, 2, 20),
        OSXSAVE = instSetCode(1, 2, 27),
        AVX = instSetCode(1, 2, 28),
        AVX2 = instSetCode(7, 1, 5),
        FMA3 = instSetCode(1, 2, 12),
        AVX512F = instSetCode(7, 1, 16),
        AVX512PF = instSetCode(7, 1, 26),
        AVX512ER = instSetCode(7, 1, 27),
        AVX512CD = instSetCode(7, 1, 28)
    };
};

#define InstructionSetEntry(_name) { #_name, InstructionSet::_name }
constexpr std::pair<const char*, uint32_t> sInstructionSetLookup[] =
{
    InstructionSetEntry(MMX),
    InstructionSetEntry(SSE),
    InstructionSetEntry(SSE2),
    InstructionSetEntry(SSE3),
    InstructionSetEntry(SSSE3),
    InstructionSetEntry(SSE4_1),
    InstructionSetEntry(SSE4_2),
    InstructionSetEntry(OSXSAVE),
    InstructionSetEntry(AVX),
    InstructionSetEntry(AVX2),
    InstructionSetEntry(FMA3),
    InstructionSetEntry(AVX512F),
    InstructionSetEntry(AVX512PF),
    InstructionSetEntry(AVX512ER),
    InstructionSetEntry(AVX512CD),
};


#if NV_WINDOWS_FAMILY
#include <intrin.h> // for __cpuidex
inline void cpuid(int cpui[4], int fn) { __cpuidex(cpui, fn, 0); }
inline bool os_supports_avx_restore() { return ((uint32_t)_xgetbv(0) & 6) == 6; }
#else
#include <cpuid.h> // for __cpuid_count
inline void cpuid(int cpui[4], int fn) { __cpuid_count(fn, 0, cpui[0], cpui[1], cpui[2], cpui[3]); }
inline bool os_supports_avx_restore()
{
    uint32_t xcr0;
    __asm__("xgetbv" : "=a" (xcr0) : "c" (0) : "%edx");
    return (xcr0 & 6) == 6;
}
#endif

static bool
device_supports_instruction_set(uint32_t inst_set)
{
    int fn, bitset, bit;
    extractInstSetBitsetAndBit(fn, bitset, bit, inst_set);

    int cpui[4];
    cpuid(cpui, 0);

    if (cpui[0] < fn) return false;

    cpuid(cpui, fn);

    return !!((cpui[bitset] >> bit) & 1);
}

static void
print_supported_instruction_sets()
{
    printf("Supported instruction sets:\n");
    for (std::pair<const char*, uint32_t> entry : sInstructionSetLookup)
    {
        printf("%s: %s\n", entry.first, device_supports_instruction_set(entry.second) ? "yes" : "no");
    }
}
