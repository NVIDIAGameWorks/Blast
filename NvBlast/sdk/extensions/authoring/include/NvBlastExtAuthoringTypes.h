/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTAUTHORINGTYPES_H
#define NVBLASTAUTHORINGTYPES_H

#include <PxVec3.h>
#include <PxVec2.h>
#include <PxBounds3.h>
#include <algorithm>
#include "NvBlastTypes.h"

#define NOT_VALID_VERTEX INT32_MAX

namespace Nv
{
namespace Blast
{

/**
	Edge representation
*/
struct Edge
{
	uint32_t s, e;
	Edge() : s(NOT_VALID_VERTEX), e(NOT_VALID_VERTEX){}
	Edge(int s, int e) : s(s), e(e) {}
	bool operator<(const Edge& b) const
	{
		if (s == b.s)
			return e < b.e;
		else
			return s < b.s;
	}
};

/**
	Mesh vertex representation
*/
struct Vertex
{
	physx::PxVec3 p; // Position
	physx::PxVec3 n; // Normal
	physx::PxVec2 uv[1]; // UV-coordinates array, currently supported only one UV coordinate.
};

/**
	Mesh triangle representation
*/
struct Triangle
{
	Triangle() {};
	Triangle(Vertex a, Vertex b, Vertex c) : a(a), b(b), c(c) {};
	Vertex a, b, c;
	int32_t userInfo;
	physx::PxVec3 getNormal()
	{
		return ((b.p - a.p).cross(c.p - a.p));
	}
};


/**
	Index based triangle
*/
struct TriangleIndexed
{
	TriangleIndexed() {};
	TriangleIndexed(uint32_t a, uint32_t b, uint32_t c) : ea(a), eb(b), ec(c) {};

	uint32_t getOpposite(uint32_t a, uint32_t b)
	{
		if (ea != a && ea != b)
			return ea;
		if (eb != a && eb != b)
			return eb;
		if (ec != a && ec != b)
			return ec;
		return NOT_VALID_VERTEX;
	}

	bool isContainEdge(uint32_t a, uint32_t b)
	{
		return (a == ea || a == eb || a == ec) && (b == ea || b == eb || b == ec);
	}

	uint32_t ea, eb, ec;
	int32_t userInfo;
};

/**
	Mesh facet representation
*/
struct Facet
{
	int32_t		firstEdgeNumber;
	uint32_t	edgesCount;
	int32_t		userData;
	Facet(int32_t fEdge = 0, uint32_t eCount = 0, int32_t userData = 0) : firstEdgeNumber(fEdge), edgesCount(eCount), userData(userData) {}
};

/**
Abstract base class for user-defined random value generator.
*/
class RandomGeneratorBase
{
public:
	// Generates uniformly distributed value in [0, 1] range. 
	virtual float	getRandomValue() = 0;
	// Seeds random value generator
	virtual void	seed(int32_t seed) = 0;
	virtual ~RandomGeneratorBase() {};
};



} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTAUTHORINGTYPES_H
