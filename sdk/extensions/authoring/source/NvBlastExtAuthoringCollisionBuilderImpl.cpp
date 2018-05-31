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

#include <NvBlastGlobals.h>
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

#include <VHACD.h>

using namespace physx;

#define SAFE_ARRAY_NEW(T, x) ((x) > 0) ? reinterpret_cast<T*>(NVBLAST_ALLOC(sizeof(T) * (x))) : nullptr;
#define SAFE_ARRAY_DELETE(x) if (x != nullptr) {NVBLAST_FREE(x); x = nullptr;}

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
	pointsCount = hullToCopy.pointsCount;
	indicesCount = hullToCopy.indicesCount;
	polygonDataCount = hullToCopy.polygonDataCount;

	points = SAFE_ARRAY_NEW(physx::PxVec3, pointsCount);
	indices = SAFE_ARRAY_NEW(uint32_t, indicesCount);
	polygonData = SAFE_ARRAY_NEW(CollisionHull::HullPolygon, polygonDataCount);
	memcpy(points, hullToCopy.points, sizeof(points[0]) * pointsCount);
	memcpy(indices, hullToCopy.indices, sizeof(indices[0]) * indicesCount);
	memcpy(polygonData, hullToCopy.polygonData, sizeof(polygonData[0]) * polygonDataCount);
}

void CollisionHullImpl::release()
{
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
		//I think the material ID is unused for collision meshes so harcoding MATERIAL_INTERIOR is ok
		Mesh* cuttingMesh = getCuttingBox(PxVec3(0, 0, 0), PxVec3(0, 0, 1), 40, 0, MATERIAL_INTERIOR);
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
	PxConvexMesh* convexMesh = buildConvexMesh(*hull);
	hull->release();
	return convexMesh;
}

PxConvexMesh* ConvexMeshBuilderImpl::buildConvexMesh(const CollisionHull& hull)
{	
	/* PxCooking::createConvexMesh expects PxHullPolygon input, which matches CollisionHull::HullPolygon */
	static_assert(sizeof(PxHullPolygon) == sizeof(CollisionHull::HullPolygon), "CollisionHull::HullPolygon size mismatch");
	static_assert(offsetof(PxHullPolygon, mPlane) == offsetof(CollisionHull::HullPolygon, mPlane), "CollisionHull::HullPolygon layout mismatch");
	static_assert(offsetof(PxHullPolygon, mNbVerts) == offsetof(CollisionHull::HullPolygon, mNbVerts), "CollisionHull::HullPolygon layout mismatch");
	static_assert(offsetof(PxHullPolygon, mIndexBase) == offsetof(CollisionHull::HullPolygon, mIndexBase), "CollisionHull::HullPolygon layout mismatch");

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

int32_t	ConvexMeshBuilderImpl::buildMeshConvexDecomposition(const Triangle* mesh, uint32_t triangleCount, const CollisionParams& iparams, CollisionHull**& convexes)
{
	std::vector<float> coords(triangleCount * 9);
	std::vector<uint32_t> indices(triangleCount * 3);

	uint32_t indx = 0;
	uint32_t indxCoord = 0;

	PxBounds3 chunkBound = PxBounds3::empty();
	for (uint32_t i = 0; i < triangleCount; ++i)
	{
		for (auto& t : { mesh[i].a.p , mesh[i].b.p , mesh[i].c.p })
		{

			chunkBound.include(t);
			coords[indxCoord] = t.x;
			coords[indxCoord + 1] = t.y;
			coords[indxCoord + 2] = t.z;
			indxCoord += 3;
		}
		indices[indx] = indx;
		indices[indx + 1] = indx + 1;
		indices[indx + 2] = indx + 2;
		indx += 3;
	}

	PxVec3 rsc = chunkBound.getDimensions();

	for (uint32_t i = 0; i < coords.size(); i += 3)
	{
		coords[i] = (coords[i] - chunkBound.minimum.x) / rsc.x;
		coords[i + 1] = (coords[i + 1] - chunkBound.minimum.y) / rsc.y;
		coords[i + 2] = (coords[i + 2] - chunkBound.minimum.z) / rsc.z;
	}
	
	VHACD::IVHACD* decomposer = VHACD::CreateVHACD();

	VHACD::IVHACD::Parameters vhacdParam;
	vhacdParam.m_maxConvexHulls = iparams.maximumNumberOfHulls;
	vhacdParam.m_resolution = iparams.voxelGridResolution;
	vhacdParam.m_concavity = iparams.concavity;
	vhacdParam.m_oclAcceleration = false;
	//TODO vhacdParam.m_callback
	vhacdParam.m_minVolumePerCH = 0.003f; // 1.f / (3 * vhacdParam.m_resolution ^ (1 / 3));

	decomposer->Compute(coords.data(), triangleCount * 3, indices.data(), triangleCount, vhacdParam);

	const uint32_t nConvexHulls = decomposer->GetNConvexHulls();
	convexes = SAFE_ARRAY_NEW(CollisionHull*, nConvexHulls);

	for (uint32_t i = 0; i < nConvexHulls; ++i)
	{
		VHACD::IVHACD::ConvexHull hl;
		decomposer->GetConvexHull(i, hl);
		std::vector<PxVec3> vertices;
		for (uint32_t v = 0; v < hl.m_nPoints; ++v)
		{
			vertices.push_back(PxVec3(hl.m_points[v * 3], hl.m_points[v * 3 + 1], hl.m_points[v * 3 + 2]));
			vertices.back().x = vertices.back().x * rsc.x + chunkBound.minimum.x;
			vertices.back().y = vertices.back().y * rsc.y + chunkBound.minimum.y;
			vertices.back().z = vertices.back().z * rsc.z + chunkBound.minimum.z;

		}
		convexes[i] = buildCollisionGeometry(vertices.size(), vertices.data());
	}
	//VHACD::~VHACD called from release does nothign and does not call Clean()
	decomposer->Clean();
	decomposer->Release();

	return nConvexHulls;
}



} // namespace Blast
} // namespace Nv
