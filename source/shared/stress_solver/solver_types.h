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

#include "NvCTypes.h"

#include "simd/simd.h"


/**
 * Scalar types for SIMD and non-SIMD calculations.
 * Currently also used as a template argument to distinguish code paths.  May need a different
 * scheme if two codepaths use the same scalar type.
 */
typedef __m128  SIMD_Scalar;
typedef float   Float_Scalar;


/**
 * Holds an angular and linear component, for angular and linear accelerations, torques and forces, etc.
 */
SIMD_ALIGN_32(
struct AngLin6
{
    SIMD_ALIGN_16(NvcVec3 ang);
    SIMD_ALIGN_16(NvcVec3 lin);
}
);


/**
 * Holds the components of a rigid body description that are necessary for the stress solver.
 */
template<typename InertiaType>
struct SolverNode
{
    NvcVec3     CoM;
    float       mass;
    InertiaType inertia;
};

typedef SolverNode<float>       SolverNodeS;
typedef SolverNode<NvcVec3>     SolverNodeD;
typedef SolverNode<NvcMat33>    SolverNodeG;


/**
 * Holds the components of a rigid body bond description that are necessary for the stress solver.
 */
struct SolverBond
{
    NvcVec3     centroid;
    uint32_t    nodes[2];   // Index into accompanying SolverNode<InertiaType> array.
};
