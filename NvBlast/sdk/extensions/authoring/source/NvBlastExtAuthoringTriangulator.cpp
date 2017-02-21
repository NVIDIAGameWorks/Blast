/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

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
#include "NvBlastExtAuthoringPerlinNoise.h"
#include <NvBlastAssert.h>

using physx::PxVec3;
using physx::PxVec2;

#define VEC_COMPARISON_OFFSET 1e-5f
#define TWO_VERTICES_THRESHOLD 1e-7

namespace Nv
{
namespace Blast
{

bool VrtComp::operator()(const Vertex& a, const Vertex& b) const
{
	if (a.p.x + VEC_COMPARISON_OFFSET < b.p.x) return true;
	if (a.p.x - VEC_COMPARISON_OFFSET > b.p.x) return false;
	if (a.p.y + VEC_COMPARISON_OFFSET < b.p.y) return true;
	if (a.p.y - VEC_COMPARISON_OFFSET > b.p.y) return false;
	if (a.p.z + VEC_COMPARISON_OFFSET < b.p.z) return true;
	if (a.p.z - VEC_COMPARISON_OFFSET > b.p.z) return false;

	if (a.n.x + 1e-3 < b.n.x) return true;
	if (a.n.x - 1e-3 > b.n.x) return false;
	if (a.n.y + 1e-3 < b.n.y) return true;
	if (a.n.y - 1e-3 > b.n.y) return false;
	if (a.n.z + 1e-3 < b.n.z) return true;
	if (a.n.z - 1e-3 > b.n.z) return false;


	if (a.uv[0].x + 1e-3 < b.uv[0].x) return true;
	if (a.uv[0].x - 1e-3 > b.uv[0].x) return false;
	if (a.uv[0].y + 1e-3 < b.uv[0].y) return true;
	return false;
}

bool VrtPositionComparator::operator()(const PxVec3& a, const PxVec3& b) const
{
	if (a.x + VEC_COMPARISON_OFFSET < b.x) return true;
	if (a.x - VEC_COMPARISON_OFFSET > b.x) return false;
	if (a.y + VEC_COMPARISON_OFFSET < b.y) return true;
	if (a.y - VEC_COMPARISON_OFFSET > b.y) return false;
	if (a.z + VEC_COMPARISON_OFFSET < b.z) return true;
	if (a.z - VEC_COMPARISON_OFFSET > b.z) return false;
	return false;
}

NV_FORCE_INLINE bool compareTwoVertices(const PxVec3& a, const PxVec3& b)
{
	return std::abs(b.x - a.x) < TWO_VERTICES_THRESHOLD && std::abs(b.y - a.y) < TWO_VERTICES_THRESHOLD && std::abs(b.z - a.z) < TWO_VERTICES_THRESHOLD;
}
NV_FORCE_INLINE bool compareTwoVertices(const PxVec2& a, const PxVec2& b)
{
	return std::abs(b.x - a.x) < TWO_VERTICES_THRESHOLD && std::abs(b.y - a.y) < TWO_VERTICES_THRESHOLD;
}

NV_FORCE_INLINE float getRotation(const PxVec2& a, const PxVec2& b)
{
	return a.x * b.y - a.y * b.x;
}

inline bool pointInside(PxVec2 a, PxVec2 b, PxVec2 c, PxVec2 pnt)
{
	if (compareTwoVertices(a, pnt) || compareTwoVertices(b, pnt) || compareTwoVertices(c, pnt))
	{
		return false;
	}
	float v1 = (getRotation((b - a), (pnt - a)));
	float v2 = (getRotation((c - b), (pnt - b)));
	float v3 = (getRotation((a - c), (pnt - c)));

	return (v1 >= 0.0f && v2 >= 0.0f && v3 >= 0.0f) ||
		(v1 <= 0.0f && v2 <= 0.0f && v3 <= 0.0f);

}
void ChunkPostProcessor::triangulatePolygonWithEarClipping(std::vector<uint32_t>& inputPolygon, Vertex* vert, ProjectionDirections dir)
{
	//	return;
	//for (uint32_t i = 0; i < inputPolygon.size(); ++i)
	//{
	//	mBaseMeshTriangles.push_back(TriangleIndexed(inputPolygon[i], inputPolygon[i], inputPolygon[(i + 1) % inputPolygon.size()]));
	//}
	//return;
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
		if (!(dir & OPPOSITE_WINDING)) rot = -rot;
		if (rot > 0.0001)
		{
			bool good = true;
			for (int vrt = 0; vrt < vCount; ++vrt)
			{
				if (vrt == curr || vrt == prev || vrt == next) continue;
				if (pointInside(cVp, nVp, pVp, getProjectedPoint(vert[inputPolygon[vrt]].p, dir)))
				{
					good = false;
					break;
				}
			}
			if (good)
			{
				addEdgeTr(Edge(inputPolygon[curr], inputPolygon[prev]));
				addEdgeTr(Edge(inputPolygon[next], inputPolygon[prev]));
				addEdgeTr(Edge(inputPolygon[curr], inputPolygon[next]));

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

int32_t unitePolygons(std::vector<uint32_t>& externalLoop, std::vector<uint32_t>& internalLoop, Vertex* vrx, ProjectionDirections dir)
{
	if (externalLoop.size() < 3 || internalLoop.size() < 3)
		return 1;
	/**
		Find point with maximum x-coordinate
	*/
	float x_max = -MAXIMUM_EXTENT;
	int32_t mIndex = -1;
	for (uint32_t i = 0; i < internalLoop.size(); ++i)
	{
		float nx = getProjectedPoint(vrx[internalLoop[i]].p, dir).x;
		if (nx > x_max)
		{
			mIndex = i;
			x_max = nx;
		}
	}
	if (mIndex == -1)
	{
		return 1;
	}

	/**
		Search for base point on external loop
	*/
	float minX = MAXIMUM_EXTENT;
	int32_t vrtIndex = -1;
	bool isFromBuffer = 0;
	PxVec2 holePoint = getProjectedPoint(vrx[internalLoop[mIndex]].p, dir);
	PxVec2 computedPoint;
	for (uint32_t i = 0; i < externalLoop.size(); ++i)
	{
		int32_t nx = (i + 1) % externalLoop.size();
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
				minX = pnt1.x;
				vrtIndex = i;
				isFromBuffer = true;
			}
			if (pnt2.x < minX && pnt2.x < pnt1.x && pnt2.x > x_max)
			{
				minX = pnt2.x;
				vrtIndex = nx;
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
					minX = tempPoint.x;
					vrtIndex = i;
					isFromBuffer = false;
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
	float bestAngle = 100;
	if (!isFromBuffer)
	{
		PxVec2 ex1 = getProjectedPoint(vrx[externalLoop[vrtIndex]].p, dir);
		PxVec2 ex2 = getProjectedPoint(vrx[externalLoop[(vrtIndex + 1) % externalLoop.size()]].p, dir);

		if (ex1.x > ex2.x)
		{
			vrtIndex = (vrtIndex + 1) % externalLoop.size();
			ex1 = ex2;
		}
		/* Check if some point is inside triangle */
		bool notFound = true;
		for (int32_t i = 0; i < (int32_t)externalLoop.size(); ++i)
		{
			PxVec2 tempPoint = getProjectedPoint(vrx[externalLoop[i]].p, dir);
			if (pointInside(holePoint, ex1, computedPoint, tempPoint))
			{
				notFound = false;
				PxVec2 cVp = getProjectedPoint(vrx[externalLoop[i]].p, dir);
				PxVec2 pVp = getProjectedPoint(vrx[externalLoop[(i - 1 + externalLoop.size()) % externalLoop.size()]].p, dir);
				PxVec2 nVp = getProjectedPoint(vrx[externalLoop[(i + 1) % externalLoop.size()]].p, dir);
				float rt = getRotation((cVp - pVp).getNormalized(), (nVp - pVp).getNormalized());
				if ((dir & OPPOSITE_WINDING)) rt = -rt;
				if (rt < 0.000001)
					continue;
				float tempAngle = PxVec2(1, 0).dot((tempPoint - holePoint).getNormalized());
				if (bestAngle < tempAngle)
				{
					bestAngle = tempAngle;
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

void ChunkPostProcessor::buildPolygonAndTriangulate(std::vector<Edge>& edges, Vertex* vertices, int32_t userData)
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
	used[0] = true;
	collected = 1;
	uint32_t lastEdge = 0;
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
				if (visitedVertices.find(edges[p].e) != visitedVertices.end()) // if we formed loop, detach it and triangulate
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
			loopNormal += (vertices[pos[vrt]].p - vertices[pos[0]].p).cross(vertices[pos[vrt + 1]].p - vertices[pos[0]].p);
		}
		loopsInfo[loop].area = loopNormal.magnitude();
		loopsInfo[loop].normal = loopNormal;
		loopsInfo[loop].index = loop;
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
			continue; // Polygon with negative area is hole
		}
		int32_t baseLoop = loopsInfo[extPoly].index;
		for (uint32_t intPoly = 0; intPoly < loopsInfo.size(); ++intPoly)
		{
			if (loopsInfo[intPoly].area > 0 || loopsInfo[intPoly].used || abs(loopsInfo[intPoly].area) > loopsInfo[extPoly].area)
			{
				continue;
			}
			int32_t holeLoop = loopsInfo[intPoly].index;

			if (!unitePolygons(serializedLoops[baseLoop], serializedLoops[holeLoop], vertices, dir))
			{
				loopsInfo[intPoly].used = true;
			};
		}
		triangulatePolygonWithEarClipping(serializedLoops[baseLoop],vertices, dir);
	}
	for (uint32_t i = oldSize; i < mBaseMeshTriangles.size(); ++i)
	{
		mBaseMeshTriangles[i].userInfo = userData;
	}
}

NV_FORCE_INLINE int32_t ChunkPostProcessor::addVerticeIfNotExist(Vertex& p)
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

NV_FORCE_INLINE void ChunkPostProcessor::addEdgeIfValid(EdgeWithParent& ed)
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
		if (mBaseMeshEdges[it->second].s == NOT_VALID_VERTEX)
		{
			mBaseMeshEdges[it->second].s = ed.s;
			mBaseMeshEdges[it->second].e = ed.e;
		}
		mBaseMeshEdges[it->second].s = NOT_VALID_VERTEX;
	}
}



void ChunkPostProcessor::prepare(Mesh* mesh)
{
	Edge* ed = mesh->getEdges();
	Vertex* vr = mesh->getVertices();
	mBaseMapping.resize(mesh->getVerticesCount());
	for (uint32_t i = 0; i < mesh->getFacetCount(); ++i)
	{
		Facet* fc = mesh->getFacet(i);
		for (uint32_t j = fc->firstEdgeNumber; j < fc->firstEdgeNumber + fc->edgesCount; ++j)
		{
			int32_t a = addVerticeIfNotExist(vr[ed[j].s]);
			int32_t b = addVerticeIfNotExist(vr[ed[j].e]);
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
		if (mBaseMeshEdges[i].s != NOT_VALID_VERTEX)
		{
			temp.push_back(mBaseMeshEdges[i]);
		}
	}

}

void ChunkPostProcessor::reset()
{
	isTesselated = false;
	mVertices.clear();
	mBaseMeshEdges.clear();
	mVertMap.clear();
	mEdgeMap.clear();
	mTrMeshEdgeMap.clear();
	mTrMeshEdges.clear();
	mTrMeshEdToTr.clear();
	mBaseMeshTriangles.clear();
	mEdgeFlag.clear();
	mVertexValence.clear();
	mRestrictionFlag.clear();
	mVerticesDistances.clear();
	mVerticesNormalsSmoothed.clear();

	mBaseMeshResultTriangles.clear();
	mTesselatedMeshResultTriangles.clear();
	mTesselatedMeshTriangles.clear();
}

void ChunkPostProcessor::triangulate(Mesh* mesh)
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
	int32_t fP = mBaseMeshEdges[0].parent;
	for (uint32_t i = 0; i < mBaseMeshEdges.size(); ++i)
	{
		if (fP != mBaseMeshEdges[i].parent)
		{
			if (temp.empty() == false)
			{
				buildPolygonAndTriangulate(temp, &mVertices[0], mesh->getFacet(fP)->userData);
			}
			temp.clear();
			fP = mBaseMeshEdges[i].parent;
		}
		temp.push_back(Edge(mBaseMeshEdges[i].s, mBaseMeshEdges[i].e));
	}
	buildPolygonAndTriangulate(temp, &mVertices[0], mesh->getFacet(fP)->userData);

	/* Build final triangles */

	mBaseMeshResultTriangles.clear();
	for (uint32_t i = 0; i < mBaseMeshTriangles.size(); ++i)
	{
		if (mBaseMeshTriangles[i].ea == NOT_VALID_VERTEX)
		{
			continue;
		}
		mBaseMeshResultTriangles.push_back(Triangle(mVertices[mBaseMeshTriangles[i].ea], mVertices[mBaseMeshTriangles[i].eb], mVertices[mBaseMeshTriangles[i].ec]));
		mBaseMeshResultTriangles.back().userInfo = mBaseMeshTriangles[i].userInfo;
	}

	computePositionedMapping();
}

void ChunkPostProcessor::prebuildTesselatedTriangles()
{
	mTesselatedMeshResultTriangles.clear();
	for (uint32_t i = 0; i < mTesselatedMeshTriangles.size(); ++i)
	{
		if (mTesselatedMeshTriangles[i].ea == NOT_VALID_VERTEX)
		{
			continue;
		}
		mTesselatedMeshResultTriangles.push_back(Triangle(mVertices[mTesselatedMeshTriangles[i].ea], mVertices[mTesselatedMeshTriangles[i].eb], mVertices[mTesselatedMeshTriangles[i].ec]));
		mTesselatedMeshResultTriangles.back().userInfo = mTesselatedMeshTriangles[i].userInfo;
	}

}


int32_t ChunkPostProcessor::addEdgeTr(const Edge& e)
{
	Edge ed = e;
	if (ed.e < ed.s) std::swap(ed.s, ed.e);
	auto it = mTrMeshEdgeMap.find(ed);
	if (it == mTrMeshEdgeMap.end())
	{
		mTrMeshEdToTr.push_back(EdgeToTriangles());
		mTrMeshEdgeMap[ed] = (int)mTrMeshEdToTr.size() - 1;
		mTrMeshEdges.push_back(ed);
		mEdgeFlag.push_back(INTERNAL_EDGE);
		return (int32_t)mTrMeshEdToTr.size() - 1;
	}
	else
	{
		return it->second;
	}
}

int32_t ChunkPostProcessor::findEdge(const Edge& e)
{
	Edge ed = e;
	if (ed.e < ed.s) std::swap(ed.s, ed.e);
	auto it = mTrMeshEdgeMap.find(ed);
	if (it == mTrMeshEdgeMap.end())
	{
		return -1;
	}
	return it->second;
}

void ChunkPostProcessor::updateEdgeTriangleInfo()
{
	mTrMeshEdToTr.clear();
	mTrMeshEdToTr.resize(mTrMeshEdges.size());
	for (uint32_t i = 0; i < mTesselatedMeshTriangles.size(); ++i)
	{
		TriangleIndexed& tr = mTesselatedMeshTriangles[i];
		if (tr.ea == NOT_VALID_VERTEX)
			continue;
		int32_t ed = addEdgeTr(Edge(tr.ea, tr.eb));
		mTrMeshEdToTr[ed].add(i);
		ed = addEdgeTr(Edge(tr.ea, tr.ec));
		mTrMeshEdToTr[ed].add(i);
		ed = addEdgeTr(Edge(tr.ec, tr.eb));
		mTrMeshEdToTr[ed].add(i);
	}
}

void ChunkPostProcessor::updateVertEdgeInfo()
{
	mVertexToTriangleMap.clear();
	mVertexToTriangleMap.resize(mVertices.size());
	for (uint32_t i = 0; i < mTesselatedMeshTriangles.size(); ++i)
	{
		TriangleIndexed& tr = mTesselatedMeshTriangles[i];
		if (tr.ea == NOT_VALID_VERTEX) continue;
		mVertexToTriangleMap[tr.ea].push_back(i);
		mVertexToTriangleMap[tr.eb].push_back(i);
		mVertexToTriangleMap[tr.ec].push_back(i);
	}
	mVertexValence.clear();
	mVertexValence.resize(mVertices.size(), 0);

	for (uint32_t i = 0; i < mTrMeshEdges.size(); ++i)
	{
		if (mTrMeshEdToTr[i].c != 0)
		{
			mVertexValence[mTrMeshEdges[i].s]++;
			mVertexValence[mTrMeshEdges[i].e]++;
		}
	}
}


void ChunkPostProcessor::collapseEdge(int32_t id)
{
	Edge cEdge = mTrMeshEdges[id];
	uint32_t from = cEdge.s;
	uint32_t to = cEdge.e;


	if (mRestrictionFlag[from] && mRestrictionFlag[to])
	{
		return;
	}
	
	if (mVertexValence[from] > mVertexValence[to])
	{
		std::swap(from, to);
	}

	if (mRestrictionFlag[from])
	{
		std::swap(from, to);
	}

	std::set<int32_t> connectedToBegin;
	std::set<int32_t> connectedToEnd;
	std::set<int32_t> neighboorTriangles;

	int32_t trWithEdge[2] = {-1, -1};
	int32_t cntr = 0;
	for (uint32_t i = 0; i < mVertexToTriangleMap[from].size(); ++i)
	{
		if (mTesselatedMeshTriangles[mVertexToTriangleMap[from][i]].ea == NOT_VALID_VERTEX)
			continue;
		if (neighboorTriangles.insert(mVertexToTriangleMap[from][i]).second && mTesselatedMeshTriangles[mVertexToTriangleMap[from][i]].isContainEdge(from, to))
		{
			trWithEdge[cntr] = mVertexToTriangleMap[from][i];
			cntr++;
		}
	}
	for (uint32_t i = 0; i < mVertexToTriangleMap[to].size(); ++i)
	{
		if (mTesselatedMeshTriangles[mVertexToTriangleMap[to][i]].ea == NOT_VALID_VERTEX)
			continue;
		if (neighboorTriangles.insert(mVertexToTriangleMap[to][i]).second && mTesselatedMeshTriangles[mVertexToTriangleMap[to][i]].isContainEdge(from, to))
		{
			trWithEdge[cntr] = mVertexToTriangleMap[to][i];
			cntr++;
		}
	}

	if (cntr == 0)
	{
		return;
	}
	if (cntr > 2)
	{
		return;
	}

	for (uint32_t i: neighboorTriangles)
	{
		if (mTesselatedMeshTriangles[i].ea == from || mTesselatedMeshTriangles[i].eb == from || mTesselatedMeshTriangles[i].ec == from)
		{
			if (mTesselatedMeshTriangles[i].ea != to && mTesselatedMeshTriangles[i].ea != from)
				connectedToBegin.insert(mTesselatedMeshTriangles[i].ea);
			if (mTesselatedMeshTriangles[i].eb != to && mTesselatedMeshTriangles[i].eb != from)
				connectedToBegin.insert(mTesselatedMeshTriangles[i].eb);
			if (mTesselatedMeshTriangles[i].ec != to && mTesselatedMeshTriangles[i].ec != from)
				connectedToBegin.insert(mTesselatedMeshTriangles[i].ec);
		}

		if (mTesselatedMeshTriangles[i].ea == to || mTesselatedMeshTriangles[i].eb == to || mTesselatedMeshTriangles[i].ec == to)
		{
			if (mTesselatedMeshTriangles[i].ea != to && mTesselatedMeshTriangles[i].ea != from)
				connectedToEnd.insert(mTesselatedMeshTriangles[i].ea);
			if (mTesselatedMeshTriangles[i].eb != to && mTesselatedMeshTriangles[i].eb != from)
				connectedToEnd.insert(mTesselatedMeshTriangles[i].eb);
			if (mTesselatedMeshTriangles[i].ec != to && mTesselatedMeshTriangles[i].ec != from)
				connectedToEnd.insert(mTesselatedMeshTriangles[i].ec);
		}
	}
	bool canBeCollapsed = true;
	for (auto it = connectedToBegin.begin(); it != connectedToBegin.end(); ++it)
	{
		uint32_t currV = *it;
		if (connectedToEnd.find(currV) == connectedToEnd.end())
			continue;
		bool found = false;
		for (int32_t tr : neighboorTriangles)
		{
			if ((mTesselatedMeshTriangles[tr].ea == from || mTesselatedMeshTriangles[tr].eb == from || mTesselatedMeshTriangles[tr].ec == from) &&
				(mTesselatedMeshTriangles[tr].ea == to || mTesselatedMeshTriangles[tr].eb == to || mTesselatedMeshTriangles[tr].ec == to) &&
				(mTesselatedMeshTriangles[tr].ea == currV || mTesselatedMeshTriangles[tr].eb == currV || mTesselatedMeshTriangles[tr].ec == currV))
			{
				found = true; 
				break;
			}
		}
		if (!found)
		{
			canBeCollapsed = false;
			break;
		}
	}
	
	if (canBeCollapsed)
	{
		for (int32_t i : neighboorTriangles)
		{
			if (trWithEdge[0] == i) continue;
			if (cntr == 2 && trWithEdge[1] == i) continue;
			TriangleIndexed tr = mTesselatedMeshTriangles[i];
			PxVec3 oldNormal = (mVertices[tr.eb].p - mVertices[tr.ea].p).cross(mVertices[tr.ec].p - mVertices[tr.ea].p);

			if (tr.ea == from)
			{
				tr.ea = to;
			}
			else
				if (tr.eb == from)
				{
					tr.eb = to;
				}
				else
					if (tr.ec == from)
					{
						tr.ec = to;
					}
			PxVec3 newNormal = (mVertices[tr.eb].p - mVertices[tr.ea].p).cross(mVertices[tr.ec].p - mVertices[tr.ea].p);
			if (newNormal.magnitude() < 1e-8f)
			{
				canBeCollapsed = false;
				break;
			}
			if (oldNormal.dot(newNormal) < 0)
			{
				canBeCollapsed = false;
				break;
			}
		}
	}

	if (canBeCollapsed)
	{
		mTesselatedMeshTriangles[trWithEdge[0]].ea = NOT_VALID_VERTEX;
		if (cntr == 2)mTesselatedMeshTriangles[trWithEdge[1]].ea = NOT_VALID_VERTEX;

		for (int32_t i : neighboorTriangles)
		{
			if (mTesselatedMeshTriangles[i].ea == NOT_VALID_VERTEX)
				continue;
			if (mTesselatedMeshTriangles[i].ea == from)
			{
				mTesselatedMeshTriangles[i].ea = to;
				mVertexToTriangleMap[from].clear();
				mVertexToTriangleMap[to].push_back(i);
			}
			else
			if (mTesselatedMeshTriangles[i].eb == from)
			{
				mTesselatedMeshTriangles[i].eb = to;
				mVertexToTriangleMap[from].clear();
				mVertexToTriangleMap[to].push_back(i);
			}
			else
			if (mTesselatedMeshTriangles[i].ec == from)
			{
				mTesselatedMeshTriangles[i].ec = to;
				mVertexToTriangleMap[from].clear();
				mVertexToTriangleMap[to].push_back(i);
			}
		}
	}
}


void ChunkPostProcessor::divideEdge(int32_t id)
{

	if (mTrMeshEdToTr[id].c == 0 )
	{
		return;
	}

	Edge cEdge = mTrMeshEdges[id];
	EdgeFlag snapRestriction = mEdgeFlag[id];
	Vertex middle;
	uint32_t nv = NOT_VALID_VERTEX;
	for (int32_t t = 0; t < mTrMeshEdToTr[id].c; ++t)
	{
		int32_t oldTriangleIndex = mTrMeshEdToTr[id].tr[t];
		TriangleIndexed tr = mTesselatedMeshTriangles[mTrMeshEdToTr[id].tr[t]];

		if (tr.ea == NOT_VALID_VERTEX)
		{
			continue;
		}

		uint32_t pbf[3];
		pbf[0] = tr.ea;
		pbf[1] = tr.eb;
		pbf[2] = tr.ec;		
		for (int32_t p = 0; p < 3; ++p)
		{
			int32_t pnx = (p + 1) % 3;
			int32_t opp = (p + 2) % 3;

			if ((pbf[p] == cEdge.s && pbf[pnx] == cEdge.e) || (pbf[p] == cEdge.e && pbf[pnx] == cEdge.s))
			{
				if (nv == NOT_VALID_VERTEX)
				{
					middle.p = (mVertices[pbf[p]].p + mVertices[pbf[pnx]].p) * 0.5f;
					middle.n = (mVertices[pbf[p]].n + mVertices[pbf[pnx]].n) * 0.5f;
					middle.uv[0] = (mVertices[pbf[p]].uv[0] + mVertices[pbf[pnx]].uv[0]) * 0.5f;

					nv = (uint32_t)mVertices.size();
					mVertices.push_back(middle);
				}
				if (nv < mRestrictionFlag.size())
				{
					mRestrictionFlag[nv] = ((snapRestriction == EXTERNAL_BORDER_EDGE) || (snapRestriction == INTERNAL_BORDER_EDGE));
				}
				else
				{
					mRestrictionFlag.push_back((snapRestriction == EXTERNAL_BORDER_EDGE) || (snapRestriction == INTERNAL_BORDER_EDGE));
				}
	
				uint32_t ind1 = addEdgeTr(Edge(pbf[p], nv));
				uint32_t ind2 = addEdgeTr(Edge(nv, pbf[pnx]));
				uint32_t ind3 = addEdgeTr(Edge(nv, pbf[opp]));

		
				mEdgeFlag[ind1] = snapRestriction;
				mEdgeFlag[ind2] = snapRestriction;
				mEdgeFlag[ind3] = INTERNAL_EDGE;
				
				mTrMeshEdToTr[ind1].add(mTrMeshEdToTr[id].tr[t]);
				int32_t userInfo = mTesselatedMeshTriangles[mTrMeshEdToTr[id].tr[t]].userInfo;
				mTesselatedMeshTriangles[mTrMeshEdToTr[id].tr[t]] = TriangleIndexed(pbf[p], nv, pbf[opp]);
				mTesselatedMeshTriangles[mTrMeshEdToTr[id].tr[t]].userInfo = userInfo;
				mTrMeshEdToTr[ind2].add((int32_t)mTesselatedMeshTriangles.size());
				mTrMeshEdToTr[ind3].add((int32_t)mTrMeshEdToTr[id].tr[t]);
				mTrMeshEdToTr[ind3].add((int32_t)mTesselatedMeshTriangles.size());
				mTesselatedMeshTriangles.push_back(TriangleIndexed(nv,pbf[pnx], pbf[opp]));
				mTesselatedMeshTriangles.back().userInfo = userInfo;
				int32_t ed1 = findEdge(Edge(pbf[pnx], pbf[opp]));
				mTrMeshEdToTr[ed1].replace(oldTriangleIndex, (int32_t)mTesselatedMeshTriangles.size() - 1);
				break;
			}
		}
	}
}


NV_FORCE_INLINE void markEdge(int32_t ui, int32_t ed, std::vector<ChunkPostProcessor::EdgeFlag>& shortMarkup, std::vector<int32_t>& lastOwner)
{
	if (shortMarkup[ed] == ChunkPostProcessor::NONE)
	{
		if (ui == 0)
		{
			shortMarkup[ed] = ChunkPostProcessor::EXTERNAL_EDGE;
		}
		else
		{
			shortMarkup[ed] = ChunkPostProcessor::INTERNAL_EDGE;
		}
		lastOwner[ed] = ui;
	}
	else
	{
		if (ui != 0)
		{
			if (shortMarkup[ed] == ChunkPostProcessor::EXTERNAL_EDGE)
			{
				shortMarkup[ed] = ChunkPostProcessor::EXTERNAL_BORDER_EDGE;
			}
			if ((shortMarkup[ed] == ChunkPostProcessor::INTERNAL_EDGE) && ui != lastOwner[ed])
			{
				shortMarkup[ed] = ChunkPostProcessor::INTERNAL_BORDER_EDGE;
			}
		}
		else
		{
			if (shortMarkup[ed] != ChunkPostProcessor::EXTERNAL_EDGE)
			{
				shortMarkup[ed] = ChunkPostProcessor::EXTERNAL_BORDER_EDGE;
			}
		}
	}
}

float falloffFunction(float x, float mx)
{
	float t = (x) / (mx + 1e-6f);
	t = std::min(1.0f, t);
	return t * t;
}

void ChunkPostProcessor::recalcNoiseDirs()
{
	/**
		Compute normals direction to apply noise
	*/
	mVerticesNormalsSmoothed.resize(mVertices.size(), PxVec3(0, 0, 0));
	for (uint32_t i = 0; i < mTesselatedMeshTriangles.size(); ++i)
	{
		if (mTesselatedMeshTriangles[i].ea == NOT_VALID_VERTEX)
		{
			continue;
		}
		TriangleIndexed& tr = mTesselatedMeshTriangles[i];
		if (tr.userInfo == 0) continue;
			
		if (tr.userInfo < 0)
			mVerticesNormalsSmoothed[mPositionMappedVrt[tr.ea]] += mVertices[tr.ea].n.getNormalized();
		else
			mVerticesNormalsSmoothed[mPositionMappedVrt[tr.ea]] -= mVertices[tr.ea].n.getNormalized();

		if (tr.userInfo < 0)
			mVerticesNormalsSmoothed[mPositionMappedVrt[tr.eb]] += mVertices[tr.eb].n.getNormalized();
		else
			mVerticesNormalsSmoothed[mPositionMappedVrt[tr.eb]] -= mVertices[tr.eb].n.getNormalized();

		if (tr.userInfo < 0)
			mVerticesNormalsSmoothed[mPositionMappedVrt[tr.ec]] += mVertices[tr.ec].n.getNormalized();
		else
			mVerticesNormalsSmoothed[mPositionMappedVrt[tr.ec]] -= mVertices[tr.ec].n.getNormalized();

	}
	for (uint32_t i = 0; i < mVerticesNormalsSmoothed.size(); ++i)
	{

		mVerticesNormalsSmoothed[i] = mVerticesNormalsSmoothed[mPositionMappedVrt[i]];
		mVerticesNormalsSmoothed[i].normalize();
	}
}



void ChunkPostProcessor::applyNoise(SimplexNoise& noise, float falloff, int32_t relaxIterations, float relaxFactor)
{
	NVBLAST_ASSERT(isTesselated);
	if (isTesselated == false)
	{
		return;
	}
	mRestrictionFlag.clear();
	mRestrictionFlag.resize(mVertices.size(), false);

	for (uint32_t i = 0; i < mTrMeshEdges.size(); ++i)
	{
		if (mTrMeshEdToTr[i].c != 0)
		{
			if (mEdgeFlag[i] == EXTERNAL_EDGE || mEdgeFlag[i] == EXTERNAL_BORDER_EDGE)
			{
				mRestrictionFlag[mTrMeshEdges[i].e] = true;
				mRestrictionFlag[mTrMeshEdges[i].s] = true;
			}
		}
	}
	std::vector<Vertex> localVertices = mVertices;
	
	recalcNoiseDirs();

	relax(relaxIterations, relaxFactor, localVertices);
	

	/** 
		Apply noise
	*/
	for (uint32_t i = 0; i < localVertices.size(); ++i)
	{

		if (!mRestrictionFlag[i])
		{

			float d = noise.sample(localVertices[i].p);
			localVertices[i].p += (falloffFunction(mVerticesDistances[i], falloff)) * mVerticesNormalsSmoothed[i] * d;
		}
	}


	/* Recalculate smoothed normals*/
	mVerticesNormalsSmoothed.assign(mVerticesNormalsSmoothed.size(), PxVec3(0, 0, 0));
	for (uint32_t i = 0; i < mTesselatedMeshTriangles.size(); ++i)
	{
		if (mTesselatedMeshTriangles[i].ea == NOT_VALID_VERTEX)
		{
			continue;
		}
		TriangleIndexed& tr = mTesselatedMeshTriangles[i];
		if (tr.userInfo == 0) continue;

		Triangle pTr(localVertices[tr.ea], localVertices[tr.eb], localVertices[tr.ec]);
		PxVec3 nrm = pTr.getNormal().getNormalized();

		mVerticesNormalsSmoothed[mPositionMappedVrt[tr.ea]] += nrm;
		mVerticesNormalsSmoothed[mPositionMappedVrt[tr.eb]] += nrm;
		mVerticesNormalsSmoothed[mPositionMappedVrt[tr.ec]] += nrm;
	}
	for (uint32_t i = 0; i < mVerticesNormalsSmoothed.size(); ++i)
	{
		mVerticesNormalsSmoothed[i] = mVerticesNormalsSmoothed[mPositionMappedVrt[i]];
		mVerticesNormalsSmoothed[i].normalize();
	}
	for (uint32_t i = 0; i < mTesselatedMeshTriangles.size(); ++i)
	{
		if (mTesselatedMeshTriangles[i].ea == NOT_VALID_VERTEX)
		{
			continue;
		}
		TriangleIndexed& tr = mTesselatedMeshTriangles[i];
		if (tr.userInfo == 0) continue;

		localVertices[tr.ea].n = mVerticesNormalsSmoothed[mPositionMappedVrt[tr.ea]];
		localVertices[tr.eb].n = mVerticesNormalsSmoothed[mPositionMappedVrt[tr.eb]];
		localVertices[tr.ec].n = mVerticesNormalsSmoothed[mPositionMappedVrt[tr.ec]];
	}

	mTesselatedMeshResultTriangles.clear();
	for (uint32_t i = 0; i < mTesselatedMeshTriangles.size(); ++i)
	{
		if (mTesselatedMeshTriangles[i].ea == NOT_VALID_VERTEX)
		{
			continue;
		}
		mTesselatedMeshResultTriangles.push_back(Triangle(localVertices[mTesselatedMeshTriangles[i].ea], localVertices[mTesselatedMeshTriangles[i].eb], localVertices[mTesselatedMeshTriangles[i].ec]));
		mTesselatedMeshResultTriangles.back().userInfo = mTesselatedMeshTriangles[i].userInfo;
	}


}


void ChunkPostProcessor::computePositionedMapping()
{
	std::map<PxVec3, int32_t, VrtPositionComparator> mPosMap;
	mPositionMappedVrt.clear();
	mPositionMappedVrt.resize(mVertices.size());

	for (uint32_t i = 0; i < mVertices.size(); ++i)
	{
		auto it = mPosMap.find(mVertices[i].p);

		if (it == mPosMap.end())
		{
			mPosMap[mVertices[i].p] = i;
			mPositionMappedVrt[i] = i;
		}
		else
		{
			mPositionMappedVrt[i] = it->second;
		}
	}
}

void ChunkPostProcessor::computeFalloffAndNormals()
{
	// Map newly created vertices according to positions

	computePositionedMapping();

	mGeometryGraph.resize(mVertices.size());
	for (uint32_t i = 0; i < mTrMeshEdges.size(); ++i)
	{
		if (mTrMeshEdToTr[i].c == 0)
		{
			continue;
		}
		int32_t v1 = mPositionMappedVrt[mTrMeshEdges[i].s];
		int32_t v2 = mPositionMappedVrt[mTrMeshEdges[i].e];

		if (std::find(mGeometryGraph[v1].begin(), mGeometryGraph[v1].end(), v2) == mGeometryGraph[v1].end())
			mGeometryGraph[v1].push_back(v2);
		if (std::find(mGeometryGraph[v2].begin(), mGeometryGraph[v2].end(), v1) == mGeometryGraph[v2].end())
			mGeometryGraph[v2].push_back(v1);
	}
	mVerticesDistances.clear();
	mVerticesDistances.resize(mVertices.size(), 10000.0f);

	std::queue<int32_t> que;

	for (uint32_t i = 0; i < mTrMeshEdges.size(); ++i)
	{
		if (mTrMeshEdToTr[i].c != 0 && (mEdgeFlag[i] == EXTERNAL_EDGE || mEdgeFlag[i] == EXTERNAL_BORDER_EDGE))
		{
			int32_t v1 = mPositionMappedVrt[mTrMeshEdges[i].s];
			int32_t v2 = mPositionMappedVrt[mTrMeshEdges[i].e];
			mVerticesDistances[v1] = 0.0f;
			mVerticesDistances[v2] = 0.0f;
			que.push(v1);
			que.push(v2);
		}
	}
	while (!que.empty())
	{
		int32_t curr = que.front();
		que.pop();

		for (uint32_t i = 0; i < mGeometryGraph[curr].size(); ++i)
		{
			int32_t to = mGeometryGraph[curr][i];
			float d = mVerticesDistances[curr] + 0.1f;// (mVertices[to].p - mVertices[curr].p).magnitudeSquared();
			if (d < mVerticesDistances[to])
			{
				mVerticesDistances[to] = d;
				que.push(to);
			}
		}
	}

	for (uint32_t i = 0; i < mVerticesDistances.size(); ++i)
	{
		int32_t from = mPositionMappedVrt[i];
		mVerticesDistances[i] = mVerticesDistances[from]; 
	}
}

bool edgeOverlapTest(PxVec3& as, PxVec3& ae, PxVec3& bs, PxVec3& be)
{
	//return false;
	if (std::max(std::min(as.x, ae.x), std::min(bs.x, be.x)) > std::min(std::max(as.x, ae.x), std::max(bs.x, be.x))) return false;
	if (std::max(std::min(as.y, ae.y), std::min(bs.y, be.y)) > std::min(std::max(as.y, ae.y), std::max(bs.y, be.y))) return false;
	if (std::max(std::min(as.z, ae.z), std::min(bs.z, be.z)) > std::min(std::max(as.z, ae.z), std::max(bs.z, be.z))) return false;

	return ((bs - as).cross(ae - as)).magnitudeSquared() < 1e-12f && ((be - as).cross(ae - as)).magnitudeSquared() < 1e-12f;
}

void ChunkPostProcessor::relax(int32_t iteration, float factor, std::vector<Vertex>& vertices)
{
	std::vector<PxVec3> verticesTemp(vertices.size());
	std::vector<PxVec3> normalsTemp(vertices.size());
	for (int32_t iter = 0; iter < iteration; ++iter)
	{
		for (uint32_t i = 0; i < vertices.size(); ++i)
		{
			if (mRestrictionFlag[i])
			{
				continue;
			}
			PxVec3 cps = vertices[i].p;
			PxVec3 cns = mVerticesNormalsSmoothed[i];
			PxVec3 averaged(0, 0, 0);
			PxVec3 averagedNormal(0, 0, 0);

			for (uint32_t p = 0; p < mGeometryGraph[mPositionMappedVrt[i]].size(); ++p)
			{
				int32_t to = mGeometryGraph[mPositionMappedVrt[i]][p];
				averaged += vertices[to].p;
				averagedNormal += mVerticesNormalsSmoothed[to];

			}
			averaged *= (1.0f / mGeometryGraph[mPositionMappedVrt[i]].size());
			averagedNormal *= (1.0f / mGeometryGraph[mPositionMappedVrt[i]].size());
			verticesTemp[i] = cps + (averaged - cps) * factor;
			normalsTemp[i] = cns * (1.0f - factor) + averagedNormal * factor;
		}
		for (uint32_t i = 0; i < vertices.size(); ++i)
		{
			if (mRestrictionFlag[i])
			{
				continue;
			}
			vertices[i].p = verticesTemp[i];
			mVerticesNormalsSmoothed[i] = normalsTemp[i].getNormalized();

		}
	}

}

void ChunkPostProcessor::prebuildEdgeFlagArray()
{
	mRestrictionFlag.clear();
	mRestrictionFlag.resize(mVertices.size());
	mEdgeFlag.clear();
	mEdgeFlag.resize(mTrMeshEdges.size(), NONE);

	std::map<PxVec3, int32_t, VrtPositionComparator> mPosMap;
	mPositionMappedVrt.clear();
	mPositionMappedVrt.resize(mVertices.size(), 0);

	for (uint32_t i = 0; i < mVertices.size(); ++i)
	{
		auto it = mPosMap.find(mVertices[i].p);

		if (it == mPosMap.end())
		{
			mPosMap[mVertices[i].p] = i;
			mPositionMappedVrt[i] = i;
		}
		else
		{
			mPositionMappedVrt[i] = it->second;
		}
	}

	std::map<Edge, int32_t> mPositionEdgeMap;
	std::vector<int32_t> mPositionBasedEdges(mTrMeshEdges.size());


	for (uint32_t i = 0; i < mTrMeshEdges.size(); ++i)
	{
		Edge tmp = Edge(mPositionMappedVrt[mTrMeshEdges[i].s], mPositionMappedVrt[mTrMeshEdges[i].e]);
		if (tmp.e < tmp.s) std::swap(tmp.e, tmp.s);
		auto it = mPositionEdgeMap.find(tmp);
		if (it == mPositionEdgeMap.end())
		{
			mPositionEdgeMap[tmp] = i;
			mPositionBasedEdges[i] = i;
		}
		else
		{
			mPositionBasedEdges[i] = it->second;
		}
	}

	std::vector<EdgeFlag> shortMarkup(mTrMeshEdges.size(), NONE);
	std::vector<int32_t> lastOwner(mTrMeshEdges.size(), 0);

	std::vector<std::vector<int32_t> > edgeOverlap(mTrMeshEdges.size());
	for (auto it1 = mPositionEdgeMap.begin(); it1 != mPositionEdgeMap.end(); ++it1)
	{
		auto it2 = it1;
		it2++;
		for (; it2 != mPositionEdgeMap.end(); ++it2)
		{
			Edge& ed1 = mTrMeshEdges[it1->second];
			Edge& ed2 = mTrMeshEdges[it2->second];
			
			if (edgeOverlapTest(mVertices[ed1.s].p, mVertices[ed1.e].p, mVertices[ed2.s].p, mVertices[ed2.e].p))
			{
				edgeOverlap[it1->second].push_back(it2->second);
			}
		}
	}

	for (uint32_t i = 0; i < mTesselatedMeshTriangles.size(); ++i)
	{
		int32_t ui = mTesselatedMeshTriangles[i].userInfo;
		int32_t ed = mPositionBasedEdges[findEdge(Edge(mTesselatedMeshTriangles[i].ea, mTesselatedMeshTriangles[i].eb))];

		
		markEdge(ui, ed, shortMarkup, lastOwner);
		for (uint32_t ov = 0; ov < edgeOverlap[ed].size(); ++ov)
		{
			markEdge(ui, edgeOverlap[ed][ov], shortMarkup, lastOwner);
		}

		ed = mPositionBasedEdges[findEdge(Edge(mTesselatedMeshTriangles[i].ea, mTesselatedMeshTriangles[i].ec))];
		markEdge(ui, ed, shortMarkup, lastOwner);
		for (uint32_t ov = 0; ov < edgeOverlap[ed].size(); ++ov)
		{
			markEdge(ui, edgeOverlap[ed][ov], shortMarkup, lastOwner);
		}

		ed = mPositionBasedEdges[findEdge(Edge(mTesselatedMeshTriangles[i].eb, mTesselatedMeshTriangles[i].ec))];
		markEdge(ui, ed, shortMarkup, lastOwner);
		for (uint32_t ov = 0; ov < edgeOverlap[ed].size(); ++ov)
		{
			markEdge(ui, edgeOverlap[ed][ov], shortMarkup, lastOwner);
		}

	}

	for (uint32_t i = 0; i < mTrMeshEdges.size(); ++i)
	{
		mEdgeFlag[i] = shortMarkup[mPositionBasedEdges[i]];
	}

	for (uint32_t i = 0; i < mTesselatedMeshTriangles.size(); ++i)
	{
		if (mTesselatedMeshTriangles[i].userInfo != 0) continue;

		int32_t ed = findEdge(Edge(mTesselatedMeshTriangles[i].ea, mTesselatedMeshTriangles[i].eb));
		mEdgeFlag[ed] = EXTERNAL_EDGE;
		ed = findEdge(Edge(mTesselatedMeshTriangles[i].ec, mTesselatedMeshTriangles[i].eb));
		mEdgeFlag[ed] = EXTERNAL_EDGE;
		ed = findEdge(Edge(mTesselatedMeshTriangles[i].ea, mTesselatedMeshTriangles[i].ec));
		mEdgeFlag[ed] = EXTERNAL_EDGE;
	}
}



void ChunkPostProcessor::tesselateInternalSurface(float maxLenIn)
{
	mTesselatedMeshTriangles = mBaseMeshTriangles;
	if (mTesselatedMeshTriangles.empty())
	{
		return;
	}

	updateEdgeTriangleInfo();
	prebuildEdgeFlagArray();
	mRestrictionFlag.resize(mVertices.size(), 0);
	for (uint32_t i = 0; i < mTrMeshEdges.size(); ++i)
	{
		if (mEdgeFlag[i] == EXTERNAL_EDGE || mEdgeFlag[i] == EXTERNAL_BORDER_EDGE || mEdgeFlag[i] == INTERNAL_BORDER_EDGE)
		{
			mRestrictionFlag[mTrMeshEdges[i].s] = 1;
			mRestrictionFlag[mTrMeshEdges[i].e] = 1;
		}
	}

	
	float maxLen = std::max(0.1f, maxLenIn);
	while (maxLen > maxLenIn)
	{
		float mlSq = maxLen * maxLen;
		float minD = maxLen * 0.5f;
		minD = minD * minD;
		
		for (int32_t iter = 0; iter < 15; ++iter)
		{
			updateVertEdgeInfo();
			uint32_t oldSize = (uint32_t)mTrMeshEdges.size();
			for (uint32_t i = 0; i < oldSize; ++i)
			{
				if (mEdgeFlag[i] == EXTERNAL_EDGE || mEdgeFlag[i] == INTERNAL_BORDER_EDGE)
				{
					continue;
				}
				if ((mVertices[mTrMeshEdges[i].s].p - mVertices[mTrMeshEdges[i].e].p).magnitudeSquared() < minD)
				{
					collapseEdge(i);
				}
			}

			oldSize = (uint32_t)mTrMeshEdges.size();
			updateEdgeTriangleInfo();
			for (uint32_t i = 0; i < oldSize; ++i)
			{
				if (mEdgeFlag[i] == EXTERNAL_EDGE)
				{
					continue;
				}
				if ((mVertices[mTrMeshEdges[i].s].p - mVertices[mTrMeshEdges[i].e].p).magnitudeSquared() > mlSq)
				{
					divideEdge(i);
				}
			}
		}
		maxLen *= 0.3;
		maxLen = std::max(maxLen, maxLenIn);
	}
	computeFalloffAndNormals();
	prebuildTesselatedTriangles();
	isTesselated = true;
}

} // namespace Blast
} // namespace Nv