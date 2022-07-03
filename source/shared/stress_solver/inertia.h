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

#include "solver_types.h"

#include "NvCMath.h"


/**
 * Holds an inertia component and a mass component.
 * The inertial component is represented by a TensorType, which may be a float (representing a multiple of
 * the unit matrix), an NvcVec3 (representing the non-zero components of a diagonal inertia tensor), or a
 * 3x3 symmetric matrix representing a general inertia tensor.
 * 
 * This structure might also be used to store reciprocals, or powers (e.g. square roots) of these quantities.
 */
template <typename TensorType>
struct Inertia
{
    TensorType  I;  
    float       m;
};

typedef Inertia<float>      InertiaS;
typedef Inertia<NvcVec3>    InertiaD;
typedef Inertia<NvcMat33>   InertiaG;


template<typename Scalar = Float_Scalar>
struct InertiaMatrixOps
{
    /**
     * Matrix-vector multiply y = I*x.
     * 
     * Apply a block-diagonal inertia matrix I to a vector of AngLin6 elements.
     * x and y may be the same vector.
     * 
     * \param[out]  y   Resulting column vector of length N.
     * \param[in]   I   Input inertia matrix representation.
     * \param[in]   x   Input column vector of length N.
     * \param[in]   N   Number of columns in x and y, and the square size of I.
     *
     * x and y may be the same vector.
     */
    inline void
    mul(AngLin6* y, const InertiaS* I, const AngLin6* x, uint32_t N)
    {
        for (uint32_t i = 0; i < N; ++i)
        {
            const InertiaS& I_i = I[i];
            const AngLin6& x_i = x[i];
            AngLin6& y_i = y[i];
            y_i.ang = I_i.I*x_i.ang;
            y_i.lin = I_i.m*x_i.lin;
        }
    }
};

template<>
struct InertiaMatrixOps<SIMD_Scalar>
{
    /**
     * Matrix-vector multiply y = I*x.
     * 
     * Apply a block-diagonal inertia matrix I to a vector of AngLin6 elements.
     * 
     * \param[out]  y   Resulting column vector of length N.
     * \param[in]   I   Input inertia matrix representation.
     * \param[in]   x   Input column vector of length N.
     * \param[in]   N   Number of columns in x and y, and the square size of I.
     *
     * x and y may be the same vector.
     */
    inline void
    mul(AngLin6* y, const InertiaS* I, const AngLin6* x, uint32_t N)
    {
        for (uint32_t i = 0; i < N; ++i)
        {
            const InertiaS& I_i = I[i];
            const AngLin6& x_i = x[i];
            AngLin6& y_i = y[i];

            __m256 _x = _mm256_load_ps(&x_i.ang.x);
            __m128 _Il = _mm_load1_ps(&I_i.I);
            __m128 _Ih = _mm_load1_ps(&I_i.m);
            __m256 _I = _mm256_set_m128(_Ih,_Il);
            __m256 _y = _mm256_mul_ps(_I, _x);
            _mm256_store_ps(&y_i.ang.x, _y);
        }
    }
};
