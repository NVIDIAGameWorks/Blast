/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastExtAuthoring.h"
#include "NvBlastExtAuthoringMeshImpl.h"
#include "NvBlastExtAuthoringMeshCleanerImpl.h"
#include "NvBlastExtAuthoringFractureToolImpl.h"
#include "NvBlastExtAuthoringCollisionBuilderImpl.h"
#include "NvBlastExtAuthoringBondGeneratorImpl.h"
#include "NvBlastTypes.h"
#include "NvBlastIndexFns.h"
#include "NvBlast.h"
#include "NvBlastGlobals.h"
#include "NvBlastExtPxAsset.h"

#include <algorithm>
#include <memory>

using namespace Nv::Blast;
using namespace physx;

#define SAFE_ARRAY_NEW(T, x) ((x) > 0) ? new T[x] : nullptr;
#define SAFE_ARRAY_DELETE(x) if (x != nullptr) {delete[] x; x = nullptr;}

Mesh* NvBlastExtAuthoringCreateMesh(const PxVec3* position, const PxVec3* normals, const PxVec2* uv, uint32_t verticesCount, const uint32_t* indices, uint32_t indicesCount)
{
	return new MeshImpl(position, normals, uv, verticesCount, indices, indicesCount);
}

MeshCleaner* NvBlastExtAuthoringCreateMeshCleaner()
{
	return new MeshCleanerImpl;
}

VoronoiSitesGenerator* NvBlastExtAuthoringCreateVoronoiSitesGenerator(Mesh* mesh, RandomGeneratorBase* rng)
{
	return new VoronoiSitesGeneratorImpl(mesh, rng);
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

void buildPhysicsChunks(ConvexMeshBuilder& collisionBuilder, AuthoringResult& result)
{
	uint32_t chunkCount = (uint32_t)result.chunkCount;
	result.collisionHullOffset = SAFE_ARRAY_NEW(uint32_t, chunkCount + 1);
	result.collisionHullOffset[0] = 0;
	result.collisionHull = SAFE_ARRAY_NEW(CollisionHull*, chunkCount);
	result.physicsSubchunks = SAFE_ARRAY_NEW(ExtPxSubchunk, chunkCount);
	result.physicsChunks = SAFE_ARRAY_NEW(ExtPxChunk, chunkCount);
	for (uint32_t i = 0; i < chunkCount; ++i)
	{
		std::vector<physx::PxVec3> vertices;
		for (uint32_t p = result.geometryOffset[i]; p < result.geometryOffset[i+1]; ++p)
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

AuthoringResult* NvBlastExtAuthoringProcessFracture(FractureTool& fTool, BlastBondGenerator& bondGenerator, ConvexMeshBuilder& collisionBuilder, int32_t defaultSupportDepth)
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
		aResult.assetToFractureChunkIdMap[i] = chunkIndex;
	}
	aResult.geometry = SAFE_ARRAY_NEW(Triangle, aResult.geometryOffset[chunkCount]);
	for (uint32_t i = 0; i < chunkCount; ++i)
	{
		uint32_t trianglesCount = aResult.geometryOffset[i + 1] - aResult.geometryOffset[i];
		memcpy(aResult.geometry + aResult.geometryOffset[i], chunkGeometry[i], trianglesCount * sizeof(Nv::Blast::Triangle));
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
	buildPhysicsChunks(collisionBuilder, aResult);

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