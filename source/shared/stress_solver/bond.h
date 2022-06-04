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

#include "coupling.h"
#include "inertia.h"
#include "anglin6.h"


/**
 * BondMatrix
 * 
 * When this matrix is applied to a vector of bond impulses, the result is a vector of the
 * differences between the the resulting velocities of the nodes joined by each bond.
 * 
 * This is done in block form, so a vector is composed of vector elements.  Each element
 * is a 6-dimensional vector, composed of a linear part followed by an angular part.
 * Matrix blocks are likewise 6x6.
 * 
 * This matrix is composed of two sparse matrices:
 *    An M x M block diagonal matrix m, where the i^th diagonal block is the 6x6 matrix:
 * 
 *             / I_i  0  \
 *      m_ii = |         |
 *             \  0  m_i /
 * 
 *    Except for possibly I_i, each "element" in m_ii is a multiple of the 3x3 unit matrix.  I_i is a
 *    3x3 symmetric inertia tensor.  See the definition of Inertia<TensorType> for its representation.
 * 
 *    The second component is the coupling matrix C, see documentation for Coupling.
 * 
 * The matrix represented by this object is (m^-1/2)*C, an M x N matrix.
 * 
 * NOTE: m, and C are _not_ stored as described above, for efficiency.
 */
template <typename TensorType>
struct BondMatrix
{
    /**
     * Set fields (shallow pointer copy).
     * 
     * \param[in]   _C          Coupling matrix, see the documentation for Coupling.
     * \param[in]   _sqrt_m_inv The inverse of the square root of the diagonal mass and inertia tensor, represented by a
     *                          vector of _M Inertia structs for the diagonal values.  The i^th element is the reciprocal
     *                          of the square root of the mass and inertia tensor of node i.
     * \param[in]   _scratch    Scratch memory required to carry out a multiply.  Must be at least _M*sizeof(AngLin6) bytes.
     * \param[in]   _M          The number of nodes.
     * \param[in]   _N          The number of bonds.
     */
    void
    set(const Coupling* _C, const Inertia<TensorType>* _sqrt_m_inv, void* _scratch, uint32_t _M, uint32_t _N)
    {
        C = _C;
        sqrt_m_inv = _sqrt_m_inv;
        scratch = _scratch;
        M = _M;
        N = _N;
    }

    const Coupling* C;
    const Inertia<TensorType>* sqrt_m_inv;
    void* scratch;
    uint32_t M, N;
};

typedef BondMatrix<float>       BondMatrixS;
typedef BondMatrix<NvcVec3>     BondMatrixD;
typedef BondMatrix<NvcMat33>    BondMatrixG;


template<typename TensorType, typename Scalar>
struct BondMatrixOps
{
    /**
     * Matrix-vector multiply y = B*x.
     * 
     * \param[out]  y   Resulting column vector of length N.
     * \param[in]   B   Input MxN matrix representation.
     * \param[in]   x   Input column vector of length M.
     * \param[in]   M   Number of rows in B.
     * \param[in]   N   Number of columns in B.
     */
    inline void
    rmul(AngLin6* y, const BondMatrix<TensorType>& B, const AngLin6* x, uint32_t M, uint32_t N) const
    {
        NV_UNUSED(M);   // BondMatrix stores these
        NV_UNUSED(N);

        // Calculate y = C*x (apply C)
        CouplingMatrixOps<AngLin6, Scalar>().rmul(y, B.C, x, B.M, B.N);

        // Calculate y = (m^-1/2)*C*x (apply m^-1/2)
        InertiaMatrixOps<Scalar>().mul(y, B.sqrt_m_inv, y, B.M);
    }

    /**
     * Matrix-vector multiply y = x*B.
     * 
     * \param[out]  y   Resulting row vector of length B.N.
     * \param[in]   x   Input row vector of length B.N.
     * \param[in]   B   Input matrix representation.
     * \param[in]   M   Number of rows in B.
     * \param[in]   N   Number of columns in B.
     */
    inline void
    lmul(AngLin6* y, const AngLin6* x, const BondMatrix<TensorType>& B, uint32_t M, uint32_t N) const
    {
        NV_UNUSED(M);   // BondMatrix stores these
        NV_UNUSED(N);

        AngLin6* s = (AngLin6*)B.scratch; // M-sized scratch s

        // Calculate s = (m^-1/2)*x (apply m^-1/2)
        InertiaMatrixOps<Scalar>().mul(s, B.sqrt_m_inv, x, B.M);

        // Calculate y = (C^T)*(m^-1/2)*x (apply C^T)
        CouplingMatrixOps<AngLin6, Scalar>().lmul(y, s, B.C, B.M, B.N);
    }
};

template<typename Scalar>
using BondMatrixOpsS = BondMatrixOps<float, Scalar>;

template<typename Scalar>
using BondMatrixOpsD = BondMatrixOps<float, NvcVec3>;

template<typename Scalar>
using BondMatrixOpsG = BondMatrixOps<float, NvcMat33>;
