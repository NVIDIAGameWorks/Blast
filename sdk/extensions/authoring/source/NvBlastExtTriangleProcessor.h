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


#ifndef NVBLASTEXTTRIANGLEPROCESSOR_H
#define NVBLASTEXTTRIANGLEPROCESSOR_H

#include <PxPhysicsAPI.h>
#include <vector>
#include <algorithm>

using namespace physx;


namespace Nv
{
namespace Blast
{
	
/**
	Triangle processor internal triangle representation. Contains only vertex positions.
*/
struct TrPrcTriangle
{
	PxVec3 points[3];
	TrPrcTriangle(PxVec3 a = PxVec3(0.0f), PxVec3 b = PxVec3(0.0f), PxVec3 c = PxVec3(0.0f))
	{
		points[0] = a;
		points[1] = b;
		points[2] = c;
	}

	TrPrcTriangle& operator=(const TrPrcTriangle& b)
	{
		points[0] = b.points[0];
		points[1] = b.points[1];
		points[2] = b.points[2];
		return *this;
	}

	TrPrcTriangle(const TrPrcTriangle& b)
	{
		points[0] = b.points[0];
		points[1] = b.points[1];
		points[2] = b.points[2];
	}
	PxVec3 getNormal() const
	{
		return (points[1] - points[0]).cross(points[2] - points[0]);
	}
};

/**
	Triangle processor internal 2D triangle representation. Contains only vertex positions.
*/
struct TrPrcTriangle2d
{
	PxVec2 points[3];
	TrPrcTriangle2d(PxVec2 a = PxVec2(0.0f), PxVec2 b = PxVec2(0.0f), PxVec2 c = PxVec2(0.0f))
	{
		points[0] = a;
		points[1] = b;
		points[2] = c;
	}

	TrPrcTriangle2d operator=(const TrPrcTriangle2d& b)
	{
		points[0] = b.points[0];
		points[1] = b.points[1];
		points[2] = b.points[2];
		return *this;
	}

	TrPrcTriangle2d(const TrPrcTriangle2d& b)
	{
		points[0] = b.points[0];
		points[1] = b.points[1];
		points[2] = b.points[2];
	}
};

class TriangleProcessor
{
public:


	TriangleProcessor()
	{};
	~TriangleProcessor()
	{
	}
		

	/**
		Build intersection between two triangles
		\param[in] a			First triangle (A)
		\param[in] aProjected	Projected triangle A
		\param[in] b			Second triangle (B)
		\param[in] centroid		Centroid of first triangle (A)
		\param[out] intersectionBuffer Result intersection polygon
		\param[in] normal		Normal vector to triangle (Common for both A and B).
		\return 1 - if if intersection is found.
	*/	
	uint32_t	getTriangleIntersection(TrPrcTriangle& a, TrPrcTriangle2d& aProjected, TrPrcTriangle &b, PxVec3& centroid, std::vector<PxVec3>& intersectionBuffer, PxVec3 normal);

	/**
		Test whether BB of triangles intersect.
		\param[in] a			First triangle (A)
		\param[in] b			Second triangle (B)
		\return true - if intersect
	*/
	bool		triangleBoundingBoxIntersection(TrPrcTriangle2d& a, TrPrcTriangle2d& b);
		

	/**
		Test whether point is inside of triangle.
		\param[in] point		Point coordinates in 2d space.
		\param[in] triangle		Triangle in 2d space.
		\return 1 - if inside, 2 if on edge, 0 if neither inside nor edge.
	*/
	uint32_t	isPointInside(const PxVec2& point, const TrPrcTriangle2d& triangle);

	/**
		Segment intersection point
		\param[in] s1 Segment-1 start point
		\param[in] e1 Segment-1 end point
		\param[in] s2 Segment-2 start point
		\param[in] e2 Segment-2 end point
		\param[out] t1 Intersection point parameter relatively to Segment-1, lies in [0.0, 1.0] range.
		\return 0 if there is no intersections, 1 - if intersection is found.
	*/
	uint32_t	getSegmentIntersection(const PxVec2& s1, const PxVec2& e1, const PxVec2& s2, const PxVec2& e2, PxF32& t1);

	/**
		Sort vertices of polygon in CCW-order
	*/
	void		sortToCCW(std::vector<PxVec3>& points, PxVec3& normal);
	
	/**
		Builds convex polygon for given set of points. Points should be coplanar.
		\param[in] points Input array of points
		\param[out] convexHull Output polygon
		\param[in] normal Normal vector to polygon.	
	*/
	void		buildConvexHull(std::vector<PxVec3>& points, std::vector<PxVec3>& convexHull, const PxVec3& normal);
};

} // namespace Blast
} // namespace Nv


#endif // NVBLASTEXTTRIANGLEPROCESSOR_H
