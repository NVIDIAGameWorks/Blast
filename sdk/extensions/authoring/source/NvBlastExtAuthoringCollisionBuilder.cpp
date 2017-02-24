/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastExtAuthoringCollisionBuilder.h"
#include <PxConvexMesh.h>
#include <PxVec3.h>
#include <PxBounds3.h>
#include "PxPhysics.h"
#include "cooking/PxCooking.h"
#include  <NvBlastExtApexSharedParts.h>
#include <NvBlastExtAuthoringInternalCommon.h>

#include <NvBlastExtAuthoringBooleanTool.h>
#include <NvBlastExtAuthoringMesh.h>

using namespace physx;

namespace Nv
{
namespace Blast
{

void ConvexMeshBuilder::buildCollisionGeometry(const std::vector<PxVec3>& vData, CollisionHull& output)
{
	std::vector<physx::PxVec3> vertexData = vData;

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
	output.polygonData.resize(resultConvexMesh->getNbPolygons());
	output.points.resize(resultConvexMesh->getNbVertices());
	int32_t indicesCount = 0;
	PxHullPolygon hPoly;
	for (uint32_t i = 0; i < resultConvexMesh->getNbPolygons(); ++i)
	{
		CollisionHull::HullPolygon& pd = output.polygonData[i];
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
	output.indices.resize(indicesCount);
	for (uint32_t i = 0; i < resultConvexMesh->getNbVertices(); ++i)
	{
		PxVec3 p = resultConvexMesh->getVertices()[i] * scale + bbCenter;
		output.points[i] = p;
	}
	for (int32_t i = 0; i < indicesCount; ++i)
	{
		output.indices[i] = resultConvexMesh->getIndexBuffer()[i];
	}
	resultConvexMesh->release();
}

void ConvexMeshBuilder::trimCollisionGeometry(std::vector<CollisionHull>& in, const std::vector<uint32_t>& chunkDepth)
{
	std::vector<std::vector<PxPlane> > chunkMidplanes(in.size());
	std::vector<PxVec3> centers(in.size());
	std::vector<PxBounds3> hullsBounds(in.size());
	for (uint32_t i = 0; i < in.size(); ++i)
	{
		hullsBounds[i].setEmpty();
		centers[i] = PxVec3(0, 0, 0);
		for (uint32_t p = 0; p < in[i].points.size(); ++p)
		{
			centers[i] += in[i].points[p];
			hullsBounds[i].include(in[i].points[p]);
		}
		centers[i] = hullsBounds[i].getCenter();
	}

	Separation params;
	for (uint32_t hull = 0; hull < in.size(); ++hull)
	{
		for (uint32_t hull2 = hull + 1; hull2 < in.size(); ++hull2)
		{
			if (chunkDepth[hull] != chunkDepth[hull2])
			{
				continue;
			}
			if (importerHullsInProximityApexFree(in[hull].points, hullsBounds[hull], PxTransform(PxIdentity), PxVec3(1, 1, 1),
				in[hull2].points, hullsBounds[hull2], PxTransform(PxIdentity), PxVec3(1, 1, 1), 0.0, &params) == false)
			{
				continue;
			}
			PxVec3 c1 = centers[hull];
			PxVec3 c2 = centers[hull2];
			float d = FLT_MAX;
			PxVec3 n1;
			PxVec3 n2;
			for (uint32_t p = 0; p < in[hull].points.size(); ++p)
			{
				float ld = (in[hull].points[p] - c2).magnitude();
				if (ld < d)
				{
					n1 = in[hull].points[p];
					d = ld;
				}
			}
			d = FLT_MAX;
			for (uint32_t p = 0; p < in[hull2].points.size(); ++p)
			{
				float ld = (in[hull2].points[p] - c1).magnitude();
				if (ld < d)
				{
					n2 = in[hull2].points[p];
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
	for (uint32_t i = 0; i < in.size(); ++i)
	{
		std::vector<Facet> facets;
		std::vector<Vertex> vertices;
		std::vector<Edge> edges;
		for (uint32_t fc = 0; fc < in[i].polygonData.size(); ++fc)
		{
			Facet nFc;
			nFc.firstEdgeNumber = edges.size();
			uint32_t n = in[i].polygonData[fc].mNbVerts;
			for (uint32_t ed = 0; ed < n; ++ed)
			{
				uint32_t vr1 = in[i].indices[(ed) + in[i].polygonData[fc].mIndexBase];
				uint32_t vr2 = in[i].indices[(ed + 1) % n + in[i].polygonData[fc].mIndexBase];
				edges.push_back(Edge(vr1, vr2));
			}
			nFc.edgesCount = n;
			facets.push_back(nFc);
		}
		vertices.resize(in[i].points.size());
		for (uint32_t vr = 0; vr < in[i].points.size(); ++vr)
		{
			vertices[vr].p = in[i].points[vr];
		}
		Mesh* hullMesh = new Mesh(vertices.data(), edges.data(), facets.data(), vertices.size(), edges.size(), facets.size());
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
		buildCollisionGeometry(hPoints, in[i]);
	}
}


PxConvexMesh* ConvexMeshBuilder::buildConvexMesh(std::vector<PxVec3>& vertexData)
{
	CollisionHull hull;
	buildCollisionGeometry(vertexData, hull);

	PxConvexMeshDesc convexMeshDescr;
	convexMeshDescr.indices.data = hull.indices.data();
	convexMeshDescr.indices.count = (uint32_t)hull.indices.size();
	convexMeshDescr.indices.stride = sizeof(uint32_t);

	convexMeshDescr.points.data = hull.points.data();
	convexMeshDescr.points.count = (uint32_t)hull.points.size();
	convexMeshDescr.points.stride = sizeof(PxVec3);

	convexMeshDescr.polygons.data = hull.polygonData.data();
	convexMeshDescr.polygons.count = (uint32_t)hull.polygonData.size();
	convexMeshDescr.polygons.stride = sizeof(PxHullPolygon);

	PxConvexMesh* convexMesh = mCooking->createConvexMesh(convexMeshDescr, *mInsertionCallback);
	return convexMesh;
}

PxConvexMesh* ConvexMeshBuilder::buildConvexMesh(CollisionHull& hull)
{	
	PxConvexMeshDesc convexMeshDescr;
	convexMeshDescr.indices.data = hull.indices.data();
	convexMeshDescr.indices.count = (uint32_t)hull.indices.size();
	convexMeshDescr.indices.stride = sizeof(uint32_t);

	convexMeshDescr.points.data = hull.points.data();
	convexMeshDescr.points.count = (uint32_t)hull.points.size();
	convexMeshDescr.points.stride = sizeof(PxVec3);

	convexMeshDescr.polygons.data = hull.polygonData.data();
	convexMeshDescr.polygons.count = (uint32_t)hull.polygonData.size();
	convexMeshDescr.polygons.stride = sizeof(PxHullPolygon);

	PxConvexMesh* convexMesh = mCooking->createConvexMesh(convexMeshDescr, *mInsertionCallback);
	return convexMesh;
}


} // namespace Blast
} // namespace Nv
