/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "BlastAssetBoxes.h"
#include "BlastFamilyBoxes.h"
#include "NvBlastExtPxAsset.h"
#include "PxPhysics.h"
#include "cooking/PxCooking.h"


BlastAssetBoxes::BlastAssetBoxes(TkFramework& framework, PxPhysics& physics, PxCooking& cooking, Renderer& renderer, const Desc& desc)
	: BlastAsset(renderer)
{
	// generate boxes slices procedurally
	CubeAssetGenerator::generate(m_generatorAsset, desc.generatorSettings);

	// asset desc / tk asset
	ExtPxAssetDesc assetDesc;
	assetDesc.chunkDescs = &m_generatorAsset.solverChunks[0];
	assetDesc.chunkCount = (uint32_t)m_generatorAsset.solverChunks.size();
	assetDesc.bondDescs = m_generatorAsset.solverBonds.size() > 0 ? &m_generatorAsset.solverBonds[0] : nullptr;
	assetDesc.bondCount = (uint32_t)m_generatorAsset.solverBonds.size();
	std::vector<uint8_t> bondFlags(assetDesc.bondCount);
	std::fill(bondFlags.begin(), bondFlags.end(), desc.jointAllBonds ? 1 : 0);
	assetDesc.bondFlags = bondFlags.data();

	// box convex
	PxVec3 vertices[8] = { { -1, -1, -1 }, { -1, -1, 1 }, { -1, 1, -1 }, { -1, 1, 1 }, { 1, -1, -1 }, { 1, -1, 1 }, { 1, 1, -1 }, { 1, 1, 1 } };
	PxConvexMeshDesc convexMeshDesc;
	convexMeshDesc.points.count = 8;
	convexMeshDesc.points.data = vertices;
	convexMeshDesc.points.stride = sizeof(PxVec3);
	convexMeshDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;
	m_boxMesh = cooking.createConvexMesh(convexMeshDesc, physics.getPhysicsInsertionCallback());

	// prepare chunks
	const uint32_t chunkCount = (uint32_t)m_generatorAsset.solverChunks.size();
	std::vector<ExtPxAssetDesc::ChunkDesc> pxChunks(chunkCount);
	std::vector<ExtPxAssetDesc::SubchunkDesc> pxSubchunks;
	pxSubchunks.reserve(chunkCount);
	for (uint32_t i = 0; i < m_generatorAsset.solverChunks.size(); i++)
	{
		uint32_t chunkID = m_generatorAsset.solverChunks[i].userData;
		GeneratorAsset::BlastChunkCube& cube = m_generatorAsset.chunks[chunkID];
		PxVec3 position = *reinterpret_cast<PxVec3*>(&cube.position);
		PxVec3 extents = *reinterpret_cast<PxVec3*>(&cube.extents);
		ExtPxAssetDesc::ChunkDesc& chunk = pxChunks[chunkID];
		ExtPxAssetDesc::SubchunkDesc subchunk =
		{
			PxTransform(position),
			PxConvexMeshGeometry(m_boxMesh, PxMeshScale(extents / 2))
		};
		pxSubchunks.push_back(subchunk);
		chunk.subchunks = &pxSubchunks.back();
		chunk.subchunkCount = 1;
		chunk.isStatic = (position.y - (extents.y - desc.generatorSettings.extents.y) / 2) <= desc.staticHeight;
	}

	// create asset
	assetDesc.pxChunks = pxChunks.data();
	m_pxAsset = ExtPxAsset::create(assetDesc, framework);

	validate();
}


BlastAssetBoxes::~BlastAssetBoxes()
{
	m_boxMesh->release();
	m_pxAsset->release();
}


BlastFamilyPtr BlastAssetBoxes::createFamily(PhysXController& physXConroller, ExtPxManager& pxManager, const ActorDesc& desc)
{
	return BlastFamilyPtr(new BlastFamilyBoxes(physXConroller, pxManager, m_renderer, *this, desc));
}
