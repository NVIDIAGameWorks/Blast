/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTEXTAPEXSHAREDPARTS_H
#define NVBLASTEXTAPEXSHAREDPARTS_H

#include "NvBlast.h"
#include <vector>
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
bool importerHullsInProximityApexFree(const std::vector<physx::PxVec3>& hull0, physx::PxBounds3& hull0Bounds, const physx::PxTransform& localToWorldRT0In, const physx::PxVec3& scale0In,
	const std::vector<physx::PxVec3>& hull1, physx::PxBounds3& hull1Bounds, const physx::PxTransform& localToWorldRT1In, const physx::PxVec3& scale1In,
	physx::PxF32 maxDistance, Separation* separation);

} // namespace Blast
} // namespace Nv


#endif // NVBLASTEXTAPEXSHAREDPARTS_H
