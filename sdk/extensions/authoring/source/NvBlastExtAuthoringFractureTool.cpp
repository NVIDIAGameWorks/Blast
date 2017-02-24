/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastExtAuthoringFractureTool.h"
// This warning arises when using some stl containers with older versions of VC
// c:\program files (x86)\microsoft visual studio 12.0\vc\include\xtree(1826): warning C4702: unreachable code
#if NV_VC && NV_VC < 14
#pragma warning(disable : 4702)
#endif
#include <queue>
#include <vector>
#include "NvBlastExtAuthoringVSA.h"
#include <float.h>
#include "NvBlastExtAuthoringTriangulator.h"
#include "NvBlastExtAuthoringBooleanTool.h"
#include "NvBlastExtAuthoringAccelerator.h"
#include "NvBlast.h"
#include "NvBlastExtAuthoringPerlinNoise.h"
#include <NvBlastAssert.h>
using namespace physx;

#define DEFAULT_BB_ACCELARATOR_RES 10

namespace Nv
{
namespace Blast
{

struct Halfspace_partitioning : public VSA::VS3D_Halfspace_Set
{
	std::vector<physx::PxPlane> planes;
	VSA::real farthest_halfspace(VSA::real plane[4], const VSA::real point[4])
	{
		float biggest_d = -FLT_MAX;
		for (uint32_t i = 0; i < planes.size(); ++i)
		{
			float d = planes[i].n.x * point[0] + planes[i].n.y * point[1] + planes[i].n.z * point[2] + planes[i].d * point[3];
			if (d > biggest_d)
			{
				biggest_d = d;
				plane[0] = planes[i].n.x;
				plane[1] = planes[i].n.y;
				plane[2] = planes[i].n.z;
				plane[3] = planes[i].d;
			}
		}
		return biggest_d;
	};
};


void findCellBasePlanes(const std::vector<PxVec3>& sites, std::vector<std::vector<int32_t> >& neighboors)
{
	Halfspace_partitioning prt;
	std::vector<physx::PxPlane>& planes = prt.planes;
	neighboors.resize(sites.size());
	for (uint32_t cellId = 0; cellId + 1 < sites.size(); ++cellId)
	{
		planes.clear();
		planes.resize(sites.size() - 1 - cellId);
		std::vector<PxVec3> midpoints(sites.size() - 1);
		int32_t collected = 0;

		for (uint32_t i = cellId + 1; i < sites.size(); ++i)
		{
			PxVec3 midpoint = 0.5 * (sites[i] + sites[cellId]);
			PxVec3 direction = (sites[i] - sites[cellId]).getNormalized();
			planes[collected].n = direction;
			planes[collected].d = -direction.dot(midpoint);
			midpoints[collected] = midpoint;
			++collected;
		}
		for (uint32_t i = 0; i < planes.size(); ++i)
		{
			planes[i].n = -planes[i].n;
			planes[i].d = -planes[i].d;

			if (VSA::vs3d_test(prt))
			{
				neighboors[cellId].push_back(i + cellId + 1);
				neighboors[i + cellId + 1].push_back(cellId);
			};
			planes[i].n = -planes[i].n;
			planes[i].d = -planes[i].d;
		}
	}
}


#define SITE_BOX_SIZE 4
#define CUTTING_BOX_SIZE 40

Mesh* getCellMesh(BooleanEvaluator& eval, int32_t planeIndexerOffset, int32_t cellId, const std::vector<PxVec3>& sites, std::vector < std::vector<int32_t> >& neighboors)
{
	Mesh* cell = getBigBox(sites[cellId], SITE_BOX_SIZE);
	Mesh* cuttingMesh = getCuttingBox(PxVec3(0, 0, 0), PxVec3(1, 1, 1), CUTTING_BOX_SIZE, 0);

	for (uint32_t i = 0; i < neighboors[cellId].size(); ++i)
	{
		int32_t nCell = neighboors[cellId][i];
		PxVec3 midpoint = 0.5 * (sites[nCell] + sites[cellId]);
		PxVec3 direction = (sites[nCell] - sites[cellId]).getNormalized();
		int32_t planeIndex = static_cast<int32_t>(sites.size()) * std::min(cellId, nCell) + std::max(cellId, nCell) + planeIndexerOffset;
		if (nCell < cellId)
			planeIndex = -planeIndex;
		setCuttingBox(midpoint, -direction, cuttingMesh, CUTTING_BOX_SIZE, planeIndex);
		eval.performFastCutting(cell, cuttingMesh, BooleanConfigurations::BOOLEAN_INTERSECION());
		Mesh* newCell = eval.createNewMesh();
		delete cell;
		cell = newCell;
		if (cell == nullptr)
			break;
	}
	return cell;
}


bool blastBondComparator(const NvBlastBondDesc& a, const NvBlastBondDesc& b)
{
	if (a.chunkIndices[0] == b.chunkIndices[0])
		return a.chunkIndices[1] < b.chunkIndices[1];
	else
		return a.chunkIndices[0] < b.chunkIndices[0];
}


#define MAX_VORONOI_ATTEMPT_NUMBER 450

VoronoiSitesGenerator::VoronoiSitesGenerator(Mesh* mesh, RandomGeneratorBase* rnd)
{
	mMesh = mesh;
	mRnd = rnd;
	mAccelerator = new BBoxBasedAccelerator(mMesh, DEFAULT_BB_ACCELARATOR_RES);
	mStencil = nullptr;
}

void VoronoiSitesGenerator::setBaseMesh(Mesh* m)
{
	mGeneratedSites.clear();
	delete mAccelerator;
	mMesh = m;
	mAccelerator = new BBoxBasedAccelerator(mMesh, DEFAULT_BB_ACCELARATOR_RES);
}

VoronoiSitesGenerator::~VoronoiSitesGenerator()
{
	delete mAccelerator;
	mAccelerator = nullptr;
}


void VoronoiSitesGenerator::setStencil(Mesh* stencil)
{
	mStencil = stencil;
}


void VoronoiSitesGenerator::clearStencil()
{
	mStencil = nullptr;
}


void VoronoiSitesGenerator::uniformlyGenerateSitesInMesh(const uint32_t sitesCount)
{
	BooleanEvaluator voronoiMeshEval(nullptr);
	PxVec3 mn = mMesh->getBoundingBox().minimum;
	PxVec3 mx = mMesh->getBoundingBox().maximum;
	PxVec3 vc = mx - mn;
	uint32_t attemptNumber = 0;
	uint32_t generatedSites = 0;
	while (generatedSites < sitesCount && attemptNumber < MAX_VORONOI_ATTEMPT_NUMBER)
	{
		float rn1 = mRnd->getRandomValue() * vc.x;
		float rn2 = mRnd->getRandomValue() * vc.y;
		float rn3 = mRnd->getRandomValue() * vc.z;
		if (voronoiMeshEval.isPointContainedInMesh(mMesh, PxVec3(rn1, rn2, rn3) + mn) && (mStencil == nullptr
			|| voronoiMeshEval.isPointContainedInMesh(mStencil, PxVec3(rn1, rn2, rn3) + mn)))
		{
			generatedSites++;
			mGeneratedSites.push_back(PxVec3(rn1, rn2, rn3) + mn);
			attemptNumber = 0;
		}
		else
		{
			attemptNumber++;
			if (attemptNumber > MAX_VORONOI_ATTEMPT_NUMBER)
				break;
		}
	}
}


void VoronoiSitesGenerator::clusteredSitesGeneration(const uint32_t numberOfClusters, const uint32_t sitesPerCluster, float clusterRadius)
{
	BooleanEvaluator voronoiMeshEval(nullptr);
	PxVec3 mn = mMesh->getBoundingBox().minimum;
	PxVec3 mx = mMesh->getBoundingBox().maximum;
	PxVec3 middle = (mx + mn) * 0.5;
	PxVec3 vc = (mx - mn) * 0.5;
	uint32_t attemptNumber = 0;
	uint32_t generatedSites = 0;
	std::vector<PxVec3> tempPoints;
	while (generatedSites < numberOfClusters)
	{
		float rn1 = mRnd->getRandomValue() * 2 - 1;
		float rn2 = mRnd->getRandomValue() * 2 - 1;
		float rn3 = mRnd->getRandomValue() * 2 - 1;
		PxVec3 p = PxVec3(middle.x + rn1 * vc.x, middle.y + rn2 * vc.y, middle.z + rn3 * vc.z);

		if (voronoiMeshEval.isPointContainedInMesh(mMesh, p) && (mStencil == nullptr
			|| voronoiMeshEval.isPointContainedInMesh(mStencil, p)))
		{
			generatedSites++;
			tempPoints.push_back(p);
			attemptNumber = 0;
		}
		else
		{
			attemptNumber++;
			if (attemptNumber > MAX_VORONOI_ATTEMPT_NUMBER)
				break;
		}
	}
	int32_t totalCount = 0;
	for (; tempPoints.size() > 0; tempPoints.pop_back())
	{
		uint32_t unif = sitesPerCluster;
		generatedSites = 0;
		while (generatedSites < unif)
		{
			PxVec3 p = tempPoints.back() + PxVec3(mRnd->getRandomValue() * 2 - 1, mRnd->getRandomValue() * 2 - 1, mRnd->getRandomValue() * 2 - 1).getNormalized() * (mRnd->getRandomValue() + 0.001f) * clusterRadius;
			if (voronoiMeshEval.isPointContainedInMesh(mMesh, p) && (mStencil == nullptr
				|| voronoiMeshEval.isPointContainedInMesh(mStencil, p)))
			{
				totalCount++;
				generatedSites++;
				mGeneratedSites.push_back(p);
				attemptNumber = 0;
			}
			else
			{
				attemptNumber++;
				if (attemptNumber > MAX_VORONOI_ATTEMPT_NUMBER)
					break;
			}
		}

	}

}


#define IN_SPHERE_ATTEMPT_NUMBER 20

void VoronoiSitesGenerator::addSite(const physx::PxVec3& site)
{
	mGeneratedSites.push_back(site);
}


void VoronoiSitesGenerator::generateInSphere(const uint32_t count, const float radius, const physx::PxVec3& center)
{
	BooleanEvaluator voronoiMeshEval(nullptr);
	uint32_t attemptNumber = 0;
	uint32_t generatedSites = 0;
	std::vector<PxVec3> tempPoints;

	while (generatedSites < count && attemptNumber < MAX_VORONOI_ATTEMPT_NUMBER)
	{
		float rn1 = mRnd->getRandomValue() * radius;
		float rn2 = mRnd->getRandomValue() * radius;
		float rn3 = mRnd->getRandomValue() * radius;
		if (voronoiMeshEval.isPointContainedInMesh(mMesh, PxVec3(rn1, rn2, rn3) + center) && (mStencil == nullptr
			|| voronoiMeshEval.isPointContainedInMesh(mStencil, PxVec3(rn1, rn2, rn3) + center)))
		{
			generatedSites++;
			mGeneratedSites.push_back(PxVec3(rn1, rn2, rn3) + center);
			attemptNumber = 0;
		}
		else
		{
			attemptNumber++;
			if (attemptNumber > MAX_VORONOI_ATTEMPT_NUMBER)
				break;
		}
	}
}


void VoronoiSitesGenerator::deleteInSphere(const float radius, const physx::PxVec3& center, float deleteProbability)
{
	float r2 = radius * radius;
	for (uint32_t i = 0; i < mGeneratedSites.size(); ++i)
	{
		if ((mGeneratedSites[i] - center).magnitudeSquared() < r2 && mRnd->getRandomValue() <= deleteProbability)
		{
			std::swap(mGeneratedSites[i], mGeneratedSites.back());
			mGeneratedSites.pop_back();
			--i;
		}
	}
}


void VoronoiSitesGenerator::radialPattern(const physx::PxVec3& center, const physx::PxVec3& normal, float radius, int32_t angularSteps, int32_t radialSteps, float angleOffset, float variability)
{
//	mGeneratedSites.push_back(center);
	physx::PxVec3 t1, t2;
	if (abs(normal.z) < 0.9)
	{
		t1 = normal.cross(PxVec3(0, 0, 1));
	}
	else
	{
		t1 = normal.cross(PxVec3(1, 0, 0));
	}
	t2 = t1.cross(normal);
	t1.normalize();
	t2.normalize();

	float radStep = radius / radialSteps;
	int32_t cCr = 0;

	float angleStep = PxPi * 2 / angularSteps;
	for (float cRadius = radStep; cRadius < radius; cRadius += radStep)
	{
		float cAngle = angleOffset * cCr;
		for (int32_t i = 0; i < angularSteps; ++i)
		{
			float angVars = mRnd->getRandomValue() * variability + (1.0f - 0.5f * variability);
			float radVars = mRnd->getRandomValue() * variability + (1.0f - 0.5f * variability);

			PxVec3 nPos = (PxCos(cAngle * angVars) * t1 + PxSin(cAngle * angVars) * t2) * cRadius * radVars + center;
			mGeneratedSites.push_back(nPos);
			cAngle += angleStep;
		}
		++cCr;
	}
}


std::vector<PxVec3>& VoronoiSitesGenerator::getVoronoiSites()
{
	return mGeneratedSites;
}


int32_t FractureTool::voronoiFracturing(uint32_t chunkId, const std::vector<physx::PxVec3>& cellPointsIn, bool replaceChunk)
{
	if (chunkId == 0 && replaceChunk)
	{
		return 1;
	}

	int32_t chunkIndex = getChunkIndex(chunkId);
	if (chunkIndex == -1 || cellPointsIn.size() < 2)
	{
		return 1;
	}
	if (!mChunkData[chunkIndex].isLeaf)
	{
		deleteAllChildsOfChunk(chunkId);
	}
	chunkIndex = getChunkIndex(chunkId);

	Mesh* mesh = mChunkData[chunkIndex].meshData;

	std::vector<PxVec3> cellPoints(cellPointsIn.size());
	for (uint32_t i = 0; i < cellPointsIn.size(); ++i)
	{
		cellPoints[i] = (cellPointsIn[i] - mOffset) * (1.0f / mScaleFactor);
	}

	/**
	Prebuild accelerator structure
	*/
	BooleanEvaluator eval(mLoggingCallback);
	BooleanEvaluator voronoiMeshEval(mLoggingCallback);

	BBoxBasedAccelerator spAccel = BBoxBasedAccelerator(mesh, DEFAULT_BB_ACCELARATOR_RES);

	std::vector<std::vector<int32_t> > neighboors;
	findCellBasePlanes(cellPoints, neighboors);

	/**
	Fracture
	*/
	int32_t parentChunk = replaceChunk ? mChunkData[chunkIndex].parent : chunkId;
	std::vector<uint32_t> newlyCreatedChunksIds;
	for (uint32_t i = 0; i < cellPoints.size(); ++i)
	{
		Mesh* cell = getCellMesh(eval, mPlaneIndexerOffset, i, cellPoints, neighboors);

		if (cell == nullptr)
		{
			continue;
		}
		DummyAccelerator dmAccel(cell->getFacetCount());
		voronoiMeshEval.performBoolean(mesh, cell, &spAccel, &dmAccel, BooleanConfigurations::BOOLEAN_INTERSECION());
		Mesh* resultMesh = voronoiMeshEval.createNewMesh();
		if (resultMesh)
		{
			mChunkData.push_back(ChunkInfo());
			mChunkData.back().isLeaf = true;
			mChunkData.back().meshData = resultMesh;
			mChunkData.back().parent = parentChunk;
			mChunkData.back().chunkId = mChunkIdCounter++;
			newlyCreatedChunksIds.push_back(mChunkData.back().chunkId);
		}
		eval.reset();
		delete cell;
	}
	mChunkData[chunkIndex].isLeaf = false;
	if (replaceChunk)
	{
		eraseChunk(chunkId);
	}
	mPlaneIndexerOffset += static_cast<int32_t>(cellPoints.size() * cellPoints.size());


	if (mRemoveIslands)
	{
		for (auto chunkToCheck : newlyCreatedChunksIds)
		{
			islandDetectionAndRemoving(chunkToCheck);
		}
	}
	return 0;
}

Mesh FractureTool::getChunkMesh(int32_t chunkId)
{
	Mesh temp = *mChunkData[getChunkIndex(chunkId)].meshData;
	for (uint32_t i = 0; i < temp.getVerticesCount(); ++i)
	{
		temp.getVertices()[i].p = temp.getVertices()[i].p * mScaleFactor + mOffset;
	}
	temp.recalculateBoundingBox();

	return temp;
}


int32_t FractureTool::voronoiFracturing(uint32_t chunkId, const std::vector<physx::PxVec3>& cellPointsIn, const physx::PxVec3& scale, bool replaceChunk)
{
	if (chunkId == 0 && replaceChunk)
	{
		return 1;
	}

	int32_t chunkIndex = getChunkIndex(chunkId);
	if (chunkIndex == -1 || cellPointsIn.size() < 2)
	{
		return 1;
	}
	if (!mChunkData[chunkIndex].isLeaf)
	{
		deleteAllChildsOfChunk(chunkId);
	}
	chunkIndex = getChunkIndex(chunkId);

	Mesh* mesh = mChunkData[chunkIndex].meshData;

	std::vector<PxVec3> cellPoints(cellPointsIn.size());
	for (uint32_t i = 0; i < cellPointsIn.size(); ++i)
	{
		cellPoints[i] = (cellPointsIn[i] - mOffset) * (1.0f / mScaleFactor);
	
		cellPoints[i].x *= (1.0f / scale.x);
		cellPoints[i].y *= (1.0f / scale.y);
		cellPoints[i].z *= (1.0f / scale.z);
	}

	/**
	Prebuild accelerator structure
	*/
	BooleanEvaluator eval(mLoggingCallback);
	BooleanEvaluator voronoiMeshEval(mLoggingCallback);

	BBoxBasedAccelerator spAccel = BBoxBasedAccelerator(mesh, DEFAULT_BB_ACCELARATOR_RES);

	std::vector<std::vector<int32_t> > neighboors;
	findCellBasePlanes(cellPoints, neighboors);

	/**
	Fracture
	*/
	int32_t parentChunk = replaceChunk ? mChunkData[chunkIndex].parent : chunkId;
	std::vector<uint32_t> newlyCreatedChunksIds;

	for (uint32_t i = 0; i < cellPoints.size(); ++i)
	{
		Mesh* cell = getCellMesh(eval, mPlaneIndexerOffset, i, cellPoints, neighboors);
				
		if (cell == nullptr)
		{
			continue;
		}

		for (uint32_t v = 0; v < cell->getVerticesCount(); ++v)
		{
			cell->getVertices()[v].p.x *= scale.x;
			cell->getVertices()[v].p.y *= scale.y;
			cell->getVertices()[v].p.z *= scale.z;
		}
		cell->recalculateBoundingBox();
		DummyAccelerator dmAccel(cell->getFacetCount());
		voronoiMeshEval.performBoolean(mesh, cell, &spAccel, &dmAccel, BooleanConfigurations::BOOLEAN_INTERSECION());
		Mesh* resultMesh = voronoiMeshEval.createNewMesh();
		if (resultMesh)
		{
			mChunkData.push_back(ChunkInfo());
			mChunkData.back().isLeaf = true;
			mChunkData.back().meshData = resultMesh;
			mChunkData.back().parent = parentChunk;
			mChunkData.back().chunkId = mChunkIdCounter++;
			newlyCreatedChunksIds.push_back(mChunkData.back().chunkId);
		}
		eval.reset();
		delete cell;
	}
	mChunkData[chunkIndex].isLeaf = false;
	if (replaceChunk)
	{
		eraseChunk(chunkId);
	}
	mPlaneIndexerOffset += static_cast<int32_t>(cellPoints.size() * cellPoints.size());

	if (mRemoveIslands)
	{
		for (auto chunkToCheck : newlyCreatedChunksIds)
		{
			islandDetectionAndRemoving(chunkToCheck);
		}
	}

	return 0;
}


int32_t FractureTool::slicing(uint32_t chunkId, SlicingConfiguration conf, bool replaceChunk, RandomGeneratorBase* rnd)
{
	if (conf.noiseAmplitude != 0)
	{
		return slicingNoisy(chunkId, conf, replaceChunk, rnd);
	}

	if (replaceChunk && chunkId == 0)
	{
		return 1;
	}

	int32_t chunkIndex = getChunkIndex(chunkId);
	if (chunkIndex == -1)
	{
		return 1;
	}
	if (!mChunkData[chunkIndex].isLeaf)
	{
		deleteAllChildsOfChunk(chunkId);
	}
	chunkIndex = getChunkIndex(chunkId);
	

	Mesh* mesh = new Mesh(*mChunkData[chunkIndex].meshData);
	
	BooleanEvaluator bTool(mLoggingCallback);

	int32_t x_slices = conf.x_slices;
	int32_t y_slices = conf.y_slices;
	int32_t z_slices = conf.z_slices;

	const PxBounds3 sourceBBox = mesh->getBoundingBox();

	PxVec3 center = PxVec3(mesh->getBoundingBox().minimum.x, 0, 0);


	float x_offset = (sourceBBox.maximum.x - sourceBBox.minimum.x) * (1.0f / (x_slices + 1));
	float y_offset = (sourceBBox.maximum.y - sourceBBox.minimum.y) * (1.0f / (y_slices + 1));
	float z_offset = (sourceBBox.maximum.z - sourceBBox.minimum.z) * (1.0f / (z_slices + 1));

	center.x += x_offset;

	PxVec3 dir(1, 0, 0);

	Mesh* slBox = getCuttingBox(center, dir, 20, 0);

	ChunkInfo ch;
	ch.isLeaf = true;
	ch.parent = replaceChunk ? mChunkData[chunkIndex].parent : chunkId;
	std::vector<ChunkInfo> xSlicedChunks;
	std::vector<ChunkInfo> ySlicedChunks;
	std::vector<uint32_t> newlyCreatedChunksIds;
	/**
	Slice along x direction
	*/
	for (int32_t slice = 0; slice < x_slices; ++slice)
	{
		PxVec3 randVect = PxVec3(2 * rnd->getRandomValue() - 1, 2 * rnd->getRandomValue() - 1, 2 * rnd->getRandomValue() - 1);
		PxVec3 lDir = dir + randVect * conf.angle_variations;

		setCuttingBox(center, lDir, slBox, 20, mPlaneIndexerOffset);
		bTool.performFastCutting(mesh, slBox, BooleanConfigurations::BOOLEAN_DIFFERENCE());
		ch.meshData = bTool.createNewMesh();

		if (ch.meshData != 0)
		{
			xSlicedChunks.push_back(ch);
		}
		inverseNormalAndSetIndices(slBox, -mPlaneIndexerOffset);
		++mPlaneIndexerOffset;
		bTool.performFastCutting(mesh, slBox, BooleanConfigurations::BOOLEAN_INTERSECION());
		Mesh* result = bTool.createNewMesh();
		delete mesh;
		mesh = result;
		if (mesh == nullptr)
		{
			break;
		}
		center.x += x_offset + (rnd->getRandomValue()) * conf.offset_variations * x_offset;
	}
	if (mesh != 0)
	{
		ch.meshData = mesh;
		xSlicedChunks.push_back(ch);
	}


	for (uint32_t chunk = 0; chunk < xSlicedChunks.size(); ++chunk)
	{
		center = PxVec3(0, sourceBBox.minimum.y, 0);
		center.y += y_offset;
		dir = PxVec3(0, 1, 0);
		mesh = xSlicedChunks[chunk].meshData;

		for (int32_t slice = 0; slice < y_slices; ++slice)
		{
			PxVec3 randVect = PxVec3(2 * rnd->getRandomValue() - 1, 2 * rnd->getRandomValue() - 1, 2 * rnd->getRandomValue() - 1);
			PxVec3 lDir = dir + randVect * conf.angle_variations;

			
			setCuttingBox(center, lDir, slBox, 20, mPlaneIndexerOffset);
			bTool.performFastCutting(mesh, slBox, BooleanConfigurations::BOOLEAN_DIFFERENCE());
			ch.meshData = bTool.createNewMesh();
			if (ch.meshData != 0)
			{
				ySlicedChunks.push_back(ch);
			}
			inverseNormalAndSetIndices(slBox, -mPlaneIndexerOffset);
			++mPlaneIndexerOffset;
			bTool.performFastCutting(mesh, slBox, BooleanConfigurations::BOOLEAN_INTERSECION());
			Mesh* result = bTool.createNewMesh();
			delete mesh;
			mesh = result;
			if (mesh == nullptr)
			{
				break;
			}
			center.y += y_offset + (rnd->getRandomValue()) * conf.offset_variations * y_offset;
		}
		if (mesh != 0)
		{
			ch.meshData = mesh;
			ySlicedChunks.push_back(ch);
		}
	}


	for (uint32_t chunk = 0; chunk < ySlicedChunks.size(); ++chunk)
	{
		center = PxVec3(0, 0, sourceBBox.minimum.z);
		center.z += z_offset;
		dir = PxVec3(0, 0, 1);
		mesh = ySlicedChunks[chunk].meshData;

		for (int32_t slice = 0; slice < z_slices; ++slice)
		{
			PxVec3 randVect = PxVec3(2 * rnd->getRandomValue() - 1, 2 * rnd->getRandomValue() - 1, 2 * rnd->getRandomValue() - 1);
			PxVec3 lDir = dir + randVect * conf.angle_variations;
			setCuttingBox(center, lDir, slBox, 20, mPlaneIndexerOffset);
			bTool.performFastCutting(mesh, slBox, BooleanConfigurations::BOOLEAN_DIFFERENCE());
			ch.meshData = bTool.createNewMesh();
			if (ch.meshData != 0)
			{
				ch.chunkId = mChunkIdCounter++;
				newlyCreatedChunksIds.push_back(ch.chunkId);
				mChunkData.push_back(ch);
			}
			inverseNormalAndSetIndices(slBox, -mPlaneIndexerOffset);
			++mPlaneIndexerOffset;
			bTool.performFastCutting(mesh, slBox, BooleanConfigurations::BOOLEAN_INTERSECION());
			Mesh* result = bTool.createNewMesh();
			delete mesh;
			mesh = result;
			if (mesh == nullptr)
			{
				break;
			}
			center.z += z_offset + (rnd->getRandomValue()) * conf.offset_variations * z_offset;
		}
		if (mesh != 0)
		{
			ch.chunkId = mChunkIdCounter++;
			ch.meshData = mesh;
			mChunkData.push_back(ch);
			newlyCreatedChunksIds.push_back(ch.chunkId);
		}
	}


	delete slBox;

	mChunkData[chunkIndex].isLeaf = false;
	if (replaceChunk)
	{
		eraseChunk(chunkId);
	}

	if (mRemoveIslands)
	{
		for (auto chunkToCheck : newlyCreatedChunksIds)
		{
			islandDetectionAndRemoving(chunkToCheck);
		}
	}

	return 0;
}

int32_t FractureTool::slicingNoisy(uint32_t chunkId, SlicingConfiguration conf, bool replaceChunk, RandomGeneratorBase* rnd)
{
	if (replaceChunk && chunkId == 0)
	{
		return 1;
	}

	int32_t chunkIndex = getChunkIndex(chunkId);
	if (chunkIndex == -1)
	{
		return 1;
	}
	if (!mChunkData[chunkIndex].isLeaf)
	{
		deleteAllChildsOfChunk(chunkId);
	}
	chunkIndex = getChunkIndex(chunkId);


	Mesh* mesh = new Mesh(*mChunkData[chunkIndex].meshData);

	BooleanEvaluator bTool(mLoggingCallback);

	int32_t x_slices = conf.x_slices;
	int32_t y_slices = conf.y_slices;
	int32_t z_slices = conf.z_slices;

	const PxBounds3 sourceBBox = mesh->getBoundingBox();

	PxVec3 center = PxVec3(mesh->getBoundingBox().minimum.x, 0, 0);


	float x_offset = (sourceBBox.maximum.x - sourceBBox.minimum.x) * (1.0f / (x_slices + 1));
	float y_offset = (sourceBBox.maximum.y - sourceBBox.minimum.y) * (1.0f / (y_slices + 1));
	float z_offset = (sourceBBox.maximum.z - sourceBBox.minimum.z) * (1.0f / (z_slices + 1));

	center.x += x_offset;

	PxVec3 dir(1, 0, 0);

	Mesh* slBox = nullptr;

	ChunkInfo ch;
	ch.isLeaf = true;
	ch.parent = replaceChunk ? mChunkData[chunkIndex].parent : chunkId;
	std::vector<ChunkInfo> xSlicedChunks;
	std::vector<ChunkInfo> ySlicedChunks;
	std::vector<uint32_t> newlyCreatedChunksIds;
	float noisyPartSize = 1.8f;
	int32_t acceleratorRes = 5;
	/**
		Slice along x direction
	*/
	for (int32_t slice = 0; slice < x_slices; ++slice)
	{
		PxVec3 randVect = PxVec3(2 * rnd->getRandomValue() - 1, 2 * rnd->getRandomValue() - 1, 2 * rnd->getRandomValue() - 1);
		PxVec3 lDir = dir + randVect * conf.angle_variations;
		slBox = getNoisyCuttingBoxPair(center, lDir, 40, noisyPartSize, conf.surfaceResolution, mPlaneIndexerOffset, conf.noiseAmplitude, conf.noiseFrequency, conf.noiseOctaveNumber, rnd->getRandomValue());
	//	DummyAccelerator accel(mesh->getFacetCount());
		IntersectionTestingAccelerator accel(mesh, acceleratorRes);
		DummyAccelerator dummy(slBox->getFacetCount());
		bTool.performBoolean(mesh, slBox, &accel, &dummy, BooleanConfigurations::BOOLEAN_DIFFERENCE());
		ch.meshData = bTool.createNewMesh();
		if (ch.meshData != 0)
		{
			xSlicedChunks.push_back(ch);
		}
		inverseNormalAndSetIndices(slBox, -mPlaneIndexerOffset);
		++mPlaneIndexerOffset;
		bTool.performBoolean(mesh, slBox, &accel, &dummy, BooleanConfigurations::BOOLEAN_INTERSECION());
		Mesh* result = bTool.createNewMesh();
		delete slBox;
		delete mesh;
		mesh = result;
		if (mesh == nullptr)
		{
			break;
		}
		center.x += x_offset + (rnd->getRandomValue()) * conf.offset_variations * x_offset;
	}
	if (mesh != 0)
	{
		ch.meshData = mesh;
		xSlicedChunks.push_back(ch);
	}
	slBox = getCuttingBox(center, dir, 20, 0);
	uint32_t slicedChunkSize = xSlicedChunks.size();
	for (uint32_t chunk = 0; chunk < slicedChunkSize; ++chunk)
	{
		center = PxVec3(0, sourceBBox.minimum.y, 0);
		center.y += y_offset;
		dir = PxVec3(0, 1, 0);
		mesh = xSlicedChunks[chunk].meshData;

		for (int32_t slice = 0; slice < y_slices; ++slice)
		{
			PxVec3 randVect = PxVec3(2 * rnd->getRandomValue() - 1, 2 * rnd->getRandomValue() - 1, 2 * rnd->getRandomValue() - 1);
			PxVec3 lDir = dir + randVect * conf.angle_variations;

			slBox = getNoisyCuttingBoxPair(center, lDir, 40, noisyPartSize, conf.surfaceResolution, mPlaneIndexerOffset, conf.noiseAmplitude, conf.noiseFrequency, conf.noiseOctaveNumber, rnd->getRandomValue());
		//	DummyAccelerator accel(mesh->getFacetCount());
			IntersectionTestingAccelerator accel(mesh, acceleratorRes);
			DummyAccelerator dummy(slBox->getFacetCount());
			bTool.performBoolean(mesh, slBox, &accel, &dummy, BooleanConfigurations::BOOLEAN_DIFFERENCE());
			ch.meshData = bTool.createNewMesh();
			if (ch.meshData != 0)
			{
				ySlicedChunks.push_back(ch);
			}
			inverseNormalAndSetIndices(slBox, -mPlaneIndexerOffset);
			++mPlaneIndexerOffset;
			bTool.performBoolean(mesh, slBox, &accel, &dummy, BooleanConfigurations::BOOLEAN_INTERSECION());
			Mesh* result = bTool.createNewMesh();
			delete slBox;
			delete mesh;
			mesh = result;
			if (mesh == nullptr)
			{
				break;
			}
			center.y += y_offset + (rnd->getRandomValue()) * conf.offset_variations * y_offset;
		}
		if (mesh != 0)
		{
			ch.meshData = mesh;
			ySlicedChunks.push_back(ch);
		}
	}

	for (uint32_t chunk = 0; chunk < ySlicedChunks.size(); ++chunk)
	{
		center = PxVec3(0, 0, sourceBBox.minimum.z);
		center.z += z_offset;
		dir = PxVec3(0, 0, 1);
		mesh = ySlicedChunks[chunk].meshData;

		for (int32_t slice = 0; slice < z_slices; ++slice)
		{
			PxVec3 randVect = PxVec3(2 * rnd->getRandomValue() - 1, 2 * rnd->getRandomValue() - 1, 2 * rnd->getRandomValue() - 1);
			PxVec3 lDir = dir + randVect * conf.angle_variations;
			slBox = getNoisyCuttingBoxPair(center, lDir, 40, noisyPartSize, conf.surfaceResolution, mPlaneIndexerOffset, conf.noiseAmplitude, conf.noiseFrequency, conf.noiseOctaveNumber, rnd->getRandomValue());
	//		DummyAccelerator accel(mesh->getFacetCount());
			IntersectionTestingAccelerator accel(mesh, acceleratorRes);
			DummyAccelerator dummy(slBox->getFacetCount());
			bTool.performBoolean(mesh, slBox, &accel, &dummy, BooleanConfigurations::BOOLEAN_DIFFERENCE());
			ch.meshData = bTool.createNewMesh();
			if (ch.meshData != 0)
			{
				ch.chunkId = mChunkIdCounter++;
				mChunkData.push_back(ch);
				newlyCreatedChunksIds.push_back(ch.chunkId);
			}
			inverseNormalAndSetIndices(slBox, -mPlaneIndexerOffset);
			++mPlaneIndexerOffset;
			bTool.performBoolean(mesh, slBox, &accel, &dummy, BooleanConfigurations::BOOLEAN_INTERSECION());
			Mesh* result = bTool.createNewMesh();
			delete mesh;
			delete slBox;
			mesh = result;
			if (mesh == nullptr)
			{
				break;
			}
			center.z += z_offset + (rnd->getRandomValue()) * conf.offset_variations * z_offset;
		}
		if (mesh != 0)
		{
			ch.chunkId = mChunkIdCounter++;
			ch.meshData = mesh;
			mChunkData.push_back(ch);
			newlyCreatedChunksIds.push_back(ch.chunkId);
		}
	}

//	delete slBox;

	mChunkData[chunkIndex].isLeaf = false;
	if (replaceChunk)
	{
		eraseChunk(chunkId);
	}

	if (mRemoveIslands)
	{
		for (auto chunkToCheck : newlyCreatedChunksIds)
		{
			islandDetectionAndRemoving(chunkToCheck);
		}
	}

	return 0;
}



int32_t FractureTool::getChunkIndex(int32_t chunkId)
{
	for (uint32_t i = 0; i < mChunkData.size(); ++i)
	{
		if (mChunkData[i].chunkId == chunkId)
		{
			return i;
		}
	}
	return -1;
}

int32_t FractureTool::getChunkDepth(int32_t chunkId)
{
	int32_t chunkIndex = getChunkIndex(chunkId);
	if (chunkIndex == -1)
	{
		return -1;
	}

	int32_t depth = 0;

	while (mChunkData[chunkIndex].parent != -1)
	{
		++depth;
		chunkIndex = getChunkIndex(mChunkData[chunkIndex].parent);
	}
	return depth;	
}

std::vector<int32_t> FractureTool::getChunksIdAtDepth(uint32_t depth)
{
	std::vector<int32_t> chunkIds;

	for (uint32_t i = 0; i < mChunkData.size(); ++i)
	{
		if (getChunkDepth(mChunkData[i].chunkId) == (int32_t)depth)
		{
			chunkIds.push_back(mChunkData[i].chunkId);
		}
	}
	return chunkIds;
}


void FractureTool::getTransformation(PxVec3& offset, float& scale)
{
	offset = mOffset;
	scale = mScaleFactor;
}

void FractureTool::setSourceMesh(Mesh* mesh)
{
	if (mesh == nullptr)
	{
		return;
	}
	reset();

	mChunkData.resize(1);
	mChunkData[0].meshData = new Mesh(*mesh);
	mChunkData[0].parent = -1;
	mChunkData[0].isLeaf = true;
	mChunkData[0].chunkId = mChunkIdCounter++;
	mesh = mChunkData[0].meshData;

	/**
	Move to origin and scale to unit cube
	*/

	mOffset = (mesh->getBoundingBox().maximum + mesh->getBoundingBox().minimum) * 0.5f;
	PxVec3 bbSizes = (mesh->getBoundingBox().maximum - mesh->getBoundingBox().minimum);

	mScaleFactor = std::max(bbSizes.x, std::max(bbSizes.y, bbSizes.z));

	Vertex* verticesBuffer = mesh->getVertices();
	for (uint32_t i = 0; i < mesh->getVerticesCount(); ++i)
	{
		verticesBuffer[i].p = (verticesBuffer[i].p - mOffset) * (1.0f / mScaleFactor);
	}

	mesh->getBoundingBox().minimum = (mesh->getBoundingBox().minimum - mOffset) * (1.0f / mScaleFactor);
	mesh->getBoundingBox().maximum = (mesh->getBoundingBox().maximum - mOffset) * (1.0f / mScaleFactor);


	for (uint32_t i = 0; i < mesh->getFacetCount(); ++i)
	{
		mesh->getFacet(i)->userData = 0; // Mark facet as initial boundary facet
	}
}


void FractureTool::reset()
{
	mChunkPostprocessors.clear();
	for (uint32_t i = 0; i < mChunkData.size(); ++i)
	{
		delete mChunkData[i].meshData;
	}
	mChunkData.clear();
	mPlaneIndexerOffset = 1;
	mChunkIdCounter = 0;
}




bool FractureTool::isAncestorForChunk(int32_t ancestorId, int32_t chunkId)
{
	if (ancestorId == chunkId)
	{
		return false;
	}
	while (chunkId != -1)
	{
		if (ancestorId == chunkId)
		{
			return true;
		}
		chunkId = getChunkIndex(chunkId);
		if (chunkId == -1)
		{
			return false;
		}
		chunkId = mChunkData[chunkId].parent;
	}
	return false;
}

void FractureTool::eraseChunk(int32_t chunkId)
{
	deleteAllChildsOfChunk(chunkId);
	int32_t index = getChunkIndex(chunkId);
	if (index != -1)
	{
		delete mChunkData[index].meshData;
		std::swap(mChunkData.back(), mChunkData[index]);
		mChunkData.pop_back();
	}
}


void FractureTool::deleteAllChildsOfChunk(int32_t chunkId)
{
	std::vector<int32_t> chunkToDelete;
	for (uint32_t i = 0; i < mChunkData.size(); ++i)
	{
		if (isAncestorForChunk(chunkId, mChunkData[i].chunkId))
		{
			chunkToDelete.push_back(i);
		}
	}
	for (int32_t i = (int32_t)chunkToDelete.size() - 1; i >= 0; --i)
	{
		int32_t m = chunkToDelete[i];
		delete mChunkData[m].meshData;
		std::swap(mChunkData.back(), mChunkData[m]);
		mChunkData.pop_back();
	}
}

void FractureTool::finalizeFracturing()
{
	for (uint32_t i = 0; i < mChunkPostprocessors.size(); ++i)
	{
		delete mChunkPostprocessors[i];
	}
	mChunkPostprocessors.resize(mChunkData.size());
	for (uint32_t i = 0; i < mChunkPostprocessors.size(); ++i)
	{
		mChunkPostprocessors[i] = new ChunkPostProcessor();
	}
	
	for (uint32_t i = 0; i < mChunkPostprocessors.size(); ++i)
	{
		mChunkPostprocessors[i]->triangulate(mChunkData[i].meshData);
	}
	std::vector<int32_t> badOnes;
	for (uint32_t i = 0; i < mChunkPostprocessors.size(); ++i)
	{
		if (mChunkPostprocessors[i]->getBaseMesh().empty())
		{
			badOnes.push_back(i);
		}
	}
	for (int32_t i = (int32_t)badOnes.size() - 1; i >= 0; --i)
	{
		int32_t chunkId = mChunkData[badOnes[i]].chunkId;
		for (uint32_t j = 0; j < mChunkData.size(); ++j)
		{
			if (mChunkData[j].parent == chunkId)
				mChunkData[j].parent = mChunkData[badOnes[i]].parent;
		}
		std::swap(mChunkPostprocessors[badOnes[i]], mChunkPostprocessors.back());
		mChunkPostprocessors.pop_back();
		std::swap(mChunkData[badOnes[i]], mChunkData.back());
		mChunkData.pop_back();
	}

}

const std::vector<ChunkInfo>& FractureTool::getChunkList()
{
	return mChunkData;
}

void FractureTool::getBaseMesh(int32_t chunkIndex, std::vector<Triangle>& output)
{
	NVBLAST_ASSERT(mChunkPostprocessors.size() > 0);
	if (mChunkPostprocessors.size() == 0)
	{		
		return; // finalizeFracturing() should be called before getting mesh!
	}
	output = mChunkPostprocessors[chunkIndex]->getBaseMesh();

	/* Scale mesh back */

	for (uint32_t i = 0; i < output.size(); ++i)
	{
		output[i].a.p = output[i].a.p * mScaleFactor + mOffset;
		output[i].b.p = output[i].b.p * mScaleFactor + mOffset;
		output[i].c.p = output[i].c.p * mScaleFactor + mOffset;
	}
}

void FractureTool::getNoisedMesh(int32_t chunkIndex, std::vector<Triangle>& output)
{
	NVBLAST_ASSERT(mChunkPostprocessors.size() > 0);
	if (mChunkPostprocessors.size() == 0)
	{
		return; // finalizeFracturing() should be called before getting mesh!
	}

	if (mChunkData[chunkIndex].chunkId == 0)
	{
		output = mChunkPostprocessors[chunkIndex]->getBaseMesh();
	}
	else
	{
		output = mChunkPostprocessors[chunkIndex]->getNoisyMesh();
	}

	for (uint32_t i = 0; i < output.size(); ++i)
	{
		output[i].a.p = output[i].a.p * mScaleFactor + mOffset;
		output[i].b.p = output[i].b.p * mScaleFactor + mOffset;
		output[i].c.p = output[i].c.p * mScaleFactor + mOffset;
	}
}

void FractureTool::tesselate(float averateEdgeLength)
{
	NVBLAST_ASSERT(mChunkPostprocessors.size() > 0);
	if (mChunkPostprocessors.size() == 0)
	{
		return; // finalizeFracturing() should be called before tesselation!
	}

	for (uint32_t i = 0; i < mChunkPostprocessors.size(); ++i)
	{
		if (mChunkData[i].chunkId == 0) // skip source mesh
		{
			continue;
		}
		mChunkPostprocessors[i]->tesselateInternalSurface(averateEdgeLength / mScaleFactor);
	}
}

void FractureTool::applyNoise(float amplitude, float frequency, int32_t octaves, float falloff, int32_t relaxIterations, float relaxFactor, int32_t seed)
{
	octaves = octaves <= 0 ? 1 : octaves;
	if (mChunkPostprocessors.empty())
	{
		return;
	}
	SimplexNoise noise(amplitude / mScaleFactor, frequency * mScaleFactor, octaves, seed);
	for (uint32_t i = 0; i < mChunkPostprocessors.size(); ++i)
	{
		if (mChunkData[i].chunkId == 0) // skip source mesh
		{
			continue;
		}
		mChunkPostprocessors[i]->applyNoise(noise, falloff, relaxIterations, relaxFactor);
	}
}

float getVolume(std::vector<Triangle>& triangles)
{
	float volume = 0.0f;

	for (uint32_t i = 0; i < triangles.size(); ++i)
	{
		PxVec3& a = triangles[i].a.p;
		PxVec3& b = triangles[i].b.p;
		PxVec3& c = triangles[i].c.p;
		volume += (a.x * b.y * c.z - a.x * b.z * c.y - a.y * b.x * c.z + a.y * b.z * c.x + a.z * b.x * c.y - a.z * b.y * c.x);
	}
	return (1.0f / 6.0f) * PxAbs(volume);
}

float FractureTool::getMeshOverlap(Mesh& meshA, Mesh& meshB)
{
	BooleanEvaluator bTool;
	bTool.performBoolean(&meshA, &meshB, BooleanConfigurations::BOOLEAN_INTERSECION());
	Mesh* result = bTool.createNewMesh();
	if (result == nullptr)
	{
		return 0.0f;
	}

	ChunkPostProcessor postProcessor;
	postProcessor.triangulate(&meshA);

	float baseVolume = getVolume(postProcessor.getBaseMesh());
	if (baseVolume == 0)
	{
		return 0.0f;
	}
	postProcessor.triangulate(result);
	float intrsVolume = getVolume(postProcessor.getBaseMesh());

	delete result;

	return intrsVolume / baseVolume;
}

void weldVertices(std::map<Vertex, uint32_t, VrtComp>& vertexMapping, std::vector<Vertex>& vertexBuffer, std::vector<uint32_t>& indexBuffer, std::vector<Triangle>& trb)
{
	for (uint32_t i = 0; i < trb.size(); ++i)
	{
		auto it = vertexMapping.find(trb[i].a);
		if (it == vertexMapping.end())
		{
			indexBuffer.push_back(static_cast<uint32_t>(vertexBuffer.size()));
			vertexMapping[trb[i].a] = static_cast<uint32_t>(vertexBuffer.size());
			vertexBuffer.push_back(trb[i].a);
		}
		else
		{
			indexBuffer.push_back(it->second);
		}
		it = vertexMapping.find(trb[i].b);
		if (it == vertexMapping.end())
		{
			indexBuffer.push_back(static_cast<uint32_t>(vertexBuffer.size()));
			vertexMapping[trb[i].b] = static_cast<uint32_t>(vertexBuffer.size());
			vertexBuffer.push_back(trb[i].b);
		}
		else
		{
			indexBuffer.push_back(it->second);
		}
		it = vertexMapping.find(trb[i].c);
		if (it == vertexMapping.end())
		{
			indexBuffer.push_back(static_cast<uint32_t>(vertexBuffer.size()));
			vertexMapping[trb[i].c] = static_cast<uint32_t>(vertexBuffer.size());
			vertexBuffer.push_back(trb[i].c);
		}
		else
		{
			indexBuffer.push_back(it->second);
		}
	}

}

void FractureTool::setRemoveIslands(bool isRemoveIslands)
{
	mRemoveIslands = isRemoveIslands;
}

int32_t FractureTool::islandDetectionAndRemoving(int32_t chunkId)
{
	if (chunkId == 0)
	{
		return 0;
	}
	int32_t chunkIndex = getChunkIndex(chunkId);
	ChunkPostProcessor prc;
	prc.triangulate(mChunkData[chunkIndex].meshData);

	Mesh* chunk = mChunkData[chunkIndex].meshData;

	std::vector<uint32_t>& mapping = prc.getBaseMapping();
	std::vector<TriangleIndexed>& trs = prc.getBaseMeshIndexed();

	std::vector<std::vector<uint32_t> > graph(prc.getWeldedVerticesCount());
	std::vector<int32_t>& pm = prc.getPositionedMapping();
	if (pm.size() == 0)
	{
		return 0;
	}

	/**
		Chunk graph
	*/
	for (uint32_t i = 0; i < trs.size(); ++i)
	{
		graph[pm[trs[i].ea]].push_back(pm[trs[i].eb]);
		graph[pm[trs[i].ea]].push_back(pm[trs[i].ec]);

		graph[pm[trs[i].ec]].push_back(pm[trs[i].eb]);
		graph[pm[trs[i].ec]].push_back(pm[trs[i].ea]);

		graph[pm[trs[i].eb]].push_back(pm[trs[i].ea]);
		graph[pm[trs[i].eb]].push_back(pm[trs[i].ec]);
	}
	for (uint32_t i = 0; i < chunk->getEdgesCount(); ++i)
	{
		int v1 = chunk->getEdges()[i].s;
		int v2 = chunk->getEdges()[i].e;

		v1 = pm[mapping[v1]];
		v2 = pm[mapping[v2]];

		graph[v1].push_back(v2);
		graph[v2].push_back(v1);

	}


	/**
		Walk graph, mark components
	*/

	std::vector<int32_t> comps(prc.getWeldedVerticesCount(), -1);
	std::queue<uint32_t> que;
	int32_t cComp = 0;
	
	for (uint32_t i = 0; i < prc.getWeldedVerticesCount(); ++i)
	{
		int32_t to = pm[i];
		if (comps[to] != -1) continue;
		que.push(to);
		comps[to] = cComp;

		while (!que.empty())
		{
			int32_t c = que.front();
			que.pop();
			
			for (uint32_t j = 0; j < graph[c].size(); ++j)
			{
				if (comps[graph[c][j]] == -1)
				{
					que.push(graph[c][j]);
					comps[graph[c][j]] = cComp;
				}
			}
		}
		cComp++;
	}
	for (uint32_t i = 0; i < prc.getWeldedVerticesCount(); ++i)
	{
		int32_t to = pm[i];
		comps[i] = comps[to];
	}
	std::vector<uint32_t> longComps(chunk->getVerticesCount());
	for (uint32_t i = 0; i < chunk->getVerticesCount(); ++i)
	{
		int32_t to = mapping[i];
		longComps[i] = comps[to];
	}
	
	if (cComp > 1)
	{
		std::vector<std::vector<Vertex> >	compVertices(cComp);
		std::vector<std::vector<Facet> >	compFacets(cComp);
		std::vector<std::vector<Edge> >		compEdges(cComp);


		std::vector<uint32_t>				compVertexMapping(chunk->getVerticesCount(), 0);
		Vertex* vrts = chunk->getVertices();
		for (uint32_t v = 0; v < chunk->getVerticesCount(); ++v)
		{
			int32_t vComp = comps[mapping[v]];
			compVertexMapping[v] = static_cast<uint32_t>(compVertices[vComp].size());
			compVertices[vComp].push_back(vrts[v]);
		}
		
		Facet* fcb = chunk->getFacetsBuffer();
		Edge* edb = chunk->getEdges();

		for (uint32_t fc = 0; fc < chunk->getFacetCount(); ++fc)
		{
			std::vector<uint32_t> edgesPerComp(cComp, 0);
			for (uint32_t ep = fcb[fc].firstEdgeNumber; ep < fcb[fc].firstEdgeNumber + fcb[fc].edgesCount; ++ep)
			{
				int32_t vComp = comps[mapping[edb[ep].s]];
				edgesPerComp[vComp]++;
				compEdges[vComp].push_back(Edge(compVertexMapping[edb[ep].s], compVertexMapping[edb[ep].e]));
			}
			for (int32_t c = 0; c < cComp; ++c)
			{
				if (edgesPerComp[c] == 0)
				{
					continue;
				}
				compFacets[c].push_back(*chunk->getFacet(fc));
				compFacets[c].back().edgesCount = edgesPerComp[c];
				compFacets[c].back().firstEdgeNumber = static_cast<int32_t>(compEdges[c].size()) - edgesPerComp[c];
			}
		}

		delete mChunkData[chunkIndex].meshData;
		mChunkData[chunkIndex].meshData = new Mesh(compVertices[0].data(), compEdges[0].data(), compFacets[0].data(), static_cast<uint32_t>(compVertices[0].size()),
													static_cast<uint32_t>(compEdges[0].size()), static_cast<uint32_t>(compFacets[0].size()));;
		for (int32_t i = 1; i < cComp; ++i)
		{
			mChunkData.push_back(ChunkInfo(mChunkData[chunkIndex]));
			mChunkData.back().chunkId = mChunkIdCounter++;
			mChunkData.back().meshData = new Mesh(compVertices[i].data(), compEdges[i].data(), compFacets[i].data(), static_cast<uint32_t>(compVertices[i].size()),
													static_cast<uint32_t>(compEdges[i].size()), static_cast<uint32_t>(compFacets[i].size()));
		}
		
		return cComp;
	}
	return 0;
}

void FractureTool::getBufferedBaseMeshes(std::vector<Vertex>& vertexBuffer, std::vector<std::vector<uint32_t> >& indexBuffer)
{
	std::map<Vertex, uint32_t, VrtComp> vertexMapping;
	vertexBuffer.clear();
	indexBuffer.clear();
	indexBuffer.resize(mChunkPostprocessors.size());

	for (uint32_t ch = 0; ch < mChunkPostprocessors.size(); ++ch)
	{
		std::vector<Triangle>& trb = mChunkPostprocessors[ch]->getBaseMesh();
		weldVertices(vertexMapping, vertexBuffer, indexBuffer[ch], trb);
	}
	for (uint32_t i = 0; i < vertexBuffer.size(); ++i)
	{
		vertexBuffer[i].p = vertexBuffer[i].p * mScaleFactor + mOffset;
	}
}

int32_t FractureTool::getChunkId(int32_t chunkIndex)
{
	if (chunkIndex < 0 || static_cast<uint32_t>(chunkIndex) >= mChunkData.size())
	{
		return -1;
	}
	return mChunkData[chunkIndex].chunkId;
}

void FractureTool::getBufferedNoiseMeshes(std::vector<Vertex>& vertexBuffer, std::vector<std::vector<uint32_t> >& indexBuffer)
{
	std::map<Vertex, uint32_t, VrtComp> vertexMapping;
	vertexBuffer.clear();
	indexBuffer.clear();
	indexBuffer.resize(mChunkPostprocessors.size());

	for (uint32_t ch = 0; ch < mChunkPostprocessors.size(); ++ch)
	{
		if (ch == 0)
		{
			std::vector<Triangle>& trb = mChunkPostprocessors[ch]->getBaseMesh();
			weldVertices(vertexMapping, vertexBuffer, indexBuffer[ch], trb);
		}
		else
		{
			std::vector<Triangle>& trb = mChunkPostprocessors[ch]->getNoisyMesh();
			weldVertices(vertexMapping, vertexBuffer, indexBuffer[ch], trb);
		}				
	}
	for (uint32_t i = 0; i < vertexBuffer.size(); ++i)
	{
		vertexBuffer[i].p = vertexBuffer[i].p * mScaleFactor + mOffset;
	}
}


} // namespace Blast
} // namespace Nv

