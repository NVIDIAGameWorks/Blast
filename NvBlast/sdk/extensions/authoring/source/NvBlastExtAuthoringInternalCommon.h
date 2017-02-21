/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTINTERNALCOMMON_H
#define NVBLASTINTERNALCOMMON_H
#include "NvBlastExtAuthoringTypes.h"

using namespace physx;

namespace Nv
{
namespace Blast
{

/**
Edge representation with index of parent facet
*/
struct EdgeWithParent
{
	int32_t s, e; // Starting and ending vertices
	int32_t parent; // Parent facet index
	EdgeWithParent() : s(0), e(0), parent(0) {}
	EdgeWithParent(int32_t s, int32_t e, int32_t p) : s(s), e(e), parent(p) {}
};


/**
Comparator for sorting edges according to parent facet number.
*/
struct EdgeComparator
{
	bool operator()(const EdgeWithParent& a, const EdgeWithParent& b) const
	{
		if (a.parent == b.parent)
		{
			if (a.s == b.s)
			{
				return a.e < b.e;
			}
			else
			{
				return a.s < b.s;
			}
		}
		else
		{
			return a.parent < b.parent;
		}
	}
};


/**
Vertex projection direction flag.
*/
enum ProjectionDirections
{
	YZ_PLANE = 1 << 1,
	XY_PLANE = 1 << 2,
	ZX_PLANE = 1 << 3,

	OPPOSITE_WINDING = 1 << 4
};

/**
Computes best direction to project points.
*/
NV_FORCE_INLINE ProjectionDirections getProjectionDirection(const physx::PxVec3& normal)
{
	float maxv = std::max(abs(normal.x), std::max(abs(normal.y), abs(normal.z)));
	ProjectionDirections retVal;
	if (maxv == abs(normal.x))
	{
		retVal = YZ_PLANE;
		if (normal.x < 0) retVal = (ProjectionDirections)((int)retVal | (int)OPPOSITE_WINDING);
		return retVal;
	}
	if (maxv == abs(normal.y))
	{
		retVal = ZX_PLANE;
		if (normal.y > 0) retVal = (ProjectionDirections)((int)retVal | (int)OPPOSITE_WINDING);
		return retVal;
	}
	retVal = XY_PLANE;
	if (normal.z < 0) retVal = (ProjectionDirections)((int)retVal | (int)OPPOSITE_WINDING);
	return retVal;
}


/**
Computes point projected on given axis aligned plane.
*/
NV_FORCE_INLINE physx::PxVec2 getProjectedPoint(const physx::PxVec3& point, ProjectionDirections dir)
{
	if (dir & YZ_PLANE)
	{
		return physx::PxVec2(point.y, point.z);
	}
	if (dir & ZX_PLANE)
	{
		return physx::PxVec2(point.x, point.z);
	}
	return physx::PxVec2(point.x, point.y);
}

/**
Computes point projected on given axis aligned plane, this method is polygon-winding aware.
*/
NV_FORCE_INLINE physx::PxVec2 getProjectedPointWithWinding(const physx::PxVec3& point, ProjectionDirections dir)
{
	if (dir & YZ_PLANE)
	{
		if (dir & OPPOSITE_WINDING)
		{
			return physx::PxVec2(point.z, point.y);
		}
		else
		return physx::PxVec2(point.y, point.z);
	}
	if (dir & ZX_PLANE)
	{
		if (dir & OPPOSITE_WINDING)
		{
			return physx::PxVec2(point.z, point.x);
		}
		return physx::PxVec2(point.x, point.z);
	}
	if (dir & OPPOSITE_WINDING)
	{
		return physx::PxVec2(point.y, point.x);
	}
	return physx::PxVec2(point.x, point.y);
}



#define MAXIMUM_EXTENT 1000 * 1000 * 1000
#define BBOX_TEST_EPS 1e-5f 

/**
Test fattened bounding box intersetion.
*/
NV_INLINE bool  weakBoundingBoxIntersection(const physx::PxBounds3& aBox, const physx::PxBounds3& bBox)
{
	if (std::max(aBox.minimum.x, bBox.minimum.x) > std::min(aBox.maximum.x, bBox.maximum.x) + BBOX_TEST_EPS)
		return false;
	if (std::max(aBox.minimum.y, bBox.minimum.y) > std::min(aBox.maximum.y, bBox.maximum.y) + BBOX_TEST_EPS)
		return false;
	if (std::max(aBox.minimum.z, bBox.minimum.z) > std::min(aBox.maximum.z, bBox.maximum.z) + BBOX_TEST_EPS)
		return false;
	return true;
}



/**
Test segment vs plane intersection. If segment intersects the plane true is returned. Point of intersection is written into 'result'.
*/
NV_INLINE bool getPlaneSegmentIntersection(const PxPlane& pl, const PxVec3& a, const PxVec3& b, PxVec3& result)
{
	float div = (b - a).dot(pl.n);
	if (PxAbs(div) < 0.0001f)
	{
		if (pl.contains(a))
		{
			result = a;
			return true;
		}
		else
		{
			return false;
		}
	}
	float t = (-a.dot(pl.n) - pl.d) / div;
	if (t < 0.0f || t > 1.0f)
	{
		return false;
	}
	result = (b - a) * t + a;
	return true;
}

}	// namespace Blast
}	// namespace Nv

#endif