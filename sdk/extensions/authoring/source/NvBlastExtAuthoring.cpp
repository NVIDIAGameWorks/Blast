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

#include "NvBlastExtAuthoring.h"
#include "NvBlastExtAuthoringMeshImpl.h"
#include "NvBlastExtAuthoringMeshCleanerImpl.h"
#include "NvBlastExtAuthoringFractureToolImpl.h"
#include "NvBlastExtAuthoringCollisionBuilderImpl.h"
#include "NvBlastExtAuthoringBondGeneratorImpl.h"
#include "NvBlastExtAuthoringCutoutImpl.h"
#include "NvBlastTypes.h"
#include "NvBlastIndexFns.h"
#include "NvBlast.h"
#include "NvBlastGlobals.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlastExtAssetUtils.h"

#include <algorithm>
#include <memory>

using namespace Nv::Blast;
using namespace physx;

#define SAFE_ARRAY_NEW(T, x) ((x) > 0) ? reinterpret_cast<T*>(NVBLAST_ALLOC(sizeof(T) * (x))) : nullptr;
#define SAFE_ARRAY_DELETE(x) if (x != nullptr) {NVBLAST_FREE(x); x = nullptr;}

Mesh* NvBlastExtAuthoringCreateMesh(const PxVec3* position, const PxVec3* normals, const PxVec2* uv, uint32_t verticesCount, const uint32_t* indices, uint32_t indicesCount)
{
	return new MeshImpl(position, normals, uv, verticesCount, indices, indicesCount);
}

Mesh* NvBlastExtAuthoringCreateMeshFromFacets(const void* vertices, const void* edges, const void* facets, uint32_t verticesCount, uint32_t edgesCount, uint32_t facetsCount)
{
	return new MeshImpl((Vertex*)vertices, (Edge*)edges, (Facet*)facets, verticesCount, edgesCount, facetsCount);
}

MeshCleaner* NvBlastExtAuthoringCreateMeshCleaner()
{
	return new MeshCleanerImpl;
}

VoronoiSitesGenerator* NvBlastExtAuthoringCreateVoronoiSitesGenerator(Mesh* mesh, RandomGeneratorBase* rng)
{
	return new VoronoiSitesGeneratorImpl(mesh, rng);
}

CutoutSet* NvBlastExtAuthoringCreateCutoutSet()
{
	return new CutoutSetImpl();
}

void NvBlastExtAuthoringBuildCutoutSet(CutoutSet& cutoutSet, const uint8_t* pixelBuffer, uint32_t bufferWidth, uint32_t bufferHeight, 
	float segmentationErrorThreshold, float snapThreshold, bool periodic, bool expandGaps)
{
	::createCutoutSet(*(CutoutSetImpl*)&cutoutSet, pixelBuffer, bufferWidth, bufferHeight, segmentationErrorThreshold, snapThreshold, periodic, expandGaps);
}

FractureTool* NvBlastExtAuthoringCreateFractureTool()
{
	return new FractureToolImpl;
}

BlastBondGenerator* NvBlastExtAuthoringCreateBondGenerator(PxCooking* cooking, PxPhysicsInsertionCallback* insertionCallback)
{
	return new BlastBondGeneratorImpl(cooking, insertionCallback);
}

ConvexMeshBuilder* NvBlastExtAuthoringCreateConvexMeshBuilder(PxCooking* cooking, PxPhysicsInsertionCallback* insertionCallback)
{
	return new ConvexMeshBuilderImpl(cooking, insertionCallback);
}


void NvBlastExtAuthoringTransformCollisionHullInPlace(CollisionHull* hull, const physx::PxVec3* scaling, const physx::PxQuat* rotation, const physx::PxVec3* translation)
{
	// Local copies of scaling (S), rotation (R), and translation (T)
	physx::PxVec3 S = { 1, 1, 1 };
	physx::PxQuat R = { 0, 0, 0, 1 };
	physx::PxVec3 T = { 0, 0, 0 };
	physx::PxVec3 cofS = { 1, 1, 1 };
	float sgnDetS = 1;

	{
		if (rotation)
		{
			R = *rotation;
		}

		if (scaling)
		{
			S = *scaling;
			cofS.x = S.y * S.z;
			cofS.y = S.z * S.x;
			cofS.z = S.x * S.y;
			sgnDetS = (S.x * S.y * S.z < 0) ? -1 : 1;
		}

		if (translation)
		{
			T = *translation;
		}
	}

	const uint32_t pointCount = hull->pointsCount;
	for (uint32_t pi = 0; pi < pointCount; pi++)
	{
		physx::PxVec3& p = hull->points[pi];
		p = (R.rotate(p.multiply(S)) + T);
	}
	
	const uint32_t planeCount = hull->polygonDataCount;
	for (uint32_t pi = 0; pi < planeCount; pi++)
	{
		float* plane = hull->polygonData[pi].mPlane;
		physx::PxPlane pxPlane(plane[0], plane[1], plane[2], plane[3]);
		PxVec3 transformedNormal = sgnDetS*R.rotate(pxPlane.n.multiply(cofS)).getNormalized();
		PxVec3 transformedPt = R.rotate(pxPlane.pointInPlane().multiply(S)) + T;
		
		physx::PxPlane transformedPlane(transformedPt, transformedNormal);
		plane[0] = transformedPlane.n[0];
		plane[1] = transformedPlane.n[1];
		plane[2] = transformedPlane.n[2];
		plane[3] = transformedPlane.d;
	}
}


CollisionHull* NvBlastExtAuthoringTransformCollisionHull(const CollisionHull* hull, const physx::PxVec3* scaling, const physx::PxQuat* rotation, const physx::PxVec3* translation)
{
	CollisionHullImpl* ret = new CollisionHullImpl(*hull);
	NvBlastExtAuthoringTransformCollisionHullInPlace(ret, scaling, rotation, translation);
	return ret;
}

void buildPhysicsChunks(ConvexMeshBuilder& collisionBuilder, AuthoringResult& result, const CollisionParams& params, uint32_t chunksToProcessCount = 0, uint32_t* chunksToProcess = nullptr)
{
	uint32_t chunkCount = (uint32_t)result.chunkCount;
	if (params.maximumNumberOfHulls == 1)
	{
		result.collisionHullOffset = SAFE_ARRAY_NEW(uint32_t, chunkCount + 1);
		result.collisionHullOffset[0] = 0;
		result.collisionHull = SAFE_ARRAY_NEW(CollisionHull*, chunkCount);
		result.physicsSubchunks = SAFE_ARRAY_NEW(ExtPxSubchunk, chunkCount);
		result.physicsChunks = SAFE_ARRAY_NEW(ExtPxChunk, chunkCount);
		for (uint32_t i = 0; i < chunkCount; ++i)
		{
			std::vector<physx::PxVec3> vertices;
			for (uint32_t p = result.geometryOffset[i]; p < result.geometryOffset[i + 1]; ++p)
			{
				Nv::Blast::Triangle& tri = result.geometry[p];
				vertices.push_back(tri.a.p);
				vertices.push_back(tri.b.p);
				vertices.push_back(tri.c.p);
			}
			result.collisionHullOffset[i + 1] = result.collisionHullOffset[i] + 1;
			result.collisionHull[i] = collisionBuilder.buildCollisionGeometry((uint32_t)vertices.size(), vertices.data());
			result.physicsSubchunks[i].transform = physx::PxTransform(physx::PxIdentity);
			result.physicsSubchunks[i].geometry = physx::PxConvexMeshGeometry(collisionBuilder.buildConvexMesh(*result.collisionHull[i]));

			result.physicsChunks[i].isStatic = false;
			result.physicsChunks[i].subchunkCount = 1;
			result.physicsChunks[i].firstSubchunkIndex = i;
			//outPhysicsChunks.get()[i].subchunks = &outPhysicsSubchunks[i];
		}
	}
	else
	{
		std::set<int32_t> chunkSet;
		for (uint32_t c = 0; c < chunksToProcessCount; c++)
		{
			chunkSet.insert(chunksToProcess[c]);
		}
		std::vector<std::vector<CollisionHull*> > hulls(chunkCount);
		int32_t totalHulls = 0;
		for (uint32_t i = 0; i < chunkCount; ++i)
		{
			if (chunkSet.size() > 0 && chunkSet.find(i) == chunkSet.end())
			{
				int32_t newHulls = result.collisionHullOffset[i + 1] - result.collisionHullOffset[i];
				int32_t off = result.collisionHullOffset[i];
				for (int32_t subhull = 0; subhull < newHulls; ++subhull)
				{
					hulls[i].push_back(result.collisionHull[off + subhull]);
				}
				totalHulls += newHulls;
				continue;
			}

			CollisionHull** tempHull;

			int32_t newHulls = collisionBuilder.buildMeshConvexDecomposition(result.geometry + result.geometryOffset[i], 
												result.geometryOffset[i + 1] - result.geometryOffset[i], params, tempHull);
			totalHulls += newHulls;			
			for (int32_t h = 0; h < newHulls; ++h)
			{
				hulls[i].push_back(tempHull[h]);
			}
			SAFE_ARRAY_DELETE(tempHull);
		}

		SAFE_ARRAY_DELETE(result.collisionHullOffset);
		SAFE_ARRAY_DELETE(result.collisionHull);
		SAFE_ARRAY_DELETE(result.physicsSubchunks);
		SAFE_ARRAY_DELETE(result.physicsChunks);

		result.collisionHullOffset = SAFE_ARRAY_NEW(uint32_t, chunkCount + 1);
		result.collisionHullOffset[0] = 0;
		result.collisionHull = SAFE_ARRAY_NEW(CollisionHull*, totalHulls);
		result.physicsSubchunks = SAFE_ARRAY_NEW(ExtPxSubchunk, totalHulls);
		result.physicsChunks = SAFE_ARRAY_NEW(ExtPxChunk, chunkCount);

		int32_t firstSubchunk = 0;
		for (uint32_t i = 0; i < chunkCount; ++i)
		{
			result.collisionHullOffset[i + 1] = result.collisionHullOffset[i] + hulls[i].size();
			int32_t off = result.collisionHullOffset[i];
			for (uint32_t subhull = 0; subhull < hulls[i].size(); ++subhull)
			{
				result.collisionHull[off + subhull] = hulls[i][subhull];
				result.physicsSubchunks[firstSubchunk + subhull].transform = physx::PxTransform(physx::PxIdentity);
				result.physicsSubchunks[firstSubchunk + subhull].geometry = physx::PxConvexMeshGeometry(collisionBuilder.buildConvexMesh(*hulls[i][subhull]));
			}			
			result.physicsChunks[i].isStatic = false;
			result.physicsChunks[i].subchunkCount = static_cast<uint32_t>(hulls[i].size());
			result.physicsChunks[i].firstSubchunkIndex = firstSubchunk;
			firstSubchunk += result.physicsChunks[i].subchunkCount;
		}
	}
}


struct AuthoringResultImpl : public AuthoringResult
{
	void releaseCollisionHulls() override
	{
		if (collisionHull != nullptr)
		{
			for (uint32_t ch = 0; ch < collisionHullOffset[chunkCount]; ch++)
			{
				collisionHull[ch]->release();
			}
			SAFE_ARRAY_DELETE(collisionHullOffset);
			SAFE_ARRAY_DELETE(collisionHull);
		}
	}

	void release() override
	{
		releaseCollisionHulls();
		NVBLAST_FREE(asset);
		SAFE_ARRAY_DELETE(assetToFractureChunkIdMap);
		SAFE_ARRAY_DELETE(geometryOffset);
		SAFE_ARRAY_DELETE(geometry);
		SAFE_ARRAY_DELETE(chunkDescs);
		SAFE_ARRAY_DELETE(bondDescs);
		SAFE_ARRAY_DELETE(physicsChunks);
		SAFE_ARRAY_DELETE(physicsSubchunks);
		delete this;
	}
};

AuthoringResult* NvBlastExtAuthoringProcessFracture(FractureTool& fTool, BlastBondGenerator& bondGenerator, ConvexMeshBuilder& collisionBuilder, const CollisionParams& collisionParam, int32_t defaultSupportDepth)
{
	fTool.finalizeFracturing();
	const uint32_t chunkCount = fTool.getChunkCount();
	if (chunkCount == 0)
	{
		return nullptr;
	}
	AuthoringResultImpl* ret = new AuthoringResultImpl;
	if (ret == nullptr)
	{
		return nullptr;
	}
	AuthoringResult& aResult = *ret;
	aResult.chunkCount = chunkCount;

	std::shared_ptr<bool> isSupport(new bool[chunkCount], [](bool* b) {delete[] b; });
	memset(isSupport.get(), 0, sizeof(bool) * chunkCount);
	for (uint32_t i = 0; i < fTool.getChunkCount(); ++i)
	{
		if (defaultSupportDepth < 0 || fTool.getChunkDepth(fTool.getChunkId(i)) < defaultSupportDepth)
		{
			isSupport.get()[i] = fTool.getChunkInfo(i).isLeaf;
		}
		else if (fTool.getChunkDepth(fTool.getChunkId(i)) == defaultSupportDepth)
		{
			isSupport.get()[i] = true;
		}
	}

	BondGenerationConfig cnf;
	cnf.bondMode = BondGenerationConfig::EXACT;

	//NvBlastChunkDesc>& chunkDescs = aResult.chunkDescs;
	//std::shared_ptr<NvBlastBondDesc>& bondDescs = aResult.bondDescs;
	const uint32_t bondCount = bondGenerator.buildDescFromInternalFracture(&fTool, isSupport.get(), aResult.bondDescs, aResult.chunkDescs);
	aResult.bondCount = bondCount;
	if (bondCount == 0)
	{
		aResult.bondDescs = nullptr;
	}

	// order chunks, build map
	std::vector<uint32_t> chunkReorderInvMap;
	{
		std::vector<uint32_t> chunkReorderMap(chunkCount);
		std::vector<char> scratch(chunkCount * sizeof(NvBlastChunkDesc));
		NvBlastEnsureAssetExactSupportCoverage(aResult.chunkDescs, chunkCount, scratch.data(), logLL);
		NvBlastBuildAssetDescChunkReorderMap(chunkReorderMap.data(), aResult.chunkDescs, chunkCount, scratch.data(), logLL);
		NvBlastApplyAssetDescChunkReorderMapInPlace(aResult.chunkDescs, chunkCount, aResult.bondDescs, bondCount, chunkReorderMap.data(), true, scratch.data(), logLL);
		chunkReorderInvMap.resize(chunkReorderMap.size());
		Nv::Blast::invertMap(chunkReorderInvMap.data(), chunkReorderMap.data(), static_cast<unsigned int>(chunkReorderMap.size()));
	}

	// get result geometry
	aResult.geometryOffset = SAFE_ARRAY_NEW(uint32_t, chunkCount + 1);
	aResult.assetToFractureChunkIdMap = SAFE_ARRAY_NEW(uint32_t, chunkCount + 1);
	aResult.geometryOffset[0] = 0;
	std::vector<Nv::Blast::Triangle*> chunkGeometry(chunkCount);
	for (uint32_t i = 0; i < chunkCount; ++i)
	{
		uint32_t chunkIndex = chunkReorderInvMap[i];
		aResult.geometryOffset[i+1] = aResult.geometryOffset[i] + fTool.getBaseMesh(chunkIndex, chunkGeometry[i]);
		aResult.assetToFractureChunkIdMap[i] = fTool.getChunkId(chunkIndex);
	}
	aResult.geometry = SAFE_ARRAY_NEW(Triangle, aResult.geometryOffset[chunkCount]);
	for (uint32_t i = 0; i < chunkCount; ++i)
	{
		uint32_t trianglesCount = aResult.geometryOffset[i + 1] - aResult.geometryOffset[i];
		memcpy(aResult.geometry + aResult.geometryOffset[i], chunkGeometry[i], trianglesCount * sizeof(Nv::Blast::Triangle));
		delete chunkGeometry[i];
		chunkGeometry[i] = nullptr;
	}

	float maxX = INT32_MIN;
	float maxY = INT32_MIN;
	float maxZ = INT32_MIN;

	float minX = INT32_MAX;
	float minY = INT32_MAX;
	float minZ = INT32_MAX;

	for (uint32_t i = 0; i < bondCount; i++)
	{
		NvBlastBondDesc& bondDesc = aResult.bondDescs[i];

		minX = std::min(minX, bondDesc.bond.centroid[0]);
		maxX = std::max(maxX, bondDesc.bond.centroid[0]);

		minY = std::min(minY, bondDesc.bond.centroid[1]);
		maxY = std::max(maxY, bondDesc.bond.centroid[1]);

		minZ = std::min(minZ, bondDesc.bond.centroid[2]);
		maxZ = std::max(maxZ, bondDesc.bond.centroid[2]);
	}

	//std::cout << "Bond bounds: " << std::endl;
	//std::cout << "MIN: " << minX << ", " << minY << ", " << minZ << std::endl;
	//std::cout << "MAX: " << maxX << ", " << maxY << ", " << maxZ << std::endl;

	// prepare physics data (convexes)
	buildPhysicsChunks(collisionBuilder, aResult, collisionParam);

	// set NvBlastChunk volume from Px geometry
	for (uint32_t i = 0; i < chunkCount; i++)
	{
		float totalVolume = 0.f;
		for (uint32_t k = 0; k < aResult.physicsChunks[i].subchunkCount; k++)
		{
			const auto& subChunk = aResult.physicsSubchunks[aResult.physicsChunks[i].firstSubchunkIndex + k];
			physx::PxVec3 localCenterOfMass; physx::PxMat33 intertia; float mass;
			subChunk.geometry.convexMesh->getMassInformation(mass, intertia, localCenterOfMass);
			const physx::PxVec3 scale = subChunk.geometry.scale.scale;
			mass *= scale.x * scale.y * scale.z;
			totalVolume += mass / 1.0f; // unit density
		}

		aResult.chunkDescs[i].volume = totalVolume;
	}

	// build and serialize ExtPhysicsAsset
	NvBlastAssetDesc	descriptor;
	descriptor.bondCount = bondCount;
	descriptor.bondDescs = aResult.bondDescs;
	descriptor.chunkCount = chunkCount;
	descriptor.chunkDescs = aResult.chunkDescs;

	std::vector<uint8_t> scratch(static_cast<unsigned int>(NvBlastGetRequiredScratchForCreateAsset(&descriptor, logLL)));
	void* mem = NVBLAST_ALLOC(NvBlastGetAssetMemorySize(&descriptor, logLL));
	aResult.asset = NvBlastCreateAsset(mem, &descriptor, scratch.data(), logLL);

	//aResult.asset = std::shared_ptr<NvBlastAsset>(asset, [=](NvBlastAsset* asset)
	//{
	//	NVBLAST_FREE(asset);
	//});

	//std::cout << "Done" << std::endl;
	ret->materialCount = 0;
	ret->materialNames = nullptr;
	return ret;
}

uint32_t NvBlastExtAuthoringFindAssetConnectingBonds
(
	const NvBlastAsset** components,
	const physx::PxVec3* scales,
	const physx::PxQuat* rotations,
	const physx::PxVec3* translations,
	const uint32_t** convexHullOffsets,
	const CollisionHull*** chunkHulls,
	uint32_t componentCount,
	NvBlastExtAssetUtilsBondDesc*& newBondDescs,
	float maxSeparation
)
{
	//We don't need to use any of the cooking related parts of this
	BlastBondGeneratorImpl bondGenerator(nullptr, nullptr);

	std::vector<uint32_t> componentChunkOffsets;
	componentChunkOffsets.reserve(componentCount + 1);
	componentChunkOffsets.push_back(0);
	
	std::vector<uint32_t> combinedConvexHullOffsets;
	std::vector<const CollisionHull*> combinedConvexHulls;
	std::vector<CollisionHull*> hullsToRelease;
	combinedConvexHullOffsets.push_back(0);

	std::vector<uint32_t> originalComponentIndex;

	const physx::PxVec3 identityScale(1);

	//Combine our hull lists into a single combined list for bondsFromPrefractured
	for (uint32_t c = 0; c < componentCount; c++)
	{
		const uint32_t chunkCount = NvBlastAssetGetChunkCount(components[c], &logLL);
		const physx::PxVec3* scale = scales ? scales + c : nullptr;
		const physx::PxQuat* rotation = rotations ? rotations + c : nullptr;
		const physx::PxVec3* translation = translations ? translations + c : nullptr;

		componentChunkOffsets.push_back(chunkCount + componentChunkOffsets.back());
		for (uint32_t chunk = 0; chunk < chunkCount; chunk++)
		{
			const uint32_t hullsStart = convexHullOffsets[c][chunk];
			const uint32_t hullsEnd = convexHullOffsets[c][chunk + 1];
			for (uint32_t hull = hullsStart; hull < hullsEnd; hull++)
			{
				if ((scale != nullptr && *scale != identityScale) || (rotation != nullptr && !rotation->isIdentity()) || (translation != nullptr && !translation->isZero()))
				{
					hullsToRelease.emplace_back(NvBlastExtAuthoringTransformCollisionHull(chunkHulls[c][hull], scale, rotation, translation));
					combinedConvexHulls.emplace_back(hullsToRelease.back());
				}
				else
				{
					//No need to transform
					combinedConvexHulls.emplace_back(chunkHulls[c][hull]);
				}
			}
			combinedConvexHullOffsets.push_back((hullsEnd - hullsStart) + combinedConvexHullOffsets.back());
			originalComponentIndex.push_back(c);
		}
	}
	const uint32_t totalChunkCount = componentChunkOffsets.back();
	//Can't use std::vector<bool> since we need a bool* later
	std::unique_ptr<bool[]> isSupportChunk(new bool[totalChunkCount]);
	for (uint32_t c = 0; c < componentCount; c++)
	{
		const uint32_t chunkCount = componentChunkOffsets[c + 1] - componentChunkOffsets[c];
		NvBlastSupportGraph supportGraph = NvBlastAssetGetSupportGraph(components[c], &logLL);
		for (uint32_t chunk = 0; chunk < chunkCount; chunk++)
		{
			auto chunkIndiciesEnd = supportGraph.chunkIndices + supportGraph.nodeCount;
			isSupportChunk[chunk + componentChunkOffsets[c]] = (std::find(supportGraph.chunkIndices, chunkIndiciesEnd, chunk) != chunkIndiciesEnd);
		}
	}

	//Find the bonds
	NvBlastBondDesc* newBonds = nullptr;
	const int32_t newBoundCount = bondGenerator.bondsFromPrefractured(totalChunkCount, combinedConvexHullOffsets.data(), combinedConvexHulls.data(), isSupportChunk.get(), originalComponentIndex.data(), newBonds, maxSeparation);

	//Convert the bonds back to per-component chunks
	newBondDescs = SAFE_ARRAY_NEW(NvBlastExtAssetUtilsBondDesc, newBoundCount);
	for (int32_t nb = 0; nb < newBoundCount; ++nb)
	{
		newBondDescs[nb].bond = newBonds[nb].bond;
		for (uint32_t ci = 0; ci < 2; ++ci)
		{
			uint32_t absChunkIdx = newBonds[nb].chunkIndices[ci];
			uint32_t componentIdx = originalComponentIndex[absChunkIdx];
			newBondDescs[nb].componentIndices[ci] = componentIdx;
			newBondDescs[nb].chunkIndices[ci] = absChunkIdx - componentChunkOffsets[componentIdx];
		}
	}
	//Don't need this anymore
	NVBLAST_FREE(newBonds);

	for (CollisionHull* hull : hullsToRelease)
	{
		hull->release();
	}

	return newBoundCount;
}


void NvBlastExtAuthoringUpdateGraphicsMesh(Nv::Blast::FractureTool& fTool, Nv::Blast::AuthoringResult& aResult)
{
	uint32_t chunkCount = fTool.getChunkCount();
	for (uint32_t i = 0; i < chunkCount; ++i)
	{
		fTool.updateBaseMesh(fTool.getChunkIndex(aResult.assetToFractureChunkIdMap[i]), aResult.geometry + aResult.geometryOffset[i]);
	}
}

void NvBlastExtAuthoringBuildCollisionMeshes(Nv::Blast::AuthoringResult& ares, Nv::Blast::ConvexMeshBuilder& collisionBuilder,
	const Nv::Blast::CollisionParams& collisionParam, uint32_t chunksToProcessCount, uint32_t* chunksToProcess)
{
	buildPhysicsChunks(collisionBuilder, ares, collisionParam, chunksToProcessCount, chunksToProcess);
}
