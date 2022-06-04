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

#include "NvCMath.h"
#include "simd/simd.h"


/**
 * Holds an angular and linear component, for angular and linear velocities, accelerations, impulses, torques and forces, etc.
 */
SIMD_ALIGN_32(
struct AngLin6
{
    SIMD_ALIGN_16(NvcVec3 ang);
    SIMD_ALIGN_16(NvcVec3 lin);
}
);


/**
 * Holds the angular and linear components of the calculated error.
 */
struct AngLin6ErrorSq
{
    float ang, lin;
};


/**
 * SISD AngLin6 operations.
 */
template<typename Scalar = float>
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
     * \param[out]  error_sq    Sum of the squared angular and linear parts of v.
     * \param[in]   v           Vector of AngLin6, of length N.
     * \param[in]   N           Number of elements in v.
     * 
     * \return the sum of the squared angular and linear errors, error.ang + error.lin.
     */
    inline float
    calculate_error(AngLin6ErrorSq& error_sq, const AngLin6* v, uint32_t N)
    {
        error_sq.ang = error_sq.lin = 0.0f;
        for (uint32_t i = 0; i < N; ++i)
        {
            const AngLin6& v_i = v[i];
            error_sq.ang += v_i.ang|v_i.ang;
            error_sq.lin += v_i.lin|v_i.lin;
        }
        return error_sq.ang + error_sq.lin;
    }
};


/**
 * SIMD AngLin6 operations.
 */
template<>
struct AngLin6Ops<__m128>
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
     * \param[out]  error_sq    Sum of the squared angular and linear parts of v.
     * \param[in]   v           Vector of AngLin6, of length N.
     * \param[in]   N           Number of elements in v.
     * 
     * \return the sum of the squared angular and linear errors, error.ang + error.lin.
     */
    inline __m128
    calculate_error(AngLin6ErrorSq& error_sq, const AngLin6* v, uint32_t N)
    {
        __m256 _res = _mm256_setzero_ps();
        for (uint32_t i = 0; i < N; ++i)
        {
            __m256 _v = _mm256_load_ps((const float*)(v+i));
            _res = _mm256_add_ps(_res, _mm256_dp_ps(_v, _v, 0x7f));
        }
        __m128 _ang_sq = _mm256_castps256_ps128(_res);
        __m128 _lin_sq = _mm256_extractf128_ps(_res, 1);

        _mm_store_ss(&error_sq.ang, _ang_sq);
        _mm_store_ss(&error_sq.lin, _lin_sq);

        return _mm_add_ps(_ang_sq, _lin_sq);
    }
};
