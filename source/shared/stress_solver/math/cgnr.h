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

#include <stdint.h>
#include <string>   // for memcpy, memset

#include "solver_common.h"
#include "simd/simd.h"


template<typename Elem, typename ElemOps, typename Mat, typename MatOps, typename Scalar = float>
struct CGNR
{
    /**
     * Conjugate Gradient Normal Equation Residual (CGNR) solver for systems of M equations and N unknowns.
     * 
     * Based on Matrix Computations (4th ed.) by Golub and Van Loan, section 11.3.9.
     * 
     * Solves A*x = b.
     * 
     * Template arguments:
     *  Elem: the type of element used in the vectors x and b, and (implicitly) in the matrix A.
     * 
     *  ElemOps: a class which defines various functions on Elem type and vectors of Elem type.
     * 
     *  Mat: the explicit type used to represent the matrix, allowing e.g. for sparse representations.
     * 
     *  MatOps: a class which defines the functions rmul and lmul, which multiply a matrix of type Mat
     *      by an Elem-typed vector on the right and left, respectively.  The function signatures must be:
     * 
     *      void rmul(Elem* y, const Mat& A, const Elem* x, uint32_t M, uint32_t N);    // y = A*x
     *      void lmul(Elem* y, const Elem* x, const Mat& A, uint32_t M, uint32_t N);    // y = x*A
     * 
     *  Scalar: set to float by default.  May be used to keep all operations in a particular representation, e.g. SIMD registers.
     * 
     * \param[out]  x           User-supplied Elem vector of length N, filled with the solution upon exit (if successful).
     * \param[in]   A           System M x N matrix of type Mat.
     * \param[in]   b           Right hand side of equation to be solved, an Elem vector of length M.
     * \param[in]   M           The number of rows in A and elements in b.
     * \param[in]   N           The number of columns in A and elements in x.
     * \param[in]   cache       Cache memory provided by the user, must be at least 2*(M+N+1)*sizeof(Elem) bytes, and 16-byte aligned.
     * \param[out]  error_sq    If not null, returns the square magnitude of the angular (error_sq.ang) and linear (error_sq.lin) parts of the residual.
     * \param[in]   tol         (Optional) relative convergence threshold for |Ax-b|/|b|.  Default value is 10^-6.
     * \param[in]   max_it      (Optional) the maximum number of internal iterations.  If set to 0, the maximum is N.  Default value is 0.
     * \param[in]   warm        (Optional) if true, use the data in x as an initial trial solution.  Default value is false.
     *                          N.B. if warm == true, then the cache *MUST* be untouched since the last call to this function or cleared using clear_solver_cache(...).
     * 
     * return the number of iterations taken to converge, if it converges.  Otherwise, returns minus the number of iterations before exiting.
     */
    int
    solve
    (
        Elem* x,
        const Mat& A,
        const Elem* b,
        uint32_t M,
        uint32_t N,
        void* cache,
        SolverError* error_sq = nullptr,
        float tol = 1.e-6f,
        uint32_t max_it = 0,
        bool warm = false
    )
    {
        // Cache and temporary storage
        static_assert(sizeof(Elem) >= sizeof(Scalar), "sizeof(Elem) must be at least as great as sizeof(Scalar).");
        float* z_last_sq_mem = (float*)cache; cache = (Elem*)z_last_sq_mem + 1; // Elem-sized storage
        float* delta_sq_mem = (float*)cache; cache = (Elem*)delta_sq_mem + 1;   // Elem-sized storage
        Elem* z = (Elem*)cache; cache = z + N;  // Array of length N
        Elem* p = (Elem*)cache; cache = p + N;  // Array of length N
        Elem* r = (Elem*)cache; cache = r + M;  // Array of length M
        Elem* s = (Elem*)cache;                 // Array of length M

        Scalar z_last_sq;
        load_float(z_last_sq, z_last_sq_mem);
        const bool cache_clear = *z_last_sq_mem == 0.0f;

        Scalar z_ang_sq, z_lin_sq;
        set_zero(z_ang_sq); // Zeroing of these is not needed, it just keeps the compiler from fretting
        set_zero(z_lin_sq);

        Scalar delta_sq;

        if (warm && !cache_clear) load_float(delta_sq, delta_sq_mem);   // Load delta_sq from cache and we're good to go
        else
        {
            delta_sq = (tol*tol)*ElemOps().length_sq(b, M); // Calculate allowed residual length squared and cache it
            store_float(delta_sq_mem, delta_sq);
            memcpy(r, b, sizeof(Elem)*M);                   // Initialize residual r = b
            if (warm)                                       // Warm start, r = b - A*x
            {
                MatOps().rmul(s, A, x, M, N);
                for (uint32_t i = 0; i < M; ++i) ElemOps().sub(r[i], r[i], s[i]);
            }
            else memset(x, 0, sizeof(Elem)*N);              // Cold start, x = 0 so r = b
            warm = false;                                   // This lets p be initialized in the loop below
        }

        // Iterate
        if (!max_it) max_it = N + (uint32_t)!N;                                     // Ensure max_it > 0
        uint32_t it = 0;
        for (; it < max_it; ++it)
        {
            MatOps().lmul(z, r, A, M, N);                                           // Set z = (A^T)*r
            ElemOps().split_length_sq(z_ang_sq, z_lin_sq, z, N);                    // Calculate residual (of modified equation) length squared
            const Scalar z_sq = z_ang_sq + z_lin_sq;
            if (z_sq <= delta_sq) break;                                            // Terminate (convergence) if within tolerance
            if (!warm)                                                              // On the first (cold) iteration set p = z
            {
                memcpy(p, z, sizeof(Elem)*N);
                warm = true;
            }
            else                                                                    // After that, p = z + (|z|^2/|z_last|^2)*p
                for (uint32_t i = 0; i < N; ++i) ElemOps().madd(p[i], z_sq/z_last_sq, p[i], z[i]);
            z_last_sq = z_sq;
            MatOps().rmul(s, A, p, M, N);                                           // Calculate s = A*p
            const Scalar mu = z_sq/ElemOps().length_sq(s, M);                       // mu = |z|^2 / |A*p|^2
            for (uint32_t i = 0; i < N; ++i) ElemOps().madd(x[i], mu, p[i], x[i]);  // x += mu*p
            for (uint32_t i = 0; i < M; ++i) ElemOps().nmadd(r[i], mu, s[i], r[i]); // r -= mu*s
        }

        // Store off remainder of state (the rest was maintained in memory with array operations)
        store_float(z_last_sq_mem, z_last_sq);

        // Store off the error if requested
        if (error_sq) *error_sq = { to_float(z_ang_sq), to_float(z_lin_sq) };

        return it < max_it ? (int)it : -(int)it;
    }

    /**
     * \param[in]   M   See solve(...) for a description.
     * \param[in]   N   See solve(...) for a description.

     * \return the required cache size (in bytes) for the given values of M and N.
     */
    size_t required_cache_size(uint32_t M, uint32_t N) { return 2*(M+N+1)*sizeof(Elem); }
};
