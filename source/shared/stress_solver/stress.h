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

#include "bond.h"
#include "math/solver_common.h"
#include <vector>


class StressProcessor
{
public:
    /** Parameters controling the data preparation. */
    struct DataParams
    {
        bool        equalizeMasses  = false;    // Use the geometric mean of the nodes' masses instead of the individual masses.
        bool        centerBonds     = false;    // Place the bond position halfway between adjoining nodes' CoMs.
    };

    /** Parameters controling the solver behavior. */
    struct SolverParams
    {
        uint32_t    maxIter         = 0;        // The maximum number of iterations.  If 0, use CGNR for default value.
        float       solverTol       = 1.e-6f;   // The relative tolerance threshold for convergence.  Iteration will stop when this is reached.
        bool        warmStart       = false;    // Whether or not to use the solve function's 'forces' parameter as a starting input vector.
    };

    /**
     * Build the internal representation of the stress network from nodes and bonds.
     * This only needs to be called initially, and any time the nodes or bonds change.
     * 
     * \param[in]   nodes   Array of SolverNodeS (scalar inertia).
     * \param[in]   N_nodes Number of elements in the nodes array.
     * \param[in]   bonds   Array of SolverBond.  The node indices in each bond entry correspond to the ordering of the nodes array.
     * \param[in]   N_bonds Number of elements in the bonds array.
     * \param[in]   params  Parameters affecting the processing of the input data (see DataParams).
     */
    void        prepare(const SolverNodeS* nodes, uint32_t N_nodes, const SolverBond* bonds, uint32_t N_bonds, const DataParams& params);

    /**
     * Solve for the bond forces given the external accelerations on each node.  The function prepare(...) must be called
     * before this can be used, but then solve(...) may be called multiple times.
     * 
     * The vector elements hold linear and angular parts.  For the ext_accel array, those correspond to linear and angular accelerations.
     * For the forces array, those correspond to forces and torques.
     * 
     * \param[out]  forces      Output array of forces exerted by each bond.  For a warm start, this is also used as an input.
     *                          Must be of length N_bonds passed into the prepare(...) function.
     * \param[in]   ext_accel   Input array of external accelerations on each node.  Must be of length N_nodes passed into the prepare(...) function.
     * \param[in]   params      Parameters affecting the solver characteristics (see SolverParams).
     * \param[out]  error_sq    (Optional) If not NULL, *error_sq will be filled with the angular and linear square errors (solver residuals).
     * 
     * \return the number of iterations taken to converge, if it converges.  Otherwise, returns minus the number of iterations before exiting.
     */
    int         solve(AngLin6* forces, const AngLin6* ext_accel, const SolverParams& params, SolverError* error_sq = nullptr);

    /**
     * Removes the indexed bond from the solver.
     * 
     * \param[in]   bondIndex   The index of the bond to remove.  Must be less than getBondCount().
     * 
     * \return true iff successful.
     */
    bool        removeBond(uint32_t bondIndex);

    /**
     * \return the number of nodes in the stress network.  (Set by prepare(...).)
     */
    uint32_t    getNodeCount() const { return (uint32_t)m_recip_sqrt_m.size(); }

    /**
     * \return the number of bonds in the stress network.  (Set by prepare(...), possibly reduced by removeBond(...).)
     */
    uint32_t    getBondCount() const { return (uint32_t)m_couplings.size(); }

    /**
     * \return whether or not the solver uses SIMD.  If the device supports SSE, AVX, and FMA instruction sets, SIMD is used. 
     */
    static bool usingSIMD() { return s_use_simd; }

protected:
    float                   m_length_scale;
    float                   m_mass_scale;
    std::vector<InertiaS>   m_recip_sqrt_m;
    std::vector<Coupling>   m_couplings;
    BondMatrixS             m_B;
    std::vector<AngLin6>    m_rhs;
    std::vector<AngLin6>    m_B_scratch;
    std::vector<AngLin6>    m_solver_cache;

    static const bool       s_use_simd;
};
