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

#include "NvCMath.h"


template<typename Scalar = Float_Scalar>
struct AngLin6Ops
{
    /** r = x + y */
    inline  void add(AngLin6& r, const AngLin6& x, const AngLin6& y)            { r.ang = x.ang + y.ang; r.lin = x.lin + y.lin; }

    /** r = x - y */
    inline  void sub(AngLin6& r, const AngLin6& x, const AngLin6& y)            { r.ang = x.ang - y.ang; r.lin = x.lin - y.lin; }

    /** r = c*x + y */
    inline  void madd(AngLin6& r, float c, const AngLin6& x, const AngLin6& y)  { r.ang = c*x.ang + y.ang; r.lin = c*x.lin + y.lin; }

    /** r = -c*x + y */
    inline  void nmadd(AngLin6& r, float c, const AngLin6& x, const AngLin6& y) { r.ang = y.ang - c*x.ang; r.lin = y.lin - c*x.lin; }

    /** Vector add */
    inline  void vadd(AngLin6* r, const AngLin6* x, const AngLin6* y, uint32_t N)               { while (N--) add(*r++, *x++, *y++); }

    /** Vector sub */
    inline  void vsub(AngLin6* r, const AngLin6* x, const AngLin6* y, uint32_t N)               { while (N--) sub(*r++, *x++, *y++); }

    /** Vector madd */
    inline  void vmadd(AngLin6* r, float c, const AngLin6* x, const AngLin6* y, uint32_t N)     { while (N--) madd(*r++, c, *x++, *y++); }

    /** Vector nmadd */
    inline  void vnmadd(AngLin6* r, float c, const AngLin6* x, const AngLin6* y, uint32_t N)    { while (N--) nmadd(*r++, c, *x++, *y++); }

    /**
     * Vector-of-vectors dot product.
     * 
     * \param[in]   v   Vector of AngLin6, of length N.
     * \param[in]   w   Vector of AngLin6, of length N.
     * \param[in]   N   Number of elements in v and w.
     * 
     * return (v|w).
     */
    inline float
    dot(const AngLin6* v, const AngLin6* w, uint32_t N)
    {
        float result = 0.0f;
        for (uint32_t i = 0; i < N; ++i)
        {
            const AngLin6& v_i = v[i];
            const AngLin6& w_i = w[i];
            result += (v_i.ang|w_i.ang) + (v_i.lin|w_i.lin);
        }
        return result;
    }

    /**
     * Vector-of-vectors length squared.
     * 
     * Equivalent to dot(v, v N), but could be faster in some cases
     * 
     * \param[in]   v   Vector of AngLin6, of length N.
     * \param[in]   N   Number of elements in v.
     * 
     * return |v|^2.
     */
    inline float
    length_sq(const AngLin6* v, uint32_t N)
    {
        float result = 0.0f;
        for (uint32_t i = 0; i < N; ++i)
        {
            const AngLin6& v_i = v[i];
            result += (v_i.ang|v_i.ang) + (v_i.lin|v_i.lin);
        }
        return result;
    }

    /**
     * Vector-of-vectors length squared, split into angular and linear contributions.
     * 
     * \param[out]  ang_sq  Sum of the squared angular parts of v.
     * \param[out]  lin_sq  Sum of the squared linear parts of v.
     * \param[in]   v       Vector of AngLin6, of length N.
     * \param[in]   N       Number of elements in v.
     */
    inline void
    split_length_sq(float& ang_sq, float& lin_sq, const AngLin6* v, uint32_t N)
    {
        ang_sq = 0.0f;
        lin_sq = 0.0f;
        for (uint32_t i = 0; i < N; ++i)
        {
            const AngLin6& v_i = v[i];
            ang_sq += v_i.ang|v_i.ang;
            lin_sq += v_i.lin|v_i.lin;
        }
    }
};

template<>
struct AngLin6Ops<SIMD_Scalar>
{
    /** r = x + y */
    inline void
    add(AngLin6& r, const AngLin6& x, const AngLin6& y)
    {
        __m256 _x = _mm256_load_ps(&x.ang.x);
        __m256 _y = _mm256_load_ps(&y.ang.x);
        __m256 _r = _mm256_add_ps(_x, _y);
        _mm256_store_ps(&r.ang.x, _r);
    }

    /** r = x - y */
    inline void
    sub(AngLin6& r, const AngLin6& x, const AngLin6& y)
    {
        __m256 _x = _mm256_load_ps(&x.ang.x);
        __m256 _y = _mm256_load_ps(&y.ang.x);
        __m256 _r = _mm256_sub_ps(_x, _y);
        _mm256_store_ps(&r.ang.x, _r);
    }

    /** r = c*x + y */
    inline void
    madd(AngLin6& r, __m128 c, const AngLin6& x, const AngLin6& y)
    {
        __m256 _c = _mm256_set_m128(c, c);
        __m256 _x = _mm256_load_ps(&x.ang.x);
        __m256 _y = _mm256_load_ps(&y.ang.x);
        __m256 _r = _mm256_fmadd_ps(_c, _x, _y);
        _mm256_store_ps(&r.ang.x, _r);
    }

    /** r = -c*x + y */
    inline void
    nmadd(AngLin6& r, __m128 c, const AngLin6& x, const AngLin6& y)
    {
        __m256 _c = _mm256_set_m128(c, c);
        __m256 _x = _mm256_load_ps(&x.ang.x);
        __m256 _y = _mm256_load_ps(&y.ang.x);
        __m256 _r = _mm256_fnmadd_ps(_c, _x, _y);
        _mm256_store_ps(&r.ang.x, _r);
    }

    /** Vector add */
    inline  void vadd(AngLin6* r, const AngLin6* x, const AngLin6* y, uint32_t N)               { while (N--) add(*r++, *x++, *y++); }

    /** Vector sub */
    inline  void vsub(AngLin6* r, const AngLin6* x, const AngLin6* y, uint32_t N)               { while (N--) sub(*r++, *x++, *y++); }

    /** Vector madd */
    inline  void vmadd(AngLin6* r, __m128 c, const AngLin6* x, const AngLin6* y, uint32_t N)    { while (N--) madd(*r++, c, *x++, *y++); }

    /** Vector nmadd */
    inline  void vnmadd(AngLin6* r, __m128 c, const AngLin6* x, const AngLin6* y, uint32_t N)   { while (N--) nmadd(*r++, c, *x++, *y++); }

    /**
     * Vector-of-vectors dot product.
     * 
     * \param[in]   v   Vector of AngLin6, of length N.
     * \param[in]   w   Vector of AngLin6, of length N.
     * \param[in]   N   Number of elements in v and w.
     * 
     * return (v|w).
     */
    inline __m128
    dot(const AngLin6* v, const AngLin6* w, uint32_t N)
    {
        __m256 _res = _mm256_setzero_ps();
        for (uint32_t i = 0; i < N; ++i)
        {
            __m256 _v = _mm256_load_ps((const float*)(v+i));
            __m256 _w = _mm256_load_ps((const float*)(w+i));
            _res = _mm256_add_ps(_res, _mm256_dp_ps(_v, _w, 0x7f));
        }
        return _mm_add_ps(_mm256_castps256_ps128(_res), _mm256_extractf128_ps(_res, 1));
    }

    /**
     * Vector-of-vectors length squared.
     * 
     * Equivalent to dot(v, v N), but could be faster in some cases
     * 
     * \param[in]   v   Vector of AngLin6, of length N.
     * \param[in]   N   Number of elements in v.
     * 
     * return |v|^2.
     */
    inline __m128
    length_sq(const AngLin6* v, uint32_t N)
    {
        __m256 _res = _mm256_setzero_ps();
        for (uint32_t i = 0; i < N; ++i)
        {
            __m256 _v = _mm256_load_ps((const float*)(v+i));
            _res = _mm256_add_ps(_res, _mm256_dp_ps(_v, _v, 0x7f));
        }
        return _mm_add_ps(_mm256_castps256_ps128(_res), _mm256_extractf128_ps(_res, 1));
    }

    /**
     * Vector-of-vectors length squared, split into angular and linear contributions.
     * 
     * \param[out]  ang_sq  Sum of the squared angular parts of v.
     * \param[out]  lin_sq  Sum of the squared linear parts of v.
     * \param[in]   v       Vector of AngLin6, of length N.
     * \param[in]   N       Number of elements in v.
     */
    inline void
    split_length_sq(__m128& ang_sq, __m128& lin_sq, const AngLin6* v, uint32_t N)
    {
        __m256 _res = _mm256_setzero_ps();
        for (uint32_t i = 0; i < N; ++i)
        {
            __m256 _v = _mm256_load_ps((const float*)(v+i));
            _res = _mm256_add_ps(_res, _mm256_dp_ps(_v, _v, 0x7f));
        }
        ang_sq = _mm256_castps256_ps128(_res);
        lin_sq = _mm256_extractf128_ps(_res, 1);
    }
};


/**
 * BondMatrix
 * 
 * When this matrix is applied to a vector of bond forces and torques, the result is a vector
 * of the differences between the the resulting linear and angular accelerations of the nodes
 * joined by each bond.
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
 *    Whatever TensorType is, it must have a multiplication operator '*' that takes an TensorType
 *    on the left and a NvcVec3 on the right, and produces an NvcVec3 result.
 * 
 *    The second component is the coupling matrix C, see documentation for Coupling.
 * 
 * The matrix represented by this object is (m^-1/2)*C, an M x N matrix.
 * 
 * NOTE: m, and C are not stored as described above, for efficiency.
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
