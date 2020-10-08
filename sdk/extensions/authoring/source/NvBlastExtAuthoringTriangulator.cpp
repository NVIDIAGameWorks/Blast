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


// This warning arises when using some stl containers with older versions of VC
// c:\program files (x86)\microsoft visual studio 12.0\vc\include\xtree(1826): warning C4702: unreachable code
#include "NvPreprocessor.h"
#if NV_VC && NV_VC < 14
#pragma warning(disable : 4702)
#endif
#include "NvBlastExtAuthoringTriangulator.h"
#include "NvBlastExtAuthoringMesh.h"
#include "NvBlastExtAuthoringTypes.h"
#include <math.h>
#include "NvPreprocessor.h"
#include <algorithm>
#include <vector>
#include <set>
#include "NvBlastExtAuthoringBooleanTool.h"
#include <queue>
#include <NvBlastAssert.h>
#include <NvBlastPxSharedHelpers.h>

using physx::PxVec2;
using physx::PxVec3;

namespace Nv
{
namespace Blast
{
NV_FORCE_INLINE bool compareTwoFloats(float a, float b)
{
	return std::abs(b - a) <= FLT_EPSILON * std::abs(b + a);
}
NV_FORCE_INLINE bool compareTwoVertices(const PxVec3& a, const PxVec3& b)
{
	return compareTwoFloats(a.x, b.x) && compareTwoFloats(a.y, b.y) && compareTwoFloats(a.z, b.z);
}
NV_FORCE_INLINE bool compareTwoVertices(const PxVec2& a, const PxVec2& b)
{
	return compareTwoFloats(a.x, b.x) && compareTwoFloats(a.y, b.y);
}

NV_FORCE_INLINE float getRotation(const PxVec2& a, const PxVec2& b)
{
	return a.x * b.y - a.y * b.x;
}

NV_FORCE_INLINE bool pointInside(PxVec2 a, PxVec2 b, PxVec2 c, PxVec2 pnt)
{
	if (compareTwoVertices(a, pnt) || compareTwoVertices(b, pnt) || compareTwoVertices(c, pnt))
	{
		return false;
	}
	float v1 = (getRotation((b - a), (pnt - a)));
	float v2 = (getRotation((c - b), (pnt - b)));
	float v3 = (getRotation((a - c), (pnt - c)));

	return (v1 >= 0.0f && v2 >= 0.0f && v3 >= 0.0f) || (v1 <= 0.0f && v2 <= 0.0f && v3 <= 0.0f);
}
void Triangulator::triangulatePolygonWithEarClipping(std::vector<uint32_t>& inputPolygon, Vertex* vert,
                                                     ProjectionDirections dir)
{
	//	return;
	// for (uint32_t i = 0; i < inputPolygon.size(); ++i)
	//{
	//	mBaseMeshTriangles.push_back(TriangleIndexed(inputPolygon[i], inputPolygon[i], inputPolygon[(i + 1) %
	//inputPolygon.size()]));
	//}
	// return;
	int32_t vCount = static_cast<int32_t>(inputPolygon.size());

	if (vCount < 3)
	{
		return;
	}
	for (int32_t curr = 0; curr < vCount && vCount > 2; ++curr)
	{
		int32_t prev = (curr == 0) ? vCount - 1 : curr - 1;
		int32_t next = (curr == vCount - 1) ? 0 : curr + 1;

		Vertex cV = vert[inputPolygon[curr]];
		Vertex nV = vert[inputPolygon[prev]];
		Vertex pV = vert[inputPolygon[next]];

		PxVec2 cVp = getProjectedPoint(cV.p, dir);
		PxVec2 nVp = getProjectedPoint(nV.p, dir);
		PxVec2 pVp = getProjectedPoint(pV.p, dir);

		// Check wheather curr is ear-tip
		float rot = getRotation((pVp - nVp).getNormalized(), (cVp - nVp).getNormalized());
		if (!(dir & OPPOSITE_WINDING))
			rot = -rot;
		if (rot > 0.0001)
		{
			bool good = true;
			for (int vrt = 0; vrt < vCount; ++vrt)
			{
				if (vrt == curr || vrt == prev || vrt == next)
					continue;
				if (pointInside(cVp, nVp, pVp, getProjectedPoint(vert[inputPolygon[vrt]].p, dir)))
				{
					good = false;
					break;
				}
			}
			if (good)
			{
				mBaseMeshTriangles.push_back(TriangleIndexed(inputPolygon[curr], inputPolygon[prev], inputPolygon[next]));
				vCount--;
				inputPolygon.erase(inputPolygon.begin() + curr);
				curr = -1;
			}
		}
	}
}


struct LoopInfo
{
	LoopInfo()
	{
		used = false;
	}
	PxVec3 normal;
	float area;
	int32_t index;
	bool used;
	bool operator<(const LoopInfo& b) const
	{
		return area < b.area;
	}
};

int32_t unitePolygons(std::vector<uint32_t>& externalLoop, std::vector<uint32_t>& internalLoop, Vertex* vrx,
                      ProjectionDirections dir)
{
	if (externalLoop.size() < 3 || internalLoop.size() < 3)
		return 1;
	/**
	    Find point with maximum x-coordinate
	*/
	float x_max    = -MAXIMUM_EXTENT;
	int32_t mIndex = -1;
	for (uint32_t i = 0; i < internalLoop.size(); ++i)
	{
		float nx = getProjectedPoint(vrx[internalLoop[i]].p, dir).x;
		if (nx > x_max)
		{
			mIndex = i;
			x_max  = nx;
		}
	}
	if (mIndex == -1)
	{
		return 1;
	}

	/**
	    Search for base point on external loop
	*/
	float minX        = MAXIMUM_EXTENT;
	int32_t vrtIndex  = -1;
	bool isFromBuffer = 0;
	PxVec2 holePoint  = getProjectedPoint(vrx[internalLoop[mIndex]].p, dir);
	PxVec2 computedPoint;
	for (uint32_t i = 0; i < externalLoop.size(); ++i)
	{
		int32_t nx  = (i + 1) % externalLoop.size();
		PxVec2 pnt1 = getProjectedPoint(vrx[externalLoop[i]].p, dir);
		PxVec2 pnt2 = getProjectedPoint(vrx[externalLoop[nx]].p, dir);
		if (pnt1.x < x_max && pnt2.x < x_max)
		{
			continue;
		}
		PxVec2 vc = pnt2 - pnt1;
		if (vc.y == 0 && pnt1.y == holePoint.y)
		{
			if (pnt1.x < minX && pnt1.x < pnt2.x && pnt1.x > x_max)
			{
				minX         = pnt1.x;
				vrtIndex     = i;
				isFromBuffer = true;
			}
			if (pnt2.x < minX && pnt2.x < pnt1.x && pnt2.x > x_max)
			{
				minX         = pnt2.x;
				vrtIndex     = nx;
				isFromBuffer = true;
			}
		}
		else
		{
			float t = (holePoint.y - pnt1.y) / vc.y;
			if (t <= 1 && t >= 0)
			{
				PxVec2 tempPoint = vc * t + pnt1;
				if (tempPoint.x < minX && tempPoint.x > x_max)
				{
					minX          = tempPoint.x;
					vrtIndex      = i;
					isFromBuffer  = false;
					computedPoint = tempPoint;
				}
			}
		}
	}
	if (vrtIndex == -1)
	{
		//	std::cout << "Triangulation: base vertex for inner loop is not found..." << std::endl;
		return 1;
	}
	int32_t bridgePoint = -1;
	float bestAngle     = 100;
	if (!isFromBuffer)
	{
		PxVec2 ex1 = getProjectedPoint(vrx[externalLoop[vrtIndex]].p, dir);
		PxVec2 ex2 = getProjectedPoint(vrx[externalLoop[(vrtIndex + 1) % externalLoop.size()]].p, dir);

		if (ex1.x > ex2.x)
		{
			vrtIndex = (vrtIndex + 1) % externalLoop.size();
			ex1      = ex2;
		}
		/* Check if some point is inside triangle */
		bool notFound = true;
		for (int32_t i = 0; i < (int32_t)externalLoop.size(); ++i)
		{
			PxVec2 tempPoint = getProjectedPoint(vrx[externalLoop[i]].p, dir);
			if (pointInside(holePoint, ex1, computedPoint, tempPoint))
			{
				notFound   = false;
				PxVec2 cVp = getProjectedPoint(vrx[externalLoop[i]].p, dir);
				PxVec2 pVp =
				    getProjectedPoint(vrx[externalLoop[(i - 1 + externalLoop.size()) % externalLoop.size()]].p, dir);
				PxVec2 nVp = getProjectedPoint(vrx[externalLoop[(i + 1) % externalLoop.size()]].p, dir);
				float rt   = getRotation((cVp - pVp).getNormalized(), (nVp - pVp).getNormalized());
				if ((dir & OPPOSITE_WINDING))
					rt = -rt;
				if (rt < 0.000001)
					continue;
				float tempAngle = PxVec2(1, 0).dot((tempPoint - holePoint).getNormalized());
				if (bestAngle < tempAngle)
				{
					bestAngle   = tempAngle;
					bridgePoint = i;
				}
			}
		}
		if (notFound)
		{
			bridgePoint = vrtIndex;
		}
		if (bridgePoint == -1)
		{
			//	std::cout << "Triangulation: bridge vertex for inner loop is not found..." << std::endl;
			return 1;
		}
	}
	else
	{
		bridgePoint = vrtIndex;
	}
	std::vector<uint32_t> temporal;

	for (int32_t i = 0; i <= bridgePoint; ++i)
	{
		temporal.push_back(externalLoop[i]);
	}
	temporal.push_back(internalLoop[mIndex]);
	for (int32_t i = (mIndex + 1) % internalLoop.size(); i != mIndex; i = (i + 1) % internalLoop.size())
	{
		temporal.push_back(internalLoop[i]);
	}
	temporal.push_back(internalLoop[mIndex]);
	for (uint32_t i = bridgePoint; i < externalLoop.size(); ++i)
	{
		temporal.push_back(externalLoop[i]);
	}
	externalLoop = temporal;
	return 0;
}

void Triangulator::buildPolygonAndTriangulate(std::vector<Edge>& edges, Vertex* vertices, int32_t userData,
                                              int32_t materialId, int32_t smoothingGroup)
{
	std::vector<std::vector<uint32_t> > serializedLoops;

	std::set<int> visitedVertices;
	std::vector<int> used(edges.size(), 0);
	uint32_t collected = 0;

	std::vector<int> edgesIds;
	/**
	Add first edge to polygon
	*/
	edgesIds.push_back(0);
	visitedVertices.insert(edges[0].s);
	visitedVertices.insert(edges[0].e);
	used[0]              = true;
	collected            = 1;
	uint32_t lastEdge    = 0;
	bool successfullPass = false;
	for (; collected < edges.size();)
	{
		successfullPass = false;
		for (uint32_t p = 0; p < edges.size(); ++p)
		{
			if (used[p] == 0 && edges[p].s == edges[lastEdge].e)
			{
				successfullPass = true;
				collected++;
				used[p] = true;
				edgesIds.push_back(p);
				lastEdge = p;
				if (visitedVertices.find(edges[p].e) != visitedVertices.end())  // if we formed loop, detach it and
				                                                                // triangulate
				{
					serializedLoops.push_back(std::vector<uint32_t>());
					std::vector<uint32_t>& serializedPositions = serializedLoops.back();
					while (edgesIds.size() > 0)
					{
						serializedPositions.push_back(edges[edgesIds.back()].s);
						visitedVertices.erase(edges[edgesIds.back()].s);
						if (edges[edgesIds.back()].s == edges[p].e)
						{
							edgesIds.pop_back();
							break;
						}
						edgesIds.pop_back();
					}
					if (edgesIds.size() > 0)
					{
						lastEdge = edgesIds.back();
					}
					else
					{
						for (uint32_t t = 0; t < edges.size(); ++t)
						{
							if (used[t] == 0)
							{
								edgesIds.push_back(t);
								visitedVertices.insert(edges[t].s);
								visitedVertices.insert(edges[t].e);
								used[t] = true;
								collected++;
								lastEdge = t;
								break;
							}
						}
					}
				}
				else
				{
					visitedVertices.insert(edges[p].e);
				}
			}
		}
		if (!successfullPass)
		{
			break;
		}
	}

	std::vector<LoopInfo> loopsInfo(serializedLoops.size());
	// Compute normal to whole polygon, and areas of loops
	PxVec3 wholeFacetNormal(0, 0, 0);
	for (uint32_t loop = 0; loop < serializedLoops.size(); ++loop)
	{
		PxVec3 loopNormal(0, 0, 0);
		std::vector<uint32_t>& pos = serializedLoops[loop];
		for (uint32_t vrt = 1; vrt + 1 < serializedLoops[loop].size(); ++vrt)
		{
			loopNormal += toPxShared(vertices[pos[vrt]].p - vertices[pos[0]].p)
			                  .cross(toPxShared(vertices[pos[vrt + 1]].p - vertices[pos[0]].p));
		}
		loopsInfo[loop].area   = loopNormal.magnitude();
		loopsInfo[loop].normal = loopNormal;
		loopsInfo[loop].index  = loop;
		wholeFacetNormal += loopNormal;
	}

	// Change areas signs according to winding direction
	for (uint32_t loop = 0; loop < serializedLoops.size(); ++loop)
	{
		if (wholeFacetNormal.dot(loopsInfo[loop].normal) < 0)
		{
			loopsInfo[loop].area = -loopsInfo[loop].area;
		}
	}
	ProjectionDirections dir = getProjectionDirection(wholeFacetNormal);
	std::sort(loopsInfo.begin(), loopsInfo.end());

	std::vector<PxVec3> tempPositions;
	int32_t oldSize = static_cast<int32_t>(mBaseMeshTriangles.size());
	for (uint32_t extPoly = 0; extPoly < loopsInfo.size(); ++extPoly)
	{
		if (loopsInfo[extPoly].area < 0)
		{
			continue;  // Polygon with negative area is hole
		}
		int32_t baseLoop = loopsInfo[extPoly].index;
		for (uint32_t intPoly = 0; intPoly < loopsInfo.size(); ++intPoly)
		{
			if (loopsInfo[intPoly].area > 0 || loopsInfo[intPoly].used ||
			    std::abs(loopsInfo[intPoly].area) > loopsInfo[extPoly].area)
			{
				continue;
			}
			int32_t holeLoop = loopsInfo[intPoly].index;

			if (!unitePolygons(serializedLoops[baseLoop], serializedLoops[holeLoop], vertices, dir))
			{
				loopsInfo[intPoly].used = true;
			};
		}
		triangulatePolygonWithEarClipping(serializedLoops[baseLoop], vertices, dir);
	}
	for (uint32_t i = oldSize; i < mBaseMeshTriangles.size(); ++i)
	{
		mBaseMeshTriangles[i].userData       = userData;
		mBaseMeshTriangles[i].materialId     = materialId;
		mBaseMeshTriangles[i].smoothingGroup = smoothingGroup;
	}
}

NV_FORCE_INLINE int32_t Triangulator::addVerticeIfNotExist(const Vertex& p)
{
	auto it = mVertMap.find(p);
	if (it == mVertMap.end())
	{
		mVertMap[p] = static_cast<int32_t>(mVertices.size());
		mVertices.push_back(p);
		return static_cast<int32_t>(mVertices.size()) - 1;
	}
	else
	{
		return it->second;
	}
}

NV_FORCE_INLINE void Triangulator::addEdgeIfValid(EdgeWithParent& ed)
{
	if (ed.s == ed.e)
		return;
	EdgeWithParent opposite(ed.e, ed.s, ed.parent);
	auto it = mEdgeMap.find(opposite);
	if (it == mEdgeMap.end())
	{
		mEdgeMap[ed] = static_cast<int32_t>(mBaseMeshEdges.size());
		mBaseMeshEdges.push_back(ed);
	}
	else
	{
		if (mBaseMeshEdges[it->second].s == kNotValidVertexIndex)
		{
			mBaseMeshEdges[it->second].s = ed.s;
			mBaseMeshEdges[it->second].e = ed.e;
		}
		else
		{
			mBaseMeshEdges[it->second].s = kNotValidVertexIndex;
		}
	}
}


void Triangulator::prepare(const Mesh* mesh)
{
	const Edge* ed   = mesh->getEdges();
	const Vertex* vr = mesh->getVertices();
	mBaseMapping.resize(mesh->getVerticesCount());
	for (uint32_t i = 0; i < mesh->getFacetCount(); ++i)
	{
		const Facet* fc = mesh->getFacet(i);
		for (uint32_t j = fc->firstEdgeNumber; j < fc->firstEdgeNumber + fc->edgesCount; ++j)
		{
			int32_t a             = addVerticeIfNotExist(vr[ed[j].s]);
			int32_t b             = addVerticeIfNotExist(vr[ed[j].e]);
			mBaseMapping[ed[j].s] = a;
			mBaseMapping[ed[j].e] = b;
			EdgeWithParent e(a, b, i);
			addEdgeIfValid(e);
		}
	}
	std::vector<EdgeWithParent> temp;
	temp.reserve(mBaseMeshEdges.size());
	for (uint32_t i = 0; i < mBaseMeshEdges.size(); ++i)
	{
		if (mBaseMeshEdges[i].s != kNotValidVertexIndex)
		{
			temp.push_back(mBaseMeshEdges[i]);
		}
	}
	mBaseMeshEdges = temp;
}

void Triangulator::reset()
{
	mVertices.clear();
	mBaseMeshEdges.clear();
	mVertMap.clear();
	mEdgeMap.clear();
	mBaseMeshTriangles.clear();
	mBaseMeshResultTriangles.clear();
}

void Triangulator::triangulate(const Mesh* mesh)
{
	reset();
	if (mesh == nullptr || !mesh->isValid())
	{
		return;
	}
	prepare(mesh);
	if (mBaseMeshEdges.empty())
	{
		return;
	}
	std::vector<Edge> temp;
	uint32_t fP = mBaseMeshEdges[0].parent;
	for (uint32_t i = 0; i < mBaseMeshEdges.size(); ++i)
	{
		if (fP != mBaseMeshEdges[i].parent)
		{
			if (temp.empty() == false)
			{
				buildPolygonAndTriangulate(temp, mVertices.data(), mesh->getFacet(fP)->userData,
				                           mesh->getFacet(fP)->materialId, mesh->getFacet(fP)->smoothingGroup);
			}
			temp.clear();
			fP = mBaseMeshEdges[i].parent;
		}
		temp.push_back({ mBaseMeshEdges[i].s, mBaseMeshEdges[i].e });
	}
	buildPolygonAndTriangulate(temp, mVertices.data(), mesh->getFacet(fP)->userData, mesh->getFacet(fP)->materialId,
	                           mesh->getFacet(fP)->smoothingGroup);

	/* Build final triangles */
	mBaseMeshResultTriangles.clear();
	for (uint32_t i = 0; i < mBaseMeshTriangles.size(); ++i)
	{
		if (mBaseMeshTriangles[i].ea == kNotValidVertexIndex)
		{
			continue;
		}
		mBaseMeshResultTriangles.push_back({ mVertices[mBaseMeshTriangles[i].ea], mVertices[mBaseMeshTriangles[i].eb],
		                                     mVertices[mBaseMeshTriangles[i].ec], mBaseMeshTriangles[i].userData,
		                                     mBaseMeshTriangles[i].materialId, mBaseMeshTriangles[i].smoothingGroup });
	}
	mBaseMeshUVFittedTriangles = mBaseMeshResultTriangles;  // Uvs will be fitted later, in FractureTool.
	computePositionedMapping();
}

void Triangulator::computePositionedMapping()
{
	std::map<NvcVec3, int32_t, VrtPositionComparator> mPosMap;
	mPositionMappedVrt.clear();
	mPositionMappedVrt.resize(mVertices.size());

	for (uint32_t i = 0; i < mVertices.size(); ++i)
	{
		auto it = mPosMap.find(mVertices[i].p);

		if (it == mPosMap.end())
		{
			mPosMap[mVertices[i].p] = i;
			mPositionMappedVrt[i]   = i;
		}
		else
		{
			mPositionMappedVrt[i] = it->second;
		}
	}
}

}  // namespace Blast
}  // namespace Nv