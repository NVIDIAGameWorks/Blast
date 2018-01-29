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
// Copyright (c) 2016-2018 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTEXTAPEXSHAREDPARTS_H
#define NVBLASTEXTAPEXSHAREDPARTS_H

#include "NvBlast.h"
#include <PxPlane.h>
namespace physx
{
	class PxVec3;
	class PxTransform;
	class PxBounds3;
}

namespace Nv
{
namespace Blast
{

struct Separation
{
	physx::PxPlane	plane;
	float	min0, max0, min1, max1;

	float getDistance()
	{
		return physx::PxMax(min0 - max1, min1 - max0);
	}
};

/**
	Function to compute midplane between two convex hulls. Is copied from APEX.
*/
bool importerHullsInProximityApexFree(	uint32_t hull0Count, const physx::PxVec3* hull0, physx::PxBounds3& hull0Bounds, const physx::PxTransform& localToWorldRT0In, const physx::PxVec3& scale0In,
										uint32_t hull1Count, const physx::PxVec3* hull1, physx::PxBounds3& hull1Bounds, const physx::PxTransform& localToWorldRT1In, const physx::PxVec3& scale1In,
										physx::PxF32 maxDistance, Separation* separation);

} // namespace Blast
} // namespace Nv


#endif // NVBLASTEXTAPEXSHAREDPARTS_H
