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


#include "NvBlastExtAuthoringAccelerator.h"
#include "NvBlastExtAuthoringMesh.h"
#include "NvBlastExtAuthoringInternalCommon.h"


using namespace physx;


namespace Nv
{
namespace Blast
{

DummyAccelerator::DummyAccelerator(int32_t count) :count(count)
{
	current = 0;
}
void DummyAccelerator::setState(const Vertex* pos, const Edge* ed, const Facet& fc)
{
	current = 0;
	NV_UNUSED(pos);
	NV_UNUSED(ed);
	NV_UNUSED(fc);
}
void DummyAccelerator::setState(const physx::PxVec3& point) {
	current = 0;
	NV_UNUSED(point);
}
int32_t DummyAccelerator::getNextFacet()
{
	if (current < count)
	{
		++current;
		return current - 1;
	}
	else
		return -1;
}



BBoxBasedAccelerator::BBoxBasedAccelerator(const Mesh* mesh, int32_t resolution) : mResolution(resolution), alreadyGotValue(1)
{
	mBounds = mesh->getBoundingBox();
	mSpatialMap.resize(resolution * resolution * resolution);
	mCells.resize(resolution * resolution * resolution);
	int32_t currentCell = 0;
	PxVec3 incr = (mBounds.maximum - mBounds.minimum) * (1.0f / mResolution);
	for (int32_t z = 0; z < resolution; ++z)
	{
		for (int32_t y = 0; y < resolution; ++y)
		{
			for (int32_t x = 0; x < resolution; ++x)
			{
				mCells[currentCell].minimum.x = mBounds.minimum.x + x * incr.x;
				mCells[currentCell].minimum.y = mBounds.minimum.y + y * incr.y;
				mCells[currentCell].minimum.z = mBounds.minimum.z + z * incr.z;

				mCells[currentCell].maximum.x = mBounds.minimum.x + (x + 1) * incr.x;
				mCells[currentCell].maximum.y = mBounds.minimum.y + (y + 1) * incr.y;
				mCells[currentCell].maximum.z = mBounds.minimum.z + (z + 1) * incr.z;

				++currentCell;
			}
		}
	}

	buildAccelStructure(mesh->getVertices(), mesh->getEdges(), mesh->getFacetsBuffer(), mesh->getFacetCount());
}


BBoxBasedAccelerator::~BBoxBasedAccelerator()
{
	mResolution = 0;
	mBounds.setEmpty();
	mSpatialMap.clear();
	mCells.clear();
}

int32_t BBoxBasedAccelerator::getNextFacet()
{
	int32_t facetId = -1;

	while (mIteratorCell != -1)
	{
		if (mIteratorFacet >= (int32_t)mSpatialMap[mIteratorCell].size())
		{
			if (!cellList.empty())
			{
				mIteratorCell = cellList.back();
				cellList.pop_back();
				mIteratorFacet = 0;
			}
			else
			{
				mIteratorCell = -1;
				break;
			}
		}
		if (alreadyGotFlag[mSpatialMap[mIteratorCell][mIteratorFacet]] != alreadyGotValue)
		{
			facetId = mSpatialMap[mIteratorCell][mIteratorFacet];
			mIteratorFacet++;
			break;
		}
		else
		{
			mIteratorFacet++;
		}
	}
	if (facetId != -1)
	{
		alreadyGotFlag[facetId] = alreadyGotValue;
	}
	return facetId;
}
void BBoxBasedAccelerator::setState(const Vertex* pos, const Edge* ed, const Facet& fc)
{
	alreadyGotValue++;
	mIteratorCell = -1;
	mIteratorFacet = -1;
	cellList.clear();
	facetBox.setEmpty();
	const Edge* edge = ed + fc.firstEdgeNumber;
	uint32_t count = fc.edgesCount;
	for (uint32_t ec = 0; ec < count; ++ec)
	{
		facetBox.include(pos[edge->s].p);
		facetBox.include(pos[edge->e].p);
		edge++;
	}
	for (uint32_t i = 0; i < mCells.size(); ++i)
	{
		if (testCellPolygonIntersection(i, facetBox))
		{
			if (!mSpatialMap[i].empty())
				cellList.push_back(i);
		}
	}
	if (!cellList.empty())
	{
		mIteratorFacet = 0;
		mIteratorCell = cellList.back();
		cellList.pop_back();
	}
}


void BBoxBasedAccelerator::setState(const PxVec3& p)
{
	alreadyGotValue++;
	mIteratorCell = -1;
	mIteratorFacet = -1;
	cellList.clear();
	int32_t perSlice = mResolution * mResolution;
	for (uint32_t i = 0; i < mCells.size(); ++i)
	{
		if (mCells[i].contains(p))
		{
			int32_t xyCellId = i % perSlice;
			for (int32_t zCell = 0; zCell < mResolution; ++zCell)
			{
				int32_t cell = zCell * perSlice + xyCellId;
				if (!mSpatialMap[cell].empty())
					cellList.push_back(cell);
			}
		}
	}
	if (!cellList.empty())
	{
		mIteratorFacet = 0;
		mIteratorCell = cellList.back();
		cellList.pop_back();
	}
}


bool BBoxBasedAccelerator::testCellPolygonIntersection(int32_t cellId, PxBounds3& facetBB)
{
	if (weakBoundingBoxIntersection(mCells[cellId], facetBB))
	{
		return true;
	}
	else
		return false;
}

void BBoxBasedAccelerator::buildAccelStructure(const Vertex* pos, const Edge* edges, const Facet* fc, int32_t facetCount)
{
	for (int32_t facet = 0; facet < facetCount; ++facet)
	{
		PxBounds3 bBox;
		bBox.setEmpty();
		const Edge* edge = &edges[0] + fc->firstEdgeNumber;
		int32_t count = fc->edgesCount;
		for (int32_t ec = 0; ec < count; ++ec)
		{
			bBox.include(pos[edge->s].p);
			bBox.include(pos[edge->e].p);
			edge++;
		}

		for (uint32_t i = 0; i < mCells.size(); ++i)
		{
			if (testCellPolygonIntersection(i, bBox))
			{
				mSpatialMap[i].push_back(facet);
			}
		}
		fc++;
	}
	alreadyGotFlag.resize(facetCount, 0);
	cellList.resize(mCells.size());
}

int32_t testEdgeAgainstCube(PxVec3& p1, PxVec3& p2)
{
	PxVec3 vec = p2 - p1;
	PxVec3 vecSigns;
	for (int32_t i = 0; i < 3; ++i)
	{
		vecSigns[i] = (vec[i] < 0) ? -1 : 1;
	}
	for (int32_t i = 0; i < 3; ++i)
	{
		if (p1[i] * vecSigns[i] > 0.5f) return 0;
		if (p2[i] * vecSigns[i] < -0.5f) return 0;
	}

	for (int32_t i = 0; i < 3; ++i)
	{
		int32_t ip1 = (i + 1) % 3;
		int32_t ip2 = (i + 2) % 3;

		float vl1 = vec[ip2] * p1[ip1] - vec[ip1] * p1[ip2];
		float vl2 = 0.5f * (vec[ip2] * vecSigns[ip1] + vec[ip1] * vecSigns[ip2]);
		if (vl1 * vl1 > vl2 * vl2)
		{
			return 0;
		}
	}
	return 1;
}

NV_INLINE int32_t isInSegm(float a, float b, float c)
{
	return (b >= c) - (a >= c);
}

NV_INLINE int32_t edgeIsAbovePoint(PxVec2& p1, PxVec2& p2, PxVec2& p)
{
	int32_t direction = isInSegm(p1.x, p2.x, p.x);
	if (direction != 0)
	{
		if (isInSegm(p1.y, p2.y, p.y))
		{
			if (direction * (p.x - p1.x) * (p2.y - p1.y) >= direction * (p.y - p1.y) * (p2.x - p1.x))
			{
				return direction;
			}
		}
		else
		{
			if (p1.y > p.y)
				return direction;
		}		
	}
	return 0;
}

int32_t pointInPolygon(PxVec3* vertices, PxVec3& diagPoint, int32_t edgeCount, PxVec3& normal)
{
	std::vector<PxVec2> projectedVertices(edgeCount * 2);
	ProjectionDirections pDir = getProjectionDirection(normal);
	PxVec2 projectedDiagPoint = getProjectedPoint(diagPoint, pDir);
	PxVec2* saveVert = projectedVertices.data();
	PxVec3* p = vertices;
	for (int32_t i = 0; i < edgeCount * 2; ++i)
	{
		*saveVert = getProjectedPoint(*p, pDir);
		++saveVert;
		++p;
	}	
	int32_t counter = 0;
	PxVec2* v = projectedVertices.data();
	for (int32_t i = 0; i < edgeCount; ++i)
	{
		PxVec2& p1 = *v;
		PxVec2& p2 = *(v + 1);
		counter += edgeIsAbovePoint(p1, p2, projectedDiagPoint);
		v += 2;
	}
	return counter != 0;
}



int32_t testFacetUnitCubeIntersectionInternal(PxVec3* vertices,PxVec3& facetNormal, int32_t edgeCount)
{
	PxVec3* pnt_p = vertices;
	for (int32_t i = 0; i < edgeCount; ++i)
	{
		if (testEdgeAgainstCube(*pnt_p, *(pnt_p + 1)) == 1)
		{
			return 1;
		}
		pnt_p += 2;
	}

	PxVec3 cubeDiag(0, 0, 0);
	for (int32_t i = 0; i < 3; ++i)
		cubeDiag[i] = (facetNormal[i] < 0) ? -1 : 1;
	float t = vertices->dot(facetNormal) / (cubeDiag.dot(facetNormal));
	if (t > 0.5 || t < -0.5)
		return 0;
	
	PxVec3 intersPoint = cubeDiag * t;
	int trs = pointInPolygon(vertices, intersPoint, edgeCount, facetNormal);
	return trs;
}

enum TrivialFlags
{
	HAS_POINT_BELOW_HIGH_X = ~(1 << 0),
	HAS_POINT_ABOVE_LOW_X = ~(1 << 1),

	HAS_POINT_BELOW_HIGH_Y = ~(1 << 2),
	HAS_POINT_ABOVE_LOW_Y = ~(1 << 3),

	HAS_POINT_BELOW_HIGH_Z = ~(1 << 4),
	HAS_POINT_ABOVE_LOW_Z = ~(1 << 5),



	ALL_ONE = (1 << 6) - 1
};





int32_t testFacetUnitCubeIntersection(const Vertex* vertices, const Edge* edges, const Facet& fc, PxBounds3 cube, float fattening)
{
	const Edge* ed = edges + fc.firstEdgeNumber;
	int32_t trivialFlags = ALL_ONE;
	cube.fattenFast(fattening);
	for (uint32_t i = 0; i < fc.edgesCount; ++i)
	{
		{
			const PxVec3& p = vertices[ed->s].p;
			if (cube.contains(p))
				return 1;
			if (p.x < cube.getCenter().x + 0.5)
				trivialFlags &= HAS_POINT_BELOW_HIGH_X;
			if (p.x > cube.getCenter().x - 0.5)
				trivialFlags &= HAS_POINT_ABOVE_LOW_X;

			if (p.y < cube.getCenter().y + 0.5)
				trivialFlags &= HAS_POINT_BELOW_HIGH_Y;
			if (p.y > cube.getCenter().y - 0.5)
				trivialFlags &= HAS_POINT_ABOVE_LOW_Y;

			if (p.z < cube.getCenter().z + 0.5)
				trivialFlags &= HAS_POINT_BELOW_HIGH_Z;
			if (p.z > cube.getCenter().z - 0.5)
				trivialFlags &= HAS_POINT_ABOVE_LOW_Z;
		}
		{
			const PxVec3& p = vertices[ed->e].p;
			if (cube.contains(p))
				return 1;
			if (p.x < cube.getCenter().x + 0.5)
				trivialFlags &= HAS_POINT_BELOW_HIGH_X;
			if (p.x > cube.getCenter().x - 0.5)
				trivialFlags &= HAS_POINT_ABOVE_LOW_X;

			if (p.y < cube.getCenter().y + 0.5)
				trivialFlags &= HAS_POINT_BELOW_HIGH_Y;
			if (p.y > cube.getCenter().y - 0.5)
				trivialFlags &= HAS_POINT_ABOVE_LOW_Y;

			if (p.z < cube.getCenter().z + 0.5)
				trivialFlags &= HAS_POINT_BELOW_HIGH_Z;
			if (p.z > cube.getCenter().z - 0.5)
				trivialFlags &= HAS_POINT_ABOVE_LOW_Z;
		}

		++ed;
	}
	if (trivialFlags != 0)
	{
		return 0;
	}
	std::vector<PxVec3> verticesRescaled(fc.edgesCount * 2);
	
	int32_t vrt = 0;
	ed = edges + fc.firstEdgeNumber;
	PxVec3 offset = cube.getCenter();
	PxVec3 normal(1, 1, 1);
	
	/**
		Compute normal
	*/
	const PxVec3& v1 = vertices[ed->s].p;
	const PxVec3* v2 = nullptr;
	const PxVec3* v3 = nullptr;
	
	for (uint32_t i = 0; i < fc.edgesCount; ++i)
	{
		if (v1 != vertices[ed->s].p)
		{
			v2 = &vertices[ed->s].p;
			break;
		}
		if (v1 != vertices[ed->e].p)
		{
			v2 = &vertices[ed->e].p;
			break;
		}
		ed++;
	}
	ed = edges + fc.firstEdgeNumber;
	for (uint32_t i = 0; i < fc.edgesCount; ++i)
	{
		if (v1 != vertices[ed->s].p && *v2 != vertices[ed->s].p)
		{
			v3 = &vertices[ed->s].p;
			break;
		}
		if (v1 != vertices[ed->e].p && *v2 != vertices[ed->e].p)
		{
			v3 = &vertices[ed->e].p;
			break;
		}
		ed++;
	}
	ed = edges + fc.firstEdgeNumber;
	if (v2 != nullptr && v3 != nullptr)
	{
		normal = (*v2 - v1).cross(*v3 - v1); 
	}
	else
	{
		return true; // If cant find normal, assume it intersects box.
	}

	
	normal.normalize();

	PxVec3 rescale(.5f / (cube.getExtents().x), .5f / (cube.getExtents().y), 0.5f / (cube.getExtents().z));
	for (uint32_t i = 0; i < fc.edgesCount; ++i)
	{
		verticesRescaled[vrt] = vertices[ed->s].p - offset;
		verticesRescaled[vrt].x *= rescale.x;
		verticesRescaled[vrt].y *= rescale.y;
		verticesRescaled[vrt].z *= rescale.z;
		++vrt;
		verticesRescaled[vrt] = vertices[ed->e].p - offset;
		verticesRescaled[vrt].x *= rescale.x;
		verticesRescaled[vrt].y *= rescale.y;
		verticesRescaled[vrt].z *= rescale.z;
		++ed;
		++vrt;
	}
	return testFacetUnitCubeIntersectionInternal(verticesRescaled.data(), normal, fc.edgesCount);
}


IntersectionTestingAccelerator::IntersectionTestingAccelerator(const Mesh* in, int32_t resolution)
{


	alreadyGotFlag.resize(in->getFacetCount(), 0);
	alreadyGotValue = 0;
	mResolution = resolution;

	float cubeSize = 1.0f / resolution;
	PxVec3 cubeMinimal(-0.5, -0.5, -0.5);
	PxVec3 extents(cubeSize, cubeSize, cubeSize);
	mCubes.resize(mResolution * mResolution * mResolution);
	mSpatialMap.resize(mCubes.size());
	int32_t cubeId = 0;

	// Build unit cube partition
	for (int32_t i = 0; i < mResolution; ++i)
	{				
		cubeMinimal.y = -0.5;
		cubeMinimal.z = -0.5;
		for (int32_t j = 0; j < mResolution; ++j)
		{
			cubeMinimal.z = -0.5;			
			for (int32_t k = 0; k < mResolution; ++k)
			{				
				mCubes[cubeId].minimum = cubeMinimal;
				mCubes[cubeId].maximum = cubeMinimal + extents;
				cubeMinimal.z += cubeSize;
				++cubeId;
			}
			cubeMinimal.y += cubeSize;
		}
		cubeMinimal.x += cubeSize;
	}
	

	for (uint32_t i = 0; i < in->getFacetCount(); ++i)
	{
		for (uint32_t c = 0; c < mCubes.size(); ++c)
		{
			if (testFacetUnitCubeIntersection(in->getVertices(), in->getEdges(), *in->getFacet(i), mCubes[c], 0.001))
			{
				mSpatialMap[c].push_back(i);
			}
		}
	}
}


int32_t IntersectionTestingAccelerator::getNextFacet()
{
	int32_t facetId = -1;

	while (mIteratorCell != -1)
	{
		if (mIteratorFacet >= (int32_t)mSpatialMap[mIteratorCell].size())
		{
			if (!cellList.empty())
			{
				mIteratorCell = cellList.back();
				cellList.pop_back();
				mIteratorFacet = 0;
			}
			else
			{
				mIteratorCell = -1;
				break;
			}
		}
		if (alreadyGotFlag[mSpatialMap[mIteratorCell][mIteratorFacet]] != alreadyGotValue)
		{
			facetId = mSpatialMap[mIteratorCell][mIteratorFacet];
			mIteratorFacet++;
			break;
		}
		else
		{
			mIteratorFacet++;
		}
	}
	if (facetId != -1)
	{
		alreadyGotFlag[facetId] = alreadyGotValue;
	}
	return facetId;
}

void IntersectionTestingAccelerator::setState(const Vertex* pos, const Edge* ed, const Facet& fc)
{
	alreadyGotValue++;
	mIteratorCell = -1;
	mIteratorFacet = -1;
	cellList.clear();
	PxBounds3 bigBox(PxVec3(-0.5, -0.5, -0.5), PxVec3(0.5, 0.5, 0.5));
	if (!testFacetUnitCubeIntersection(pos, ed, fc, bigBox, 0.001f))
	{
		return;
	}
	for (uint32_t i = 0; i < mCubes.size(); ++i)
	{
		if (!mSpatialMap[i].empty())
		if (testFacetUnitCubeIntersection(pos, ed, fc, mCubes[i], 0.001f))
		{
				cellList.push_back(i);
		}
	}
	if (!cellList.empty())
	{
		mIteratorFacet = 0;
		mIteratorCell = cellList.back();
		cellList.pop_back();
	}
}

void IntersectionTestingAccelerator::setState(const PxVec3& p)
{
	alreadyGotValue++;
	mIteratorCell = -1;
	mIteratorFacet = -1;
	cellList.clear();
	
	
	for (uint32_t i = 0; i < mCubes.size(); ++i)
	{		
		PxBounds3 tmp = mCubes[i];
		tmp.fattenFast(0.001);
		if (tmp.contains(p))
		{
			int32_t xyCellId = (((int)((float)i / mResolution)) * mResolution);
			for (int32_t zCell = 0; zCell < mResolution; ++zCell)
			{
				int32_t cell = zCell + xyCellId;
				if (!mSpatialMap[cell].empty())
				{
					cellList.push_back(cell);
				}
					
			}
		}
	}
	if (!cellList.empty())
	{
		mIteratorFacet = 0;
		mIteratorCell = cellList.back();
		cellList.pop_back();
	}
}


#define SWEEP_RESOLUTION 2048


void buildIndex(std::vector<SegmentToIndex>& segm, float offset, float mlt, std::vector<std::vector<uint32_t>>& blocks)
{
	std::set<uint32_t> currentEnabled;
	uint32_t lastBlock = 0;
	for (uint32_t i = 0; i < segm.size(); ++i)
	{
		uint32_t currentBlock = (segm[i].coord - offset) * mlt;
		if (currentBlock >= SWEEP_RESOLUTION) break;
		if (currentBlock != lastBlock)
		{
			for (uint32_t j = lastBlock + 1; j <= currentBlock; ++j)
			{
				for (auto id : currentEnabled)
					blocks[j].push_back(id);
			}
			lastBlock = currentBlock;
		}
		if (segm[i].end == false)
		{
			blocks[lastBlock].push_back(segm[i].index);
			currentEnabled.insert(segm[i].index);
		}
		else
		{
			currentEnabled.erase(segm[i].index);
		}
	}
	
}


SweepingAccelerator::SweepingAccelerator(Nv::Blast::Mesh* in)
{
	PxBounds3 bnd;

	const Vertex* verts = in->getVertices();
	const Edge* edges = in->getEdges();

	facetCount = in->getFacetCount();

	foundx.resize(facetCount, 0);
	foundy.resize(facetCount, 0);


	std::vector<SegmentToIndex> xevs;
	std::vector<SegmentToIndex> yevs;
	std::vector<SegmentToIndex> zevs;


	for (uint32_t i = 0; i < in->getFacetCount(); ++i)
	{
		const Facet* fc = in->getFacet(i);
		bnd.setEmpty();
		for (uint32_t v = 0; v < fc->edgesCount; ++v)
		{
			bnd.include(verts[edges[v + fc->firstEdgeNumber].s].p);
		}
		bnd.scaleFast(1.1f);
		xevs.push_back(SegmentToIndex(bnd.minimum.x, i, false));
		xevs.push_back(SegmentToIndex(bnd.maximum.x, i, true));

		yevs.push_back(SegmentToIndex(bnd.minimum.y, i, false));
		yevs.push_back(SegmentToIndex(bnd.maximum.y, i, true));

		zevs.push_back(SegmentToIndex(bnd.minimum.z, i, false));
		zevs.push_back(SegmentToIndex(bnd.maximum.z, i, true));

	}

	std::sort(xevs.begin(), xevs.end());
	std::sort(yevs.begin(), yevs.end());
	std::sort(zevs.begin(), zevs.end());

	
	minimal.x = xevs[0].coord;
	minimal.y = yevs[0].coord;
	minimal.z = zevs[0].coord;

	
	maximal.x = xevs.back().coord;
	maximal.y = yevs.back().coord;
	maximal.z = zevs.back().coord;

		
	rescale = (maximal - minimal) * 1.01f;
	rescale.x = 1.0f / rescale.x * SWEEP_RESOLUTION;
	rescale.y = 1.0f / rescale.y * SWEEP_RESOLUTION;
	rescale.z = 1.0f / rescale.z * SWEEP_RESOLUTION;

	xSegm.resize(SWEEP_RESOLUTION);
	ySegm.resize(SWEEP_RESOLUTION);
	zSegm.resize(SWEEP_RESOLUTION);


	buildIndex(xevs, minimal.x, rescale.x, xSegm);
	buildIndex(yevs, minimal.y, rescale.y, ySegm);
	buildIndex(zevs, minimal.z, rescale.z, zSegm);

	
	iterId = 1;
	current = 0;
}

void SweepingAccelerator::setState(const Vertex* pos, const Edge* ed, const Facet& fc)
{
	current = 0;
	indices.clear();
	
	PxBounds3 bnd;
	bnd.setEmpty();
	for (uint32_t i = 0; i < fc.edgesCount; ++i)
	{
		bnd.include(pos[ed[fc.firstEdgeNumber + i].s].p);
	}
	bnd.scaleFast(1.1);
	uint32_t start = (std::max(0.0f, bnd.minimum.x - minimal.x)) * rescale.x;
	uint32_t end = (std::max(0.0f, bnd.maximum.x - minimal.x)) * rescale.x;
	for (uint32_t i = start; i <= end && i < SWEEP_RESOLUTION; ++i)
	{
		for (auto id : xSegm[i])
		{
			foundx[id] = iterId;
		}
	}
	start = (std::max(0.0f, bnd.minimum.y - minimal.y)) * rescale.y;
	end = (std::max(0.0f, bnd.maximum.y - minimal.y)) * rescale.y;
	for (uint32_t i = start; i <= end && i < SWEEP_RESOLUTION; ++i)
	{
		for (auto id : ySegm[i])
		{
			foundy[id] = iterId;
		}
	}
	start = (std::max(0.0f, bnd.minimum.z - minimal.z)) * rescale.z;
	end = (std::max(0.0f, bnd.maximum.z - minimal.z)) * rescale.z;
	for (uint32_t i = start; i <= end && i < SWEEP_RESOLUTION; ++i)
	{
		for (auto id : zSegm[i])
		{
			if (foundy[id] == iterId && foundx[id] == iterId)
			{
				foundx[id] = iterId + 1;
				foundy[id] = iterId + 1;
				indices.push_back(id);
			}
		}
	}

	iterId += 2;
}


void SweepingAccelerator::setState(const physx::PxVec3& point) {
	
	indices.clear();

	/*for (uint32_t i = 0; i < facetCount; ++i)
	{
		indices.push_back(i);
	}*/

	uint32_t xIndex = (point.x - minimal.x) * rescale.x;
	uint32_t yIndex = (point.y- minimal.y) * rescale.y;

	for (uint32_t i = 0; i < xSegm[xIndex].size(); ++i)
	{
		foundx[xSegm[xIndex][i]] = iterId;
	}
	for (uint32_t i = 0; i < ySegm[yIndex].size(); ++i)
	{
		if (foundx[ySegm[yIndex][i]] == iterId)
		{
			indices.push_back(ySegm[yIndex][i]);
		}
	}
	iterId++;
	current = 0;
	NV_UNUSED(point);
}
int32_t SweepingAccelerator::getNextFacet()
{
	if (static_cast<uint32_t>(current) < indices.size())
	{
		++current;
		return indices[current - 1];
	}
	else
		return -1;
}






} // namespace Blast
} // namespace Nv
