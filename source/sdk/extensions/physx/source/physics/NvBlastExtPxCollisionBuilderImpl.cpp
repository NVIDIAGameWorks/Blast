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

#include <NvBlastGlobals.h>
#include "NvBlastExtPxCollisionBuilderImpl.h"
#include "NvBlastExtAuthoringTypes.h"
#include "NvBlastExtPxAsset.h"
#include <PxConvexMesh.h>
#include "PxPhysics.h"
#include "cooking/PxCooking.h"
#include <NvBlastPxSharedHelpers.h>
#include <vector>
#include <set>

using namespace physx;

#define SAFE_ARRAY_NEW(T, x) ((x) > 0) ? reinterpret_cast<T*>(NVBLAST_ALLOC(sizeof(T) * (x))) : nullptr;
#define SAFE_ARRAY_DELETE(x)                                                                                           \
	if (x != nullptr)                                                                                                  \
	{                                                                                                                  \
		NVBLAST_FREE(x);                                                                                               \
		x = nullptr;                                                                                                   \
	}

namespace Nv
{
namespace Blast
{

CollisionHullImpl::~CollisionHullImpl()
{
	SAFE_ARRAY_DELETE(points);
	SAFE_ARRAY_DELETE(indices);
	SAFE_ARRAY_DELETE(polygonData);
}

CollisionHullImpl::CollisionHullImpl(const CollisionHull& hullToCopy)
{
	pointsCount      = hullToCopy.pointsCount;
	indicesCount     = hullToCopy.indicesCount;
	polygonDataCount = hullToCopy.polygonDataCount;

	points      = SAFE_ARRAY_NEW(NvcVec3, pointsCount);
	indices     = SAFE_ARRAY_NEW(uint32_t, indicesCount);
	polygonData = SAFE_ARRAY_NEW(HullPolygon, polygonDataCount);
	memcpy(points, hullToCopy.points, sizeof(points[0]) * pointsCount);
	memcpy(indices, hullToCopy.indices, sizeof(indices[0]) * indicesCount);
	memcpy(polygonData, hullToCopy.polygonData, sizeof(polygonData[0]) * polygonDataCount);
}

CollisionHull* ExtPxCollisionBuilderImpl::buildCollisionGeometry(uint32_t verticesCount, const NvcVec3* vData)
{
	CollisionHull* output = new CollisionHullImpl();
	std::vector<physx::PxVec3> vertexData(verticesCount);
	memcpy(vertexData.data(), vData, sizeof(physx::PxVec3) * verticesCount);

	PxConvexMeshDesc convexMeshDescr;
	PxConvexMesh* resultConvexMesh;
	PxBounds3 bounds;
	// Scale chunk to unit cube size, to avoid numerical errors
	bounds.setEmpty();
	for (uint32_t i = 0; i < vertexData.size(); ++i)
	{
		bounds.include(vertexData[i]);
	}
	PxVec3 bbCenter = bounds.getCenter();
	float scale = PxMax(PxAbs(bounds.getExtents(0)), PxMax(PxAbs(bounds.getExtents(1)), PxAbs(bounds.getExtents(2))));
	for (uint32_t i = 0; i < vertexData.size(); ++i)
	{
		vertexData[i] = vertexData[i] - bbCenter;
		vertexData[i] *= (1.0f / scale);
	}
	bounds.setEmpty();
	for (uint32_t i = 0; i < vertexData.size(); ++i)
	{
		bounds.include(vertexData[i]);
	}
	convexMeshDescr.points.data   = vertexData.data();
	convexMeshDescr.points.stride = sizeof(PxVec3);
	convexMeshDescr.points.count  = (uint32_t)vertexData.size();
	convexMeshDescr.flags         = PxConvexFlag::eCOMPUTE_CONVEX;
	resultConvexMesh              = mCooking->createConvexMesh(convexMeshDescr, *mInsertionCallback);
	if (!resultConvexMesh)
	{
		vertexData.clear();
		vertexData.push_back(bounds.minimum);
		vertexData.push_back(PxVec3(bounds.minimum.x, bounds.maximum.y, bounds.minimum.z));
		vertexData.push_back(PxVec3(bounds.maximum.x, bounds.maximum.y, bounds.minimum.z));
		vertexData.push_back(PxVec3(bounds.maximum.x, bounds.minimum.y, bounds.minimum.z));
		vertexData.push_back(PxVec3(bounds.minimum.x, bounds.minimum.y, bounds.maximum.z));
		vertexData.push_back(PxVec3(bounds.minimum.x, bounds.maximum.y, bounds.maximum.z));
		vertexData.push_back(PxVec3(bounds.maximum.x, bounds.maximum.y, bounds.maximum.z));
		vertexData.push_back(PxVec3(bounds.maximum.x, bounds.minimum.y, bounds.maximum.z));
		convexMeshDescr.points.data  = vertexData.data();
		convexMeshDescr.points.count = (uint32_t)vertexData.size();
		resultConvexMesh             = mCooking->createConvexMesh(convexMeshDescr, *mInsertionCallback);
	}
	output->polygonDataCount = resultConvexMesh->getNbPolygons();
	if (output->polygonDataCount)
		output->polygonData = SAFE_ARRAY_NEW(HullPolygon, output->polygonDataCount);
	output->pointsCount  = resultConvexMesh->getNbVertices();
	output->points       = SAFE_ARRAY_NEW(NvcVec3, output->pointsCount);
	int32_t indicesCount = 0;
	PxHullPolygon hPoly;
	for (uint32_t i = 0; i < resultConvexMesh->getNbPolygons(); ++i)
	{
		HullPolygon& pd = output->polygonData[i];
		resultConvexMesh->getPolygonData(i, hPoly);
		pd.indexBase = hPoly.mIndexBase;
		pd.vertexCount   = hPoly.mNbVerts;
		pd.plane[0]  = hPoly.mPlane[0];
		pd.plane[1]  = hPoly.mPlane[1];
		pd.plane[2]  = hPoly.mPlane[2];
		pd.plane[3]  = hPoly.mPlane[3];

		pd.plane[0] /= scale;
		pd.plane[1] /= scale;
		pd.plane[2] /= scale;
		pd.plane[3] -= (pd.plane[0] * bbCenter.x + pd.plane[1] * bbCenter.y + pd.plane[2] * bbCenter.z);
		float length = sqrt(pd.plane[0] * pd.plane[0] + pd.plane[1] * pd.plane[1] + pd.plane[2] * pd.plane[2]);
		pd.plane[0] /= length;
		pd.plane[1] /= length;
		pd.plane[2] /= length;
		pd.plane[3] /= length;
		indicesCount = PxMax(indicesCount, pd.indexBase + pd.vertexCount);
	}
	output->indicesCount = indicesCount;
	output->indices      = SAFE_ARRAY_NEW(uint32_t, indicesCount);
	for (uint32_t i = 0; i < resultConvexMesh->getNbVertices(); ++i)
	{
		PxVec3 p          = resultConvexMesh->getVertices()[i] * scale + bbCenter;
		output->points[i] = fromPxShared(p);
	}
	for (int32_t i = 0; i < indicesCount; ++i)
	{
		output->indices[i] = resultConvexMesh->getIndexBuffer()[i];
	}
	resultConvexMesh->release();
	return output;
}

void ExtPxCollisionBuilderImpl::releaseCollisionHull(Nv::Blast::CollisionHull* ch) const
{
	if (ch)
	{
		SAFE_ARRAY_DELETE(ch->indices);
		SAFE_ARRAY_DELETE(ch->points);
		SAFE_ARRAY_DELETE(ch->polygonData);
		delete ch;
	}
}

physx::PxConvexMesh* ExtPxCollisionBuilderImpl::buildConvexMesh(const CollisionHull& hull)
{
	/* PxCooking::createConvexMesh expects PxHullPolygon input, which matches HullPolygon */
	static_assert(sizeof(PxHullPolygon) == sizeof(HullPolygon), "HullPolygon size mismatch");
	static_assert(offsetof(PxHullPolygon, mPlane) == offsetof(HullPolygon, plane), "HullPolygon layout mismatch");
	static_assert(offsetof(PxHullPolygon, mNbVerts) == offsetof(HullPolygon, vertexCount), "HullPolygon layout mismatch");
	static_assert(offsetof(PxHullPolygon, mIndexBase) == offsetof(HullPolygon, indexBase),
	              "HullPolygon layout mismatch");

	PxConvexMeshDesc convexMeshDescr;
	convexMeshDescr.indices.data   = hull.indices;
	convexMeshDescr.indices.count  = (uint32_t)hull.indicesCount;
	convexMeshDescr.indices.stride = sizeof(uint32_t);

	convexMeshDescr.points.data   = hull.points;
	convexMeshDescr.points.count  = (uint32_t)hull.pointsCount;
	convexMeshDescr.points.stride = sizeof(PxVec3);

	convexMeshDescr.polygons.data   = hull.polygonData;
	convexMeshDescr.polygons.count  = (uint32_t)hull.polygonDataCount;
	convexMeshDescr.polygons.stride = sizeof(PxHullPolygon);

	return mCooking->createConvexMesh(convexMeshDescr, *mInsertionCallback);
}

void ExtPxCollisionBuilderImpl::buildPhysicsChunks(uint32_t chunkCount, uint32_t* hullOffsets,
                                              CollisionHull** hulls, ExtPxChunk* physicsChunks,
                                              ExtPxSubchunk* physicsSubchunks)
{
	//Nv::Blast::AuthoringResult& result = *ar;
	//uint32_t chunkCount                = (uint32_t)result.chunkCount;
	//uint32_t* hullOffsets              = result.collisionHullOffset;
	//CollisionHull** hulls               = result.collisionHull;
	for (uint32_t i = 0; i < chunkCount; ++i)
	{
		int32_t beg = hullOffsets[i];
		int32_t end = hullOffsets[i + 1];
		for (int32_t subhull = beg; subhull < end; ++subhull)
		{
			physicsSubchunks[subhull].transform = physx::PxTransform(physx::PxIdentity);
			physicsSubchunks[subhull].geometry  = physx::PxConvexMeshGeometry(
                reinterpret_cast<physx::PxConvexMesh*>(buildConvexMesh(*hulls[subhull])));
		}
		physicsChunks[i].isStatic           = false;
		physicsChunks[i].subchunkCount      = static_cast<uint32_t>(end - beg);
		physicsChunks[i].firstSubchunkIndex = beg;
	}
}

void ExtPxCollisionBuilderImpl::release()
{
	NVBLAST_DELETE(this, ExtPxCollisionBuilderImpl);
}

}  // namespace Blast
}  // namespace Nv
