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
// Copyright (c) 2016-2021 NVIDIA Corporation. All rights reserved.

//! @file
//!
//! @brief Defines the API for the NvBlastExtAuthoring blast sdk extension's BooleanTool

#ifndef NVBLASTAUTHORINGBOOLEANTOOL_H
#define NVBLASTAUTHORINGBOOLEANTOOL_H

#include "NvBlastExtAuthoringTypes.h"

namespace Nv
{
namespace Blast
{

// Forward declaration
class Mesh;
class SpatialAccelerator;

/**
    Tool for performing boolean operations on polygonal meshes.
    Tool supports only closed meshes. Performing boolean on meshes with holes can lead to unexpected behavior, e.g. holes in result geometry.
*/
class BooleanTool
{
public:
    virtual ~BooleanTool() {}

    /**
     *  Release BooleanTool memory
     */
    virtual void release() = 0;

    /**
     *  Operation to perform
     */
    enum Op
    {
        Intersection,
        Union,
        Difference
    };

    /**
     *  Perform boolean operation on two polygonal meshes (A and B).
     *  \param[in] meshA    Mesh A
     *  \param[in] accelA   Spatial accelerator for meshA.  Can be nullptr.
     *  \param[in] meshB    Mesh B
     *  \param[in] accelB   Spatial accelerator for meshB.  Can be nullptr.
     *  \param[in] op       Boolean operation type (see BooleanTool::Op)
     *  \return new mesh result of the boolean operation.  If nullptr, result is the empty set.
     */
    virtual Mesh*   performBoolean(const Mesh* meshA, SpatialAccelerator* accelA, const Mesh* meshB, SpatialAccelerator* accelB, Op op) = 0;

    /**
     *  Test whether point contained in mesh.
     *  \param[in] mesh     Mesh geometry
     *  \param[in] accel    Spatial accelerator for mesh.  Can be nullptr.
     *  \param[in] point    Point which should be tested
     *  \return true iff point is inside of mesh
     */
    virtual bool    pointInMesh(const Mesh* mesh, SpatialAccelerator* accel, const NvcVec3& point) = 0;
};

}  // namespace Blast
}  // namespace Nv

#endif  // ifndef NVBLASTAUTHORINGBOOLEANTOOL_H
