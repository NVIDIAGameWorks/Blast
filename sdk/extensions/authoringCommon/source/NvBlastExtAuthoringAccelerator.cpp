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
#include "NvBlastGlobals.h"

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
void DummyAccelerator::setState(const physx::PxBounds3* bound) {
	current = 0;
	NV_UNUSED(bound);
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

Grid::Grid(int32_t resolution) : mResolution(resolution)
{
	/**
		Set up 3d grid
	*/
	r3 = resolution * resolution * resolution;
	mSpatialMap.resize(resolution * resolution * resolution);
}

void Grid::setMesh(const Mesh* m)
{
	physx::PxBounds3 bd = m->getBoundingBox();
	mappedFacetCount = m->getFacetCount();
	bd.fattenFast(0.001f);
	spos = bd.minimum;
	deltas = PxVec3(mResolution / bd.getDimensions().x, mResolution / bd.getDimensions().y, mResolution / bd.getDimensions().z);

	for (int32_t i = 0; i < r3; ++i)
		mSpatialMap[i].clear();

	const float ofs = 0.001f;

	for (uint32_t fc = 0; fc < m->getFacetCount(); ++fc)
	{
		physx::PxBounds3 cfc = *m->getFacetBound(fc);

		int32_t is = std::max(0.f, (cfc.minimum.x - spos.x - ofs) * deltas.x);
		int32_t ie = std::max(0.f, (cfc.maximum.x - spos.x + ofs) * deltas.x);

		int32_t js = std::max(0.f, (cfc.minimum.y - spos.y - ofs) * deltas.y);
		int32_t je = std::max(0.f, (cfc.maximum.y - spos.y + ofs) * deltas.y);

		int32_t ks = std::max(0.f, (cfc.minimum.z - spos.z - ofs) * deltas.z);
		int32_t ke = std::max(0.f, (cfc.maximum.z - spos.z + ofs) * deltas.z);

		for (int32_t i = is; i < mResolution && i <= ie; ++i)
		{
			for (int32_t j = js; j < mResolution && j <= je; ++j)
			{
				for (int32_t k = ks; k < mResolution && k <= ke; ++k)
				{
					mSpatialMap[(i * mResolution + j) * mResolution + k].push_back(fc);
				}
			}
		}
	}
}


GridWalker::GridWalker(Grid* grd)
{
	mGrid = grd;
	alreadyGotValue = 0;
	alreadyGotFlag.resize(1 << 12);
	cellList.resize(1 << 12);
	pointCmdDir = 0;
}

void GridWalker::setState(const Vertex* pos, const Edge* ed, const Facet& fc)
{
	
	physx::PxBounds3 cfc(PxBounds3::empty());

	for (uint32_t v = 0; v < fc.edgesCount; ++v)
	{
		cfc.include(pos[ed[fc.firstEdgeNumber + v].s].p);
		cfc.include(pos[ed[fc.firstEdgeNumber + v].e].p);
	}
	setState(&cfc);
}

void GridWalker::setState(const PxBounds3* facetBounding)
{
	alreadyGotValue++;
	mIteratorCell = -1;
	mIteratorFacet = -1;
	gotCells = 0;

	physx::PxBounds3 cfc = *facetBounding;



	int32_t is = std::max(0.f, (cfc.minimum.x - mGrid->spos.x - 0.001f) * mGrid->deltas.x);
	int32_t ie = std::max(0.f, (cfc.maximum.x - mGrid->spos.x + 0.001f) * mGrid->deltas.x);

	int32_t js = std::max(0.f, (cfc.minimum.y - mGrid->spos.y - 0.001f) * mGrid->deltas.y);
	int32_t je = std::max(0.f, (cfc.maximum.y - mGrid->spos.y + 0.001f) * mGrid->deltas.y);

	int32_t ks = std::max(0.f, (cfc.minimum.z - mGrid->spos.z - 0.001f) * mGrid->deltas.z);
	int32_t ke = std::max(0.f, (cfc.maximum.z - mGrid->spos.z + 0.001f) * mGrid->deltas.z);

	for (int32_t i = is; i < mGrid->mResolution && i <= ie; ++i)
	{
		for (int32_t j = js; j < mGrid->mResolution && j <= je; ++j)
		{
			for (int32_t k = ks; k < mGrid->mResolution && k <= ke; ++k)
			{
				int32_t id = (i * mGrid->mResolution + j) * mGrid->mResolution + k;
				if (!mGrid->mSpatialMap[id].empty())
				{
					cellList[gotCells++] = id;
				}

			}
		}
	}
	if (gotCells != 0)
	{
		mIteratorFacet = 0;
		mIteratorCell = cellList[gotCells - 1];
		gotCells--;
	}
}


void GridWalker::setPointCmpDirection(int32_t d)
{
	pointCmdDir = d;
}


void GridWalker::setState(const physx::PxVec3& point)
{
	alreadyGotValue++;
	mIteratorCell = -1;
	mIteratorFacet = -1;
	gotCells = 0;
	
	int32_t is = std::max(0.f, (point.x - mGrid->spos.x - 0.001f) * mGrid->deltas.x);
	int32_t ie = std::max(0.f, (point.x - mGrid->spos.x + 0.001f) * mGrid->deltas.x);

	int32_t js = std::max(0.f, (point.y - mGrid->spos.y - 0.001f) * mGrid->deltas.y);
	int32_t je = std::max(0.f, (point.y - mGrid->spos.y + 0.001f) * mGrid->deltas.y);

	int32_t ks = 0;
	int32_t ke = mGrid->mResolution;
	switch (pointCmdDir)
	{
	case 1:
		ks = std::max(0.f, (point.z - mGrid->spos.z - 0.001f) * mGrid->deltas.z);
		break;
	case -1:
		ke = std::max(0.f, (point.z - mGrid->spos.z + 0.001f) * mGrid->deltas.z);
	}

	for (int32_t i = is; i < mGrid->mResolution && i <= ie; ++i)
	{
		for (int32_t j = js; j < mGrid->mResolution && j <= je; ++j)
		{
			for (int32_t k = ks; k <= ke && k < mGrid->mResolution; ++k)
			{
				int32_t id = (i * mGrid->mResolution + j) * mGrid->mResolution + k;
				if (!mGrid->mSpatialMap[id].empty())
				{
					cellList[gotCells++] = id;
				}
			}
		}
	}

	if (gotCells != 0)
	{
		mIteratorFacet = 0;
		mIteratorCell = cellList[gotCells - 1];
		gotCells--;
	}
}
int32_t GridWalker::getNextFacet()
{
	int32_t facetId = -1;

	while (mIteratorCell != -1)
	{
		if (mIteratorFacet >= (int32_t)mGrid->mSpatialMap[mIteratorCell].size())
		{
			if (gotCells != 0)
			{
				mIteratorCell = cellList[gotCells - 1];
				gotCells--;
				mIteratorFacet = 0;
			}
			else
			{
				mIteratorCell = -1;
				break;
			}
		}
		if (alreadyGotFlag[mGrid->mSpatialMap[mIteratorCell][mIteratorFacet]] != alreadyGotValue)
		{
			facetId = mGrid->mSpatialMap[mIteratorCell][mIteratorFacet];
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
	cellList.resize(1 << 16);
	gotCells = 0;
	buildAccelStructure(mesh->getVertices(), mesh->getEdges(), mesh->getFacetsBuffer(), mesh->getFacetCount());
}


BBoxBasedAccelerator::~BBoxBasedAccelerator()
{
	mResolution = 0;
	mBounds.setEmpty();
	mSpatialMap.clear();
	mCells.clear();
	cellList.clear();
}

int32_t BBoxBasedAccelerator::getNextFacet()
{
	int32_t facetId = -1;

	while (mIteratorCell != -1)
	{
		if (mIteratorFacet >= (int32_t)mSpatialMap[mIteratorCell].size())
		{
			if (gotCells != 0)
			{
				mIteratorCell = cellList[gotCells - 1];
				gotCells--;
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

	physx::PxBounds3 cfc(PxBounds3::empty());

	for (uint32_t v = 0; v < fc.edgesCount; ++v)
	{
		cfc.include(pos[ed[fc.firstEdgeNumber + v].s].p);
		cfc.include(pos[ed[fc.firstEdgeNumber + v].e].p);
	}
	setState(&cfc);
}

void BBoxBasedAccelerator::setState(const PxBounds3* facetBox)
{
	alreadyGotValue++;
	mIteratorCell = -1;
	mIteratorFacet = -1;
	gotCells = 0;
	
	for (uint32_t i = 0; i < mCells.size(); ++i)
	{
		if (weakBoundingBoxIntersection(mCells[i], *facetBox))
		{
			if (!mSpatialMap[i].empty())
				cellList[gotCells++] = i;
		}
	}
	if (gotCells != 0)
	{
		mIteratorFacet = 0;
		mIteratorCell = cellList[gotCells - 1];
		gotCells--;
	}
}


void BBoxBasedAccelerator::setState(const PxVec3& p)
{
	alreadyGotValue++;
	mIteratorCell = -1;
	mIteratorFacet = -1;
	gotCells = 0;
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
					cellList[gotCells++] = cell;
			}
		}
	}
	if (gotCells != 0)
	{
		mIteratorFacet = 0;
		mIteratorCell = cellList[gotCells - 1];
		gotCells--;
	}
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
			if (weakBoundingBoxIntersection(mCells[i], bBox))
			{
				mSpatialMap[i].push_back(facet);
			}
		}
		fc++;
	}
	alreadyGotFlag.resize(facetCount, 0);
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

void SweepingAccelerator::setState(const PxBounds3* facetBounds)
{
	current = 0;
	indices.clear();
	
	PxBounds3 bnd = *facetBounds;

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

void SweepingAccelerator::setState(const Vertex* pos, const Edge* ed, const Facet& fc)
{

	physx::PxBounds3 cfc(PxBounds3::empty());

	for (uint32_t v = 0; v < fc.edgesCount; ++v)
	{
		cfc.include(pos[ed[fc.firstEdgeNumber + v].s].p);
		cfc.include(pos[ed[fc.firstEdgeNumber + v].e].p);
	}
	setState(&cfc);
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
