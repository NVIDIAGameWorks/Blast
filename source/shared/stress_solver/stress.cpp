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

#include "stress.h"
#include "math/cgnr.h"
#include "simd/simd_device_query.h"

#include <algorithm>
#include <cmath>

#define MASS_AND_LENGTH_SCALING 1


typedef CGNR<AngLin6, AngLin6Ops<Float_Scalar>, BondMatrixS, BondMatrixOpsS<Float_Scalar>, Float_Scalar, AngLin6ErrorSq>    CGNR_SISD;
typedef CGNR<AngLin6, AngLin6Ops<SIMD_Scalar>, BondMatrixS, BondMatrixOpsS<SIMD_Scalar>, SIMD_Scalar, AngLin6ErrorSq>       CGNR_SIMD;


/**
 * StressProcessor static members
 */

// Check for SSE, AVX, and FMA
const bool
StressProcessor::s_use_simd =
    device_supports_instruction_set(InstructionSet::SSE) &&     // Basic SSE
    device_supports_instruction_set(InstructionSet::OSXSAVE) && // OS uses XSAVE and XRSTORE instructions allowing saving YMM registers on context switch
    device_supports_instruction_set(InstructionSet::AVX) &&     // Advanced Vector Extensions (256 bit operations)
    device_supports_instruction_set(InstructionSet::FMA3) &&    // Fused Multiply-Add instructions
    os_supports_avx_restore();                                  // OS has enabled the required extended state for AVX


/**
 * StressProcessor methods
 */

void
StressProcessor::prepare(const SolverNodeS* nodes, uint32_t N_nodes, const SolverBond* bonds, uint32_t N_bonds, const DataParams& params)
{
    m_recip_sqrt_m.resize(N_nodes);
    m_couplings.resize(N_bonds);
    m_rhs.resize(N_nodes);
    m_B_scratch.resize(N_nodes);
    m_solver_cache.resize(s_use_simd ? CGNR_SIMD().required_cache_size(N_nodes, N_bonds) : CGNR_SISD().required_cache_size(N_nodes, N_bonds));
    m_can_hot_start = false;

    // Calculate bond offsets and length scale
    uint32_t offsets_to_scale = 0;
    m_length_scale = 0.0f;
    for (uint32_t i = 0; i < N_bonds; ++i)
    {
        const SolverBond& bond = bonds[i];
        const uint32_t b0 = bond.nodes[0];
        const uint32_t b1 = bond.nodes[1];
        Coupling& c = m_couplings[i];

        NvcVec3 offset0, offset1;
        if (!params.centerBonds)
        {
            offset0 = nodes[b0].mass > 0 ? bond.centroid - nodes[b0].CoM : nodes[b1].CoM - bond.centroid;
            offset1 = nodes[b1].mass > 0 ? bond.centroid - nodes[b1].CoM : nodes[b0].CoM - bond.centroid;
        }
        else
        {
            if (nodes[b0].mass <= 0)
            {
                offset1 = bond.centroid - nodes[b1].CoM;
                offset0 = -offset1;
            }
            else
            if (nodes[b1].mass <= 0)
            {
                offset0 = bond.centroid - nodes[b0].CoM;
                offset1 = -offset0;
            }
            else
            {
                offset0 = 0.5f*(nodes[b1].CoM - nodes[b0].CoM);
                offset1 = -offset0;
            }
        }

        if (nodes[b0].mass > 0.0f)
        {
            ++offsets_to_scale;
            m_length_scale += std::sqrt(offset0|offset0);
        }
        if (nodes[b1].mass > 0.0f)
        {
            ++offsets_to_scale;
            m_length_scale += std::sqrt(offset1|offset1);
        }

        c.offset0 = offset0;
        c.node0 = bond.nodes[0];
        c.offset1 = offset1;
        c.node1 = bond.nodes[1];
    }
#if MASS_AND_LENGTH_SCALING
    m_length_scale = offsets_to_scale ? m_length_scale / offsets_to_scale : 1.0f;
#else
    m_length_scale = 1.0f;
#endif

    // Scale offsets by length scale
    const float recip_length_scale = 1.0f/m_length_scale;
    for (uint32_t j = 0; j < N_bonds; ++j)
    {
        Coupling& coupling = m_couplings[j];
        coupling.offset0 *= recip_length_scale;
        coupling.offset1 *= recip_length_scale;
    }

    // Set mass scale to geometric mean of the masses
    m_mass_scale = 0.0f;
    uint32_t nonzero_mass_count = 0;
    for (uint32_t i = 0; i < N_nodes; ++i)
    {
        if (nodes[i].mass > 0.0f)
        {
            m_mass_scale += std::log(nodes[i].mass);
            ++nonzero_mass_count;
        }
    }

#define MASS_SCALE (1.0f)
#if MASS_AND_LENGTH_SCALING
    m_mass_scale = MASS_SCALE*(nonzero_mass_count ? std::exp(m_mass_scale / nonzero_mass_count) : 1.0f);
#else
    m_mass_scale = MASS_SCALE;
#endif

    // Generate m^-1/2
    std::vector<InertiaS> invM(N_nodes);
    const float inertia_scale = m_mass_scale*m_length_scale*m_length_scale;
    if (!params.equalizeMasses)
    {
        for (uint32_t i = 0; i < N_nodes; ++i)
        {
            invM[i] =
            {
                nodes[i].inertia > 0.0f ? inertia_scale/nodes[i].inertia : 0.0f,
                nodes[i].mass > 0.0f ? m_mass_scale/nodes[i].mass : 0.0f
            };
            m_recip_sqrt_m[i] = { std::sqrt(invM[i].I), std::sqrt(invM[i].m) };
        }
    }
    else
    {
        for (uint32_t i = 0; i < N_nodes; ++i)
        {
            invM[i] =
            {
                nodes[i].inertia > 0.0f ? MASS_SCALE : 0.0f,
                nodes[i].mass > 0.0f ? MASS_SCALE : 0.0f
            };
            m_recip_sqrt_m[i] = { std::sqrt(invM[i].I), std::sqrt(invM[i].m) };
        }
    }

    // Create sparse matrix representation for B = (m^-1/2)*C
    m_B.set(m_couplings.data(), m_recip_sqrt_m.data(), m_B_scratch.data(), N_nodes, N_bonds);
}


int
StressProcessor::solve(AngLin6* impulses, const AngLin6* velocities, const SolverParams& params, AngLin6ErrorSq* error_sq /* = nullptr */)
{
    const InertiaS* sqrt_m_inv = m_recip_sqrt_m.data();
    const uint32_t N_nodes = getNodeCount();
    const uint32_t N_bonds = getBondCount();
    void* cache = m_solver_cache.data();

    const float recip_length_scale = 1.0f/m_length_scale;

    // Apply length and mass scaling to impulses if warm-starting
    if (params.warmStart)
    {
        const float recip_mass_scale = 1.0f/m_mass_scale;
        const float recip_linear_impulse_scale = recip_length_scale*recip_mass_scale;
        const float recip_angular_impulse_scale = recip_length_scale*recip_linear_impulse_scale;
        for (uint32_t j = 0; j < N_bonds; ++j)
        {
            impulses[j].ang *= recip_angular_impulse_scale;
            impulses[j].lin *= recip_linear_impulse_scale;
        }
    }

    // Calculate r.h.s. vector b = -(m^1/2)*velocities
    AngLin6* b = m_rhs.data();
    for (uint32_t i = 0; i < N_nodes; ++i)
    {
        const InertiaS& m_i = sqrt_m_inv[i];
        const AngLin6& v_i = velocities[i];
        AngLin6& b_i = b[i];
        b_i.ang = v_i.ang/(-(m_i.I > 0 ? m_i.I : 1.0f));
        b_i.lin = (-recip_length_scale/(m_i.m > 0 ? m_i.m : 1.0f))*v_i.lin;
    }

    // Solve B*J = b for J, where B = (m^-1/2)*C.
    // Since CGNR does this by solving (B^T)*B*J = (B^T)*b, this actually solves
    // (C^T)*(m^-1)*C*J = -(C^T)*v for J, which is the equation we really wanted to solve.
    const uint32_t maxIter = params.maxIter ? params.maxIter : 6*std::max(N_nodes, N_bonds);

    // Set solver warmth
    const unsigned warmth = params.warmStart ? (m_can_hot_start ? 2 : 1) : 0;

    // Choose solver based on parameters
    const int result = s_use_simd ?
        CGNR_SIMD().solve(impulses, m_B, b, N_nodes, N_bonds, cache, error_sq, params.solverTol, maxIter, warmth) :
        CGNR_SISD().solve(impulses, m_B, b, N_nodes, N_bonds, cache, error_sq, params.solverTol, maxIter, warmth);

    // Undo length and mass scaling
    const float linear_impulse_scale = m_length_scale*m_mass_scale;
    const float angular_impulse_scale = m_length_scale*linear_impulse_scale;
    for (uint32_t j = 0; j < N_bonds; ++j)
    {
        impulses[j].ang *= angular_impulse_scale;
        impulses[j].lin *= linear_impulse_scale;
    }

    m_can_hot_start = true;

    return result;
}


bool
StressProcessor::removeBond(uint32_t bondIndex)
{
    if (bondIndex >= getBondCount()) return false;

    m_couplings[bondIndex] = m_couplings.back();
    m_couplings.pop_back();
    --m_B.N;
    m_can_hot_start = false;

    return true;
}
