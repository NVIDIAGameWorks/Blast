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
// Copyright (c) 2016-2017 NVIDIA Corporation. All rights reserved.


#include "NvBlastExtAuthoringCollisionBuilderImpl.h"
#include <PxConvexMesh.h>
#include <PxVec3.h>
#include <PxBounds3.h>
#include "PxPhysics.h"
#include "cooking/PxCooking.h"
#include  <NvBlastExtApexSharedParts.h>
#include <NvBlastExtAuthoringInternalCommon.h>

#include <NvBlastExtAuthoringBooleanTool.h>
#include <NvBlastExtAuthoringMeshImpl.h>

using namespace physx;

#define SAFE_ARRAY_NEW(T, x) ((x) > 0) ? new T[x] : nullptr;
#define SAFE_ARRAY_DELETE(x) if (x != nullptr) {delete[] x; x = nullptr;}

namespace Nv
{
namespace Blast
{

void CollisionHullImpl::release()
{
	SAFE_ARRAY_DELETE(points);
	SAFE_ARRAY_DELETE(indices);
	SAFE_ARRAY_DELETE(polygonData);
	delete this;
}

CollisionHull* ConvexMeshBuilderImpl::buildCollisionGeometry(uint32_t verticesCount, const physx::PxVec3* vData)
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
	convexMeshDescr.points.data = vertexData.data();
	convexMeshDescr.points.stride = sizeof(PxVec3);
	convexMeshDescr.points.count = (uint32_t)vertexData.size();
	convexMeshDescr.flags = PxConvexFlag::eCOMPUTE_CONVEX;
	resultConvexMesh = mCooking->createConvexMesh(convexMeshDescr, *mInsertionCallback);
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
		convexMeshDescr.points.data = vertexData.data();
		convexMeshDescr.points.count = (uint32_t)vertexData.size();
		resultConvexMesh = mCooking->createConvexMesh(convexMeshDescr, *mInsertionCallback);
	}
	output->polygonDataCount = resultConvexMesh->getNbPolygons();
	if (output->polygonDataCount)
	output->polygonData = SAFE_ARRAY_NEW(CollisionHull::HullPolygon, output->polygonDataCount);
	output->pointsCount = resultConvexMesh->getNbVertices();
	output->points = SAFE_ARRAY_NEW(PxVec3, output->pointsCount);
	int32_t indicesCount = 0;
	PxHullPolygon hPoly;
	for (uint32_t i = 0; i < resultConvexMesh->getNbPolygons(); ++i)
	{
		CollisionHull::HullPolygon& pd = output->polygonData[i];
		resultConvexMesh->getPolygonData(i, hPoly);
		pd.mIndexBase = hPoly.mIndexBase;
		pd.mNbVerts = hPoly.mNbVerts;
		pd.mPlane[0] = hPoly.mPlane[0];
		pd.mPlane[1] = hPoly.mPlane[1];
		pd.mPlane[2] = hPoly.mPlane[2];
		pd.mPlane[3] = hPoly.mPlane[3];

		pd.mPlane[0] /= scale;
		pd.mPlane[1] /= scale;
		pd.mPlane[2] /= scale;
		pd.mPlane[3] -= (pd.mPlane[0] * bbCenter.x + pd.mPlane[1] * bbCenter.y + pd.mPlane[2] * bbCenter.z);
		float length = sqrt(pd.mPlane[0] * pd.mPlane[0] + pd.mPlane[1] * pd.mPlane[1] + pd.mPlane[2] * pd.mPlane[2]);
		pd.mPlane[0] /= length;
		pd.mPlane[1] /= length;
		pd.mPlane[2] /= length;
		pd.mPlane[3] /= length;
		indicesCount = PxMax(indicesCount, pd.mIndexBase + pd.mNbVerts);
	}
	output->indicesCount = indicesCount;
	output->indices = SAFE_ARRAY_NEW(uint32_t, indicesCount);
	for (uint32_t i = 0; i < resultConvexMesh->getNbVertices(); ++i)
	{
		PxVec3 p = resultConvexMesh->getVertices()[i] * scale + bbCenter;
		output->points[i] = p;
	}
	for (int32_t i = 0; i < indicesCount; ++i)
	{
		output->indices[i] = resultConvexMesh->getIndexBuffer()[i];
	}
	resultConvexMesh->release();
	return output;
}

void ConvexMeshBuilderImpl::trimCollisionGeometry(uint32_t chunksCount, CollisionHull** in, const uint32_t* chunkDepth)
{
	std::vector<std::vector<PxPlane> > chunkMidplanes(chunksCount);
	std::vector<PxVec3> centers(chunksCount);
	std::vector<PxBounds3> hullsBounds(chunksCount);
	for (uint32_t i = 0; i < chunksCount; ++i)
	{
		hullsBounds[i].setEmpty();
		centers[i] = PxVec3(0, 0, 0);
		for (uint32_t p = 0; p < in[i]->pointsCount; ++p)
		{
			centers[i] += in[i]->points[p];
			hullsBounds[i].include(in[i]->points[p]);
		}
		centers[i] = hullsBounds[i].getCenter();
	}

	Separation params;
	for (uint32_t hull = 0; hull < chunksCount; ++hull)
	{
		for (uint32_t hull2 = hull + 1; hull2 < chunksCount; ++hull2)
		{
			if (chunkDepth[hull] != chunkDepth[hull2])
			{
				continue;
			}
			if (importerHullsInProximityApexFree(in[hull]->pointsCount, in[hull]->points, hullsBounds[hull], PxTransform(PxIdentity), PxVec3(1, 1, 1),
				in[hull2]->pointsCount, in[hull2]->points, hullsBounds[hull2], PxTransform(PxIdentity), PxVec3(1, 1, 1), 0.0, &params) == false)
			{
				continue;
			}
			PxVec3 c1 = centers[hull];
			PxVec3 c2 = centers[hull2];
			float d = FLT_MAX;
			PxVec3 n1;
			PxVec3 n2;
			for (uint32_t p = 0; p < in[hull]->pointsCount; ++p)
			{
				float ld = (in[hull]->points[p] - c2).magnitude();
				if (ld < d)
				{
					n1 = in[hull]->points[p];
					d = ld;
				}
			}
			d = FLT_MAX;
			for (uint32_t p = 0; p < in[hull2]->pointsCount; ++p)
			{
				float ld = (in[hull2]->points[p] - c1).magnitude();
				if (ld < d)
				{
					n2 = in[hull2]->points[p];
					d = ld;
				}
			}

			PxVec3 dir = c2 - c1;

			PxPlane pl = PxPlane((n1 + n2) * 0.5, dir.getNormalized());
			chunkMidplanes[hull].push_back(pl);
			PxPlane pl2 = PxPlane((n1 + n2) * 0.5, -dir.getNormalized());
			chunkMidplanes[hull2].push_back(pl2);		
		}
	}
	std::vector<PxVec3> hPoints;
	for (uint32_t i = 0; i < chunksCount; ++i)
	{
		std::vector<Facet> facets;
		std::vector<Vertex> vertices;
		std::vector<Edge> edges;
		for (uint32_t fc = 0; fc < in[i]->polygonDataCount; ++fc)
		{
			Facet nFc;
			nFc.firstEdgeNumber = edges.size();
			auto& pd = in[i]->polygonData[fc];
			uint32_t n = pd.mNbVerts;
			for (uint32_t ed = 0; ed < n; ++ed)
			{
				uint32_t vr1 = in[i]->indices[(ed) + pd.mIndexBase];
				uint32_t vr2 = in[i]->indices[(ed + 1) % n + pd.mIndexBase];
				edges.push_back(Edge(vr1, vr2));
			}
			nFc.edgesCount = n;
			facets.push_back(nFc);
		}
		vertices.resize(in[i]->pointsCount);
		for (uint32_t vr = 0; vr < in[i]->pointsCount; ++vr)
		{
			vertices[vr].p = in[i]->points[vr];
		}
		Mesh* hullMesh = new MeshImpl(vertices.data(), edges.data(), facets.data(), vertices.size(), edges.size(), facets.size());
		BooleanEvaluator evl;
		Mesh* cuttingMesh = getCuttingBox(PxVec3(0, 0, 0), PxVec3(0, 0, 1), 40, 0);
		for (uint32_t p = 0; p < chunkMidplanes[i].size(); ++p)
		{
			PxPlane& pl = chunkMidplanes[i][p];
			setCuttingBox(pl.pointInPlane(), pl.n.getNormalized(), cuttingMesh, 60, 0);
			evl.performFastCutting(hullMesh, cuttingMesh, BooleanConfigurations::BOOLEAN_DIFFERENCE());
			Mesh* result = evl.createNewMesh();
			if (result == nullptr)
			{
				break;
			}
			delete hullMesh;
			hullMesh = result;
		}
		delete cuttingMesh;
		if (hullMesh == nullptr)
		{
			continue;
		}
		hPoints.clear();
		hPoints.resize(hullMesh->getVerticesCount());
		for (uint32_t v = 0; v < hullMesh->getVerticesCount(); ++v)
		{
			hPoints[v] = hullMesh->getVertices()[v].p;
		}
		delete hullMesh;
		if (in[i] != nullptr)
		{
			in[i]->release();
		}
		in[i] = buildCollisionGeometry(hPoints.size(), hPoints.data());
	}
}


PxConvexMesh* ConvexMeshBuilderImpl::buildConvexMesh(uint32_t verticesCount, const physx::PxVec3* vertexData)
{
	CollisionHull* hull = buildCollisionGeometry(verticesCount, vertexData);

	PxConvexMeshDesc convexMeshDescr;
	convexMeshDescr.indices.data = hull->indices;
	convexMeshDescr.indices.count = (uint32_t)hull->indicesCount;
	convexMeshDescr.indices.stride = sizeof(uint32_t);

	convexMeshDescr.points.data = hull->points;
	convexMeshDescr.points.count = (uint32_t)hull->pointsCount;
	convexMeshDescr.points.stride = sizeof(PxVec3);

	convexMeshDescr.polygons.data = hull->polygonData;
	convexMeshDescr.polygons.count = (uint32_t)hull->polygonDataCount;
	convexMeshDescr.polygons.stride = sizeof(PxHullPolygon);

	PxConvexMesh* convexMesh = mCooking->createConvexMesh(convexMeshDescr, *mInsertionCallback);
	hull->release();
	return convexMesh;
}

PxConvexMesh* ConvexMeshBuilderImpl::buildConvexMesh(const CollisionHull& hull)
{	
	PxConvexMeshDesc convexMeshDescr;
	convexMeshDescr.indices.data = hull.indices;
	convexMeshDescr.indices.count = (uint32_t)hull.indicesCount;
	convexMeshDescr.indices.stride = sizeof(uint32_t);

	convexMeshDescr.points.data = hull.points;
	convexMeshDescr.points.count = (uint32_t)hull.pointsCount;
	convexMeshDescr.points.stride = sizeof(PxVec3);

	convexMeshDescr.polygons.data = hull.polygonData;
	convexMeshDescr.polygons.count = (uint32_t)hull.polygonDataCount;
	convexMeshDescr.polygons.stride = sizeof(PxHullPolygon);

	PxConvexMesh* convexMesh = mCooking->createConvexMesh(convexMeshDescr, *mInsertionCallback);
	return convexMesh;
}

void ConvexMeshBuilderImpl::release()
{
	delete this;
}

} // namespace Blast
} // namespace Nv
