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
// Copyright (c) 2020 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTVOLUMEINTEGRALS_H
#define NVBLASTVOLUMEINTEGRALS_H

#include "NvBlastPxSharedHelpers.h"
#include "NvBlastAssert.h"


namespace Nv {
namespace Blast{


/**
Calculate the volume and centroid of a closed mesh with outward-pointing normals.
\param[out] centroid    the calculated centroid of the given mesh
\param[in]  mesh        a class of templated type MeshQuery

MeshQuery must support the following functions:

size_t faceCount()
size_t vertexCount(size_t faceIndex)
NvcVec3 vertex(size_t faceIndex, size_t vertexIndex)

\return the volume of the given mesh
*/
template<class MeshQuery>
NV_INLINE float calculateMeshVolumeAndCentroid(NvcVec3& centroid, const MeshQuery& mesh)
{
    centroid = { 0.0f, 0.0f, 0.0f };

    // First find an approximate centroid for a more accurate calculation
    size_t N = 0;
    NvcVec3 disp = { 0.0f, 0.0f, 0.0f };
    for (size_t i = 0; i < mesh.faceCount(); ++i)
    {
        const size_t faceVertexCount = mesh.vertexCount(i);
        for (size_t j = 0; j < faceVertexCount; ++j)
        {
            disp = disp + mesh.vertex(i, j);
        }
        N += faceVertexCount;
    }

    if (N == 0)
    {
        return 0.0f;
    }

    disp = disp / (float)N;

    float sixV = 0.0f;
    for (size_t i = 0; i < mesh.faceCount(); ++i)
    {
        const size_t faceVertexCount = mesh.vertexCount(i);
        if (faceVertexCount < 3)
        {
            continue;
        }
        const NvcVec3 a = mesh.vertex(i, 0) - disp;
        NvcVec3 b = mesh.vertex(i, 1) - disp;
        for (size_t j = 2; j < faceVertexCount; ++j)
        {
            const NvcVec3 c = mesh.vertex(i, j) - disp;

            const float sixTetV =
                a.x * b.y * c.z - a.x * b.z * c.y - a.y * b.x * c.z +
                a.y * b.z * c.x + a.z * b.x * c.y - a.z * b.y * c.x;

            sixV += sixTetV;

            centroid = centroid + sixTetV*(a + b + c);

            b = c;
        }
    }

    // Extra factor of four to average tet vertices
    centroid = centroid / (4.0f * sixV) + disp;

    return std::abs(sixV) / 6.0f;
}


} // namespace Blast
} // namespace Nv


#endif // NVBLASTVOLUMEINTEGRALS_H
