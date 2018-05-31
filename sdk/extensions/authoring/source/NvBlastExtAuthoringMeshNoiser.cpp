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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.


// This warning arises when using some stl containers with older versions of VC
// c:\program files (x86)\microsoft visual studio 12.0\vc\include\xtree(1826): warning C4702: unreachable code
#include "NvPreprocessor.h"
#if NV_VC && NV_VC < 14
#pragma warning(disable : 4702)
#endif

#include "NvBlastExtAuthoringMeshNoiser.h"
#include "NvBlastExtAuthoringPerlinNoise.h"
#include <set>
#include <queue>
#include <NvBlastAssert.h>

using namespace Nv::Blast;
using namespace std;


void MeshNoiser::computeFalloffAndNormals()
{
	// Map newly created vertices according to positions

	computePositionedMapping();

	mGeometryGraph.resize(mVertices.size());
	for (uint32_t i = 0; i < mEdges.size(); ++i)
	{
		if (mTrMeshEdToTr[i].c == 0)
		{
			continue;
		}
		int32_t v1 = mPositionMappedVrt[mEdges[i].s];
		int32_t v2 = mPositionMappedVrt[mEdges[i].e];

		if (std::find(mGeometryGraph[v1].begin(), mGeometryGraph[v1].end(), v2) == mGeometryGraph[v1].end())
			mGeometryGraph[v1].push_back(v2);
		if (std::find(mGeometryGraph[v2].begin(), mGeometryGraph[v2].end(), v1) == mGeometryGraph[v2].end())
			mGeometryGraph[v2].push_back(v1);
	}
	mVerticesDistances.clear();
	mVerticesDistances.resize(mVertices.size(), 10000.0f);

	std::queue<int32_t> que;

	for (uint32_t i = 0; i < mEdges.size(); ++i)
	{
		if (mTrMeshEdToTr[i].c != 0 && (mEdgeFlag[i] == EXTERNAL_EDGE || mEdgeFlag[i] == EXTERNAL_BORDER_EDGE))
		{
			int32_t v1 = mPositionMappedVrt[mEdges[i].s];
			int32_t v2 = mPositionMappedVrt[mEdges[i].e];
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

	return ((bs - as).cross(ae - as)).magnitudeSquared() < 1e-6f && ((be - as).cross(ae - as)).magnitudeSquared() < 1e-6f;
}

void MeshNoiser::computePositionedMapping()
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




void MeshNoiser::relax(int32_t iteration, float factor, std::vector<Vertex>& vertices)
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

NV_FORCE_INLINE void markEdge(int32_t ui, int32_t ed, std::vector<MeshNoiser::EdgeFlag>& shortMarkup, std::vector<int32_t>& lastOwner)
{
	if (shortMarkup[ed] == MeshNoiser::NONE)
	{
		if (ui == 0)
		{
			shortMarkup[ed] = MeshNoiser::EXTERNAL_EDGE;
		}
		else
		{
			shortMarkup[ed] = MeshNoiser::INTERNAL_EDGE;
		}
		lastOwner[ed] = ui;
	}
	else
	{
		if (ui != 0)
		{
			if (shortMarkup[ed] == MeshNoiser::EXTERNAL_EDGE)
			{
				shortMarkup[ed] = MeshNoiser::EXTERNAL_BORDER_EDGE;
			}
			if ((shortMarkup[ed] == MeshNoiser::INTERNAL_EDGE) && ui != lastOwner[ed])
			{
				shortMarkup[ed] = MeshNoiser::INTERNAL_BORDER_EDGE;
			}
		}
		else
		{
			if (shortMarkup[ed] != MeshNoiser::EXTERNAL_EDGE)
			{
				shortMarkup[ed] = MeshNoiser::EXTERNAL_BORDER_EDGE;
			}
		}
	}
}

void MeshNoiser::prebuildEdgeFlagArray()
{
	mRestrictionFlag.clear();
	mRestrictionFlag.resize(mVertices.size());
	mEdgeFlag.clear();
	mEdgeFlag.resize(mEdges.size(), NONE);

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
	std::vector<int32_t> mPositionBasedEdges(mEdges.size());


	for (uint32_t i = 0; i < mEdges.size(); ++i)
	{
		Edge tmp = Edge(mPositionMappedVrt[mEdges[i].s], mPositionMappedVrt[mEdges[i].e]);
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

	std::vector<EdgeFlag> shortMarkup(mEdges.size(), NONE);
	std::vector<int32_t> lastOwner(mEdges.size(), 0);

	std::vector<std::vector<int32_t> > edgeOverlap(mEdges.size());
	for (auto it1 = mPositionEdgeMap.begin(); it1 != mPositionEdgeMap.end(); ++it1)
	{
		auto it2 = it1;
		it2++;
		for (; it2 != mPositionEdgeMap.end(); ++it2)
		{
			Edge& ed1 = mEdges[it1->second];
			Edge& ed2 = mEdges[it2->second];
			
			if (edgeOverlapTest(mVertices[ed1.s].p, mVertices[ed1.e].p, mVertices[ed2.s].p, mVertices[ed2.e].p))
			{
				edgeOverlap[it1->second].push_back(it2->second);
			}
		}
	}

	for (uint32_t i = 0; i < mTriangles.size(); ++i)
	{
		int32_t ui = mTriangles[i].userData;
		int32_t ed = mPositionBasedEdges[findEdge(Edge(mTriangles[i].ea, mTriangles[i].eb))];

		
		markEdge(ui, ed, shortMarkup, lastOwner);
		for (uint32_t ov = 0; ov < edgeOverlap[ed].size(); ++ov)
		{
			markEdge(ui, edgeOverlap[ed][ov], shortMarkup, lastOwner);
		}

		ed = mPositionBasedEdges[findEdge(Edge(mTriangles[i].ea, mTriangles[i].ec))];
		markEdge(ui, ed, shortMarkup, lastOwner);
		for (uint32_t ov = 0; ov < edgeOverlap[ed].size(); ++ov)
		{
			markEdge(ui, edgeOverlap[ed][ov], shortMarkup, lastOwner);
		}

		ed = mPositionBasedEdges[findEdge(Edge(mTriangles[i].eb, mTriangles[i].ec))];
		markEdge(ui, ed, shortMarkup, lastOwner);
		for (uint32_t ov = 0; ov < edgeOverlap[ed].size(); ++ov)
		{
			markEdge(ui, edgeOverlap[ed][ov], shortMarkup, lastOwner);
		}

	}

	for (uint32_t i = 0; i < mEdges.size(); ++i)
	{
		mEdgeFlag[i] = shortMarkup[mPositionBasedEdges[i]];
	}

	for (uint32_t i = 0; i < mTriangles.size(); ++i)
	{
		if (mTriangles[i].userData != 0) continue;

		int32_t ed = findEdge(Edge(mTriangles[i].ea, mTriangles[i].eb));
		mEdgeFlag[ed] = EXTERNAL_EDGE;
		ed = findEdge(Edge(mTriangles[i].ec, mTriangles[i].eb));
		mEdgeFlag[ed] = EXTERNAL_EDGE;
		ed = findEdge(Edge(mTriangles[i].ea, mTriangles[i].ec));
		mEdgeFlag[ed] = EXTERNAL_EDGE;
	}
}




NV_FORCE_INLINE int32_t MeshNoiser::addVerticeIfNotExist(const Vertex& p)
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

NV_FORCE_INLINE int32_t MeshNoiser::addEdge(const Edge& e)
{
	Edge ed = e;
	if (ed.e < ed.s) std::swap(ed.s, ed.e);
	auto it = mEdgeMap.find(ed);
	if (it == mEdgeMap.end())
	{
		mTrMeshEdToTr.push_back(EdgeToTriangles());
		mEdgeMap[ed] = (int)mEdgeMap.size();
		mEdges.push_back(ed);
		mEdgeFlag.push_back(INTERNAL_EDGE);
		return (int32_t)mEdges.size() - 1;
	}
	else
	{
		return it->second;
	}
}

NV_FORCE_INLINE int32_t MeshNoiser::findEdge(const Edge& e)
{
	Edge ed = e;
	if (ed.e < ed.s) std::swap(ed.s, ed.e);
	auto it = mEdgeMap.find(ed);
	if (it == mEdgeMap.end())
	{
		return -1;
	}
	else
	{
		return it->second;
	}
}


/**
	Weld input vertices, build edge and triangle buffers
*/
void MeshNoiser::setMesh(const vector<Triangle>& mesh)
{
	uint32_t a, b, c;
	PxBounds3 box;
	box.setEmpty();
	for (uint32_t i = 0; i < mesh.size(); ++i)
	{
		const Triangle& tr = mesh[i];
		a = addVerticeIfNotExist(tr.a);
		b = addVerticeIfNotExist(tr.b);
		c = addVerticeIfNotExist(tr.c);
		box.include(tr.a.p);
		box.include(tr.b.p);
		box.include(tr.c.p);
		addEdge(Edge(a, b));
		addEdge(Edge(b, c));
		addEdge(Edge(a, c));
		mTriangles.push_back(TriangleIndexed(a, b, c));
		mTriangles.back().userData = tr.userData;
		mTriangles.back().materialId = tr.materialId;
		mTriangles.back().smoothingGroup = tr.smoothingGroup;

	}
	mOffset = box.getCenter();
	mScale = max(box.getExtents(0), max(box.getExtents(1), box.getExtents(2)));
	float invScale = 1.0f / mScale;
	for (uint32_t i = 0; i < mVertices.size(); ++i)
	{
		mVertices[i].p = mVertices[i].p - box.getCenter();
		mVertices[i].p *= invScale;
	}
}


void MeshNoiser::tesselateInternalSurface(float maxLenIn)
{
	if (mTriangles.empty())
	{
		return;
	}

	updateEdgeTriangleInfo();
	prebuildEdgeFlagArray();
	mRestrictionFlag.resize(mVertices.size(), 0);
	for (uint32_t i = 0; i < mEdges.size(); ++i)
	{
		if (mEdgeFlag[i] == EXTERNAL_EDGE || mEdgeFlag[i] == EXTERNAL_BORDER_EDGE || mEdgeFlag[i] == INTERNAL_BORDER_EDGE)
		{
			mRestrictionFlag[mEdges[i].s] = 1;
			mRestrictionFlag[mEdges[i].e] = 1;
		}
	}

	
	float maxLen = maxLenIn; 
	float mlSq = maxLen * maxLen;
	float minD = maxLen * 0.5f;
		minD = minD * minD;
		
		for (int32_t iter = 0; iter < 15; ++iter)
		{
			updateVertEdgeInfo();
			uint32_t oldSize = (uint32_t)mEdges.size();
			for (uint32_t i = 0; i < oldSize; ++i)
			{
				if (mEdgeFlag[i] == EXTERNAL_EDGE || mEdgeFlag[i] == INTERNAL_BORDER_EDGE)
				{
					continue;
				}
				if ((mVertices[mEdges[i].s].p - mVertices[mEdges[i].e].p).magnitudeSquared() < minD)
				{
					collapseEdge(i);
				}
			}
			oldSize = (uint32_t)mEdges.size();
			updateEdgeTriangleInfo();
			for (uint32_t i = 0; i < oldSize; ++i)
			{
				if (mEdgeFlag[i] == EXTERNAL_EDGE)
				{
					continue;
				}
				if ((mVertices[mEdges[i].s].p - mVertices[mEdges[i].e].p).magnitudeSquared() > mlSq)
				{
					divideEdge(i);
				}
			}
		}
	computeFalloffAndNormals();
	prebuildTesselatedTriangles();
	isTesselated = true;
}

void MeshNoiser::updateEdgeTriangleInfo()
{
	mTrMeshEdToTr.clear();
	mTrMeshEdToTr.resize(mEdges.size());
	for (uint32_t i = 0; i < mTriangles.size(); ++i)
	{
		TriangleIndexed& tr = mTriangles[i];
		if (tr.ea == NOT_VALID_VERTEX)
			continue;
		int32_t ed = addEdge(Edge(tr.ea, tr.eb));
		mTrMeshEdToTr[ed].add(i);
		ed = addEdge(Edge(tr.ea, tr.ec));
		mTrMeshEdToTr[ed].add(i);
		ed = addEdge(Edge(tr.ec, tr.eb));
		mTrMeshEdToTr[ed].add(i);
	}
}

void MeshNoiser::updateVertEdgeInfo()
{
	mVertexToTriangleMap.clear();
	mVertexToTriangleMap.resize(mVertices.size());
	for (uint32_t i = 0; i < mTriangles.size(); ++i)
	{
		TriangleIndexed& tr = mTriangles[i];
		if (tr.ea == NOT_VALID_VERTEX) continue;
		mVertexToTriangleMap[tr.ea].push_back(i);
		mVertexToTriangleMap[tr.eb].push_back(i);
		mVertexToTriangleMap[tr.ec].push_back(i);
	}
	mVertexValence.clear();
	mVertexValence.resize(mVertices.size(), 0);

	for (uint32_t i = 0; i < mEdges.size(); ++i)
	{
		if (mTrMeshEdToTr[i].c != 0)
		{
			mVertexValence[mEdges[i].s]++;
			mVertexValence[mEdges[i].e]++;
		}
	}
}


void MeshNoiser::collapseEdge(int32_t id)
{
	Edge cEdge = mEdges[id];
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
		if (mTriangles[mVertexToTriangleMap[from][i]].ea == NOT_VALID_VERTEX)
			continue;
		if (neighboorTriangles.insert(mVertexToTriangleMap[from][i]).second && mTriangles[mVertexToTriangleMap[from][i]].isContainEdge(from, to))
		{
			trWithEdge[cntr] = mVertexToTriangleMap[from][i];
			cntr++;
		}
	}
	for (uint32_t i = 0; i < mVertexToTriangleMap[to].size(); ++i)
	{
		if (mTriangles[mVertexToTriangleMap[to][i]].ea == NOT_VALID_VERTEX)
			continue;
		if (neighboorTriangles.insert(mVertexToTriangleMap[to][i]).second && mTriangles[mVertexToTriangleMap[to][i]].isContainEdge(from, to))
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
		if (mTriangles[i].ea == from || mTriangles[i].eb == from || mTriangles[i].ec == from)
		{
			if (mTriangles[i].ea != to && mTriangles[i].ea != from)
				connectedToBegin.insert(mTriangles[i].ea);
			if (mTriangles[i].eb != to && mTriangles[i].eb != from)
				connectedToBegin.insert(mTriangles[i].eb);
			if (mTriangles[i].ec != to && mTriangles[i].ec != from)
				connectedToBegin.insert(mTriangles[i].ec);
		}

		if (mTriangles[i].ea == to || mTriangles[i].eb == to || mTriangles[i].ec == to)
		{
			if (mTriangles[i].ea != to && mTriangles[i].ea != from)
				connectedToEnd.insert(mTriangles[i].ea);
			if (mTriangles[i].eb != to && mTriangles[i].eb != from)
				connectedToEnd.insert(mTriangles[i].eb);
			if (mTriangles[i].ec != to && mTriangles[i].ec != from)
				connectedToEnd.insert(mTriangles[i].ec);
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
			if ((mTriangles[tr].ea == from || mTriangles[tr].eb == from || mTriangles[tr].ec == from) &&
				(mTriangles[tr].ea == to || mTriangles[tr].eb == to || mTriangles[tr].ec == to) &&
				(mTriangles[tr].ea == currV || mTriangles[tr].eb == currV || mTriangles[tr].ec == currV))
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
			TriangleIndexed tr = mTriangles[i];
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
		mTriangles[trWithEdge[0]].ea = NOT_VALID_VERTEX;
		if (cntr == 2)mTriangles[trWithEdge[1]].ea = NOT_VALID_VERTEX;

		for (int32_t i : neighboorTriangles)
		{
			if (mTriangles[i].ea == NOT_VALID_VERTEX)
				continue;
			if (mTriangles[i].ea == from)
			{
				mTriangles[i].ea = to;
				mVertexToTriangleMap[from].clear();
				mVertexToTriangleMap[to].push_back(i);
			}
			else
			if (mTriangles[i].eb == from)
			{
				mTriangles[i].eb = to;
				mVertexToTriangleMap[from].clear();
				mVertexToTriangleMap[to].push_back(i);
			}
			else
			if (mTriangles[i].ec == from)
			{
				mTriangles[i].ec = to;
				mVertexToTriangleMap[from].clear();
				mVertexToTriangleMap[to].push_back(i);
			}
		}
	}
}


void MeshNoiser::divideEdge(int32_t id)
{

	if (mTrMeshEdToTr[id].c == 0 )
	{
		return;
	}

	Edge cEdge = mEdges[id];
	EdgeFlag snapRestriction = mEdgeFlag[id];
	Vertex middle;
	uint32_t nv = NOT_VALID_VERTEX;
	for (int32_t t = 0; t < mTrMeshEdToTr[id].c; ++t)
	{
		int32_t oldTriangleIndex = mTrMeshEdToTr[id].tr[t];
		TriangleIndexed tr = mTriangles[mTrMeshEdToTr[id].tr[t]];

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
	
				uint32_t ind1 = addEdge(Edge(pbf[p], nv));
				uint32_t ind2 = addEdge(Edge(nv, pbf[pnx]));
				uint32_t ind3 = addEdge(Edge(nv, pbf[opp]));

		
				mEdgeFlag[ind1] = snapRestriction;
				mEdgeFlag[ind2] = snapRestriction;
				mEdgeFlag[ind3] = INTERNAL_EDGE;
				
				mTrMeshEdToTr[ind1].add(mTrMeshEdToTr[id].tr[t]);
				int32_t userInfo = mTriangles[mTrMeshEdToTr[id].tr[t]].userData;
				int32_t matId = mTriangles[mTrMeshEdToTr[id].tr[t]].materialId;
				int32_t smId = mTriangles[mTrMeshEdToTr[id].tr[t]].smoothingGroup;
				mTriangles[mTrMeshEdToTr[id].tr[t]] = TriangleIndexed(pbf[p], nv, pbf[opp]);
				mTriangles[mTrMeshEdToTr[id].tr[t]].userData = userInfo;
				mTriangles[mTrMeshEdToTr[id].tr[t]].materialId = matId;
				mTriangles[mTrMeshEdToTr[id].tr[t]].smoothingGroup = smId;

				mTrMeshEdToTr[ind2].add((int32_t)mTriangles.size());
				mTrMeshEdToTr[ind3].add((int32_t)mTrMeshEdToTr[id].tr[t]);
				mTrMeshEdToTr[ind3].add((int32_t)mTriangles.size());
				mTriangles.push_back(TriangleIndexed(nv,pbf[pnx], pbf[opp]));
				mTriangles.back().userData = userInfo;
				mTriangles.back().materialId = matId;
				mTriangles.back().smoothingGroup = smId;

				int32_t ed1 = findEdge(Edge(pbf[pnx], pbf[opp]));
				mTrMeshEdToTr[ed1].replace(oldTriangleIndex, (int32_t)mTriangles.size() - 1);
				break;
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

void MeshNoiser::recalcNoiseDirs()
{
	/**
		Compute normals direction to apply noise
	*/
	mVerticesNormalsSmoothed.resize(mVertices.size(), PxVec3(0, 0, 0));
	for (uint32_t i = 0; i < mTriangles.size(); ++i)
	{
		if (mTriangles[i].ea == NOT_VALID_VERTEX)
		{
			continue;
		}
		TriangleIndexed& tr = mTriangles[i];
		if (tr.userData == 0) continue;
			
		if (tr.userData < 0)
			mVerticesNormalsSmoothed[mPositionMappedVrt[tr.ea]] += mVertices[tr.ea].n.getNormalized();
		else
			mVerticesNormalsSmoothed[mPositionMappedVrt[tr.ea]] -= mVertices[tr.ea].n.getNormalized();

		if (tr.userData < 0)
			mVerticesNormalsSmoothed[mPositionMappedVrt[tr.eb]] += mVertices[tr.eb].n.getNormalized();
		else
			mVerticesNormalsSmoothed[mPositionMappedVrt[tr.eb]] -= mVertices[tr.eb].n.getNormalized();

		if (tr.userData < 0)
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



void MeshNoiser::applyNoise(SimplexNoise& noise, float falloff, int32_t /*relaxIterations*/, float /*relaxFactor*/)
{
	NVBLAST_ASSERT(isTesselated);
	if (isTesselated == false)
	{
		return;
	}
	mRestrictionFlag.clear();
	mRestrictionFlag.resize(mVertices.size(), false);

	for (uint32_t i = 0; i < mEdges.size(); ++i)
	{
		if (mTrMeshEdToTr[i].c != 0)
		{
			if (mEdgeFlag[i] == EXTERNAL_EDGE || mEdgeFlag[i] == EXTERNAL_BORDER_EDGE)
			{
				mRestrictionFlag[mEdges[i].e] = true;
				mRestrictionFlag[mEdges[i].s] = true;
			}
		}
	}
	std::vector<Vertex> localVertices = mVertices;
	
	recalcNoiseDirs();

	//relax(relaxIterations, relaxFactor, localVertices);
	

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
	for (uint32_t i = 0; i < mTriangles.size(); ++i)
	{
		if (mTriangles[i].ea == NOT_VALID_VERTEX)
		{
			continue;
		}
		TriangleIndexed& tr = mTriangles[i];
		if (tr.userData == 0) continue;

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
	for (uint32_t i = 0; i < mTriangles.size(); ++i)
	{
		if (mTriangles[i].ea == NOT_VALID_VERTEX)
		{
			continue;
		}
		TriangleIndexed& tr = mTriangles[i];
		if (tr.userData == 0) continue;

		localVertices[tr.ea].n = mVerticesNormalsSmoothed[mPositionMappedVrt[tr.ea]];
		localVertices[tr.eb].n = mVerticesNormalsSmoothed[mPositionMappedVrt[tr.eb]];
		localVertices[tr.ec].n = mVerticesNormalsSmoothed[mPositionMappedVrt[tr.ec]];
	}

	mResultTriangles.clear();
	for (uint32_t i = 0; i < mTriangles.size(); ++i)
	{
		if (mTriangles[i].ea == NOT_VALID_VERTEX)
		{
			continue;
		}
		mResultTriangles.push_back(Triangle(localVertices[mTriangles[i].ea], localVertices[mTriangles[i].eb], localVertices[mTriangles[i].ec]));
		mResultTriangles.back().userData = mTriangles[i].userData;
		mResultTriangles.back().materialId = mTriangles[i].materialId;
		mResultTriangles.back().smoothingGroup = mTriangles[i].smoothingGroup;

	}
}


void MeshNoiser::prebuildTesselatedTriangles()
{
	mResultTriangles.clear();

	for (uint32_t i = 0; i < mVertices.size(); ++i)
	{
		mVertices[i].p = mVertices[i].p * mScale + mOffset;
	}

	for (uint32_t i = 0; i < mTriangles.size(); ++i)
	{
		if (mTriangles[i].ea == NOT_VALID_VERTEX)
		{
			continue;
		}
		mResultTriangles.push_back(Triangle(mVertices[mTriangles[i].ea], mVertices[mTriangles[i].eb], mVertices[mTriangles[i].ec]));
		mResultTriangles.back().userData = mTriangles[i].userData;
		mResultTriangles.back().materialId = mTriangles[i].materialId;
		mResultTriangles.back().smoothingGroup = mTriangles[i].smoothingGroup;

	}

}


std::vector<Triangle> MeshNoiser::getMesh()
{
	return mResultTriangles;
}


void MeshNoiser::reset()
{
	mVertices.clear();
	mTriangles.clear();
	mEdges.clear();
	mVertMap.clear();
	mEdgeMap.clear();
    mResultTriangles.clear();
	mRestrictionFlag.clear();
	mEdgeFlag.clear();
	mTrMeshEdToTr.clear();
	mVertexValence.clear();
	mVertexToTriangleMap.clear();

	mVerticesDistances.clear();
	mVerticesNormalsSmoothed.clear();
	mPositionMappedVrt.clear();
	mGeometryGraph.clear();

	isTesselated = false;
	mOffset = PxVec3(0, 0, 0);
	mScale = 1.0f;
}