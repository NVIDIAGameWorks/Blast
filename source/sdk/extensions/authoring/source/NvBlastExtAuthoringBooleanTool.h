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
// Copyright (c) 2016-2020 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTEXTAUTHORINGBOOLEANTOOL_H
#define NVBLASTEXTAUTHORINGBOOLEANTOOL_H

#include "NvBlastExtAuthoringTypes.h"
#include "NvBlastExtAuthoringInternalCommon.h"
#include <vector>
#include "NvBlastTypes.h"


namespace Nv
{
namespace Blast
{

class Mesh;

/**
	Boolean tool config, used to perform different operations: UNION, INTERSECTION, DIFFERENCE
*/
struct BooleanConf
{
	int32_t ca, cb, ci;
	BooleanConf(int32_t a, int32_t b, int32_t c) : ca(a), cb(b), ci(c)
	{
	}
};


namespace BooleanConfigurations
{
	/**
		Creates boolean tool configuration to perform intersection of meshes A and B.
	*/
inline BooleanConf BOOLEAN_INTERSECION()
{
	return BooleanConf(0, 0, 1);
}

/**
	Creates boolean tool configuration to perform union of meshes A and B.
*/
inline BooleanConf BOOLEAN_UNION()
{
	return BooleanConf(1, 1, -1);
}
/**
	Creates boolean tool configuration to perform difference of meshes(A - B).
*/
inline BooleanConf BOOLEAN_DIFFERENCE()
{
	return BooleanConf(1, 0, -1);
}
}

/**
	Structure which holds information about intersection facet with edge.
*/
struct EdgeFacetIntersectionData
{
	int32_t	edId;
	int32_t	intersectionType;
	Vertex	intersectionPoint;
	EdgeFacetIntersectionData(int32_t edId, int32_t intersType, Vertex& inters) : edId(edId), intersectionType(intersType), intersectionPoint(inters)
	{	}
	EdgeFacetIntersectionData(int32_t edId) : edId(edId)
	{	}
	bool operator<(const EdgeFacetIntersectionData& b) const
	{
		return edId < b.edId;
	}
};


class SpatialAccelerator;

/**
	Tool for performing boolean operations on polygonal meshes.
	Tool supports only closed meshes. Performing boolean on meshes with holes can lead to unexpected behavior, e.g. holes in result geometry.
*/
class BooleanEvaluator
{

public:
	BooleanEvaluator();
	~BooleanEvaluator();

	/**
		Perform boolean operation on two polygonal meshes (A and B).
		\param[in] meshA	Mesh A 
		\param[in] meshB	Mesh B
		\param[in] spAccelA Acceleration structure for mesh A
		\param[in] spAccelB Acceleration structure for mesh B
		\param[in] mode		Boolean operation type
	*/
	void	performBoolean(const Mesh* meshA, const Mesh* meshB, SpatialAccelerator* spAccelA, SpatialAccelerator* spAccelB, BooleanConf mode);

	/**
		Perform boolean operation on two polygonal meshes (A and B).
		\param[in] meshA	Mesh A
		\param[in] meshB	Mesh B
		\param[in] mode		Boolean operation type
	*/
	void	performBoolean(const Mesh* meshA, const Mesh* meshB, BooleanConf mode);

	/**
		Perform cutting of mesh with some large box, which represents cutting plane. This method skips part of intersetion computations, so
		should be used ONLY with cutting box, received from getBigBox(...) method from NvBlastExtAuthoringMesh.h. For cutting use only BOOLEAN_INTERSECTION or BOOLEAN_DIFFERENCE mode.
		\param[in] meshA	Mesh A
		\param[in] meshB	Cutting box
		\param[in] spAccelA Acceleration structure for mesh A
		\param[in] spAccelB Acceleration structure for cutting box
		\param[in] mode		Boolean operation type
	*/
	void	performFastCutting(const Mesh* meshA, const Mesh* meshB, SpatialAccelerator* spAccelA, SpatialAccelerator* spAccelB, BooleanConf mode);

	/**
		Perform cutting of mesh with some large box, which represents cutting plane. This method skips part of intersetion computations, so
		should be used ONLY with cutting box, received from getBigBox(...) method from NvBlastExtAuthoringMesh.h. For cutting use only BOOLEAN_INTERSECTION or BOOLEAN_DIFFERENCE mode.
		\param[in] meshA	Mesh A
		\param[in] meshB	Cutting box
		\param[in] mode		Boolean operation type
	*/
	void	performFastCutting(const Mesh* meshA, const Mesh* meshB, BooleanConf mode);

	/**
		Test whether point contained in mesh.
		\param[in] mesh		Mesh geometry
		\param[in] point	Point which should be tested
		\return not 0 if point is inside of mesh
	*/
	int32_t	isPointContainedInMesh(const Mesh* mesh, const NvcVec3& point);
	/**
		Test whether point contained in mesh.
		\param[in] mesh		Mesh geometry
		\param[in] spAccel	Acceleration structure for mesh
		\param[in] point	Point which should be tested
		\return not 0 if point is inside of mesh
	*/
	int32_t	isPointContainedInMesh(const Mesh* mesh, SpatialAccelerator* spAccel, const NvcVec3& point);


	/**
		Generates result polygon mesh after performing boolean operation.
		\return If not nullptr - result mesh geometry.
	*/
	Mesh*	createNewMesh();

	/**
		Reset tool state.
	*/
	void	reset();

private:

	void	buildFaceFaceIntersections(BooleanConf);
	void	buildFastFaceFaceIntersection(BooleanConf);
	void	collectRetainedPartsFromA(BooleanConf mode);
	void	collectRetainedPartsFromB(BooleanConf mode);

	int32_t	addIfNotExist(Vertex& p);
	void	addEdgeIfValid(EdgeWithParent& ed);
private:

	int32_t	vertexMeshStatus03(const NvcVec3& p, const Mesh* mesh);
	int32_t	vertexMeshStatus30(const NvcVec3& p, const Mesh* mesh);

	const Mesh*												mMeshA;
	const Mesh*												mMeshB;

	SpatialAccelerator*										mAcceleratorA;
	SpatialAccelerator*										mAcceleratorB;

	std::vector<EdgeWithParent>								mEdgeAggregate;
	std::vector<Vertex>										mVerticesAggregate;

	std::vector<std::vector<EdgeFacetIntersectionData> >	mEdgeFacetIntersectionData12;
	std::vector<std::vector<EdgeFacetIntersectionData> >	mEdgeFacetIntersectionData21;
};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTAUTHORINGBOOLEANTOOL_H
