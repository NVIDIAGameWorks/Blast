/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/


#include "BlastFamilyBoxes.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlastExtPxActor.h"
#include "BlastAssetBoxes.h"
#include "Renderer.h"
#include "PhysXController.h"
#include "RenderUtils.h"
#include "PxRigidDynamic.h"

using namespace physx;


BlastFamilyBoxes::BlastFamilyBoxes(PhysXController& physXController, ExtPxManager& pxManager, Renderer& renderer, const BlastAssetBoxes& blastAsset, const BlastAsset::ActorDesc& desc)
	: BlastFamily(physXController, pxManager, blastAsset), m_renderer(renderer)
{
	// prepare renderables
	IRenderMesh* boxRenderMesh = renderer.getPrimitiveRenderMesh(PrimitiveRenderMeshType::Box);
	RenderMaterial* primitiveRenderMaterial = physXController.getPrimitiveRenderMaterial();

	const ExtPxAsset* pxAsset = m_blastAsset.getPxAsset();
	const uint32_t chunkCount = pxAsset->getChunkCount();
	const ExtPxChunk* chunks = pxAsset->getChunks();
	const ExtPxSubchunk* subChunks = pxAsset->getSubchunks();
	m_chunkRenderables.resize(chunkCount);
	for (uint32_t i = 0; i < chunkCount; i++)
	{
		Renderable* renderable = renderer.createRenderable(*boxRenderMesh, *primitiveRenderMaterial);
		renderable->setHidden(true);
		renderable->setScale(subChunks[chunks[i].firstSubchunkIndex].geometry.scale.scale);
		m_chunkRenderables[i] = renderable;
	}

	// initialize in position
	initialize(desc);
}

BlastFamilyBoxes::~BlastFamilyBoxes()
{
	for (uint32_t i = 0; i < m_chunkRenderables.size(); i++)
	{
		m_renderer.removeRenderable(m_chunkRenderables[i]);
	}
}

void BlastFamilyBoxes::onActorCreated(const ExtPxActor& actor)
{
	DirectX::XMFLOAT4 color = getRandomPastelColor();

	const uint32_t* chunkIndices = actor.getChunkIndices();
	uint32_t chunkCount = actor.getChunkCount();
	for (uint32_t i = 0; i < chunkCount; i++)
	{
		const uint32_t chunkIndex = chunkIndices[i];
		m_chunkRenderables[chunkIndex]->setHidden(false);
		m_chunkRenderables[chunkIndex]->setColor(color);
	}
}

void BlastFamilyBoxes::onActorUpdate(const ExtPxActor& actor)
{
	const ExtPxChunk* chunks = m_blastAsset.getPxAsset()->getChunks();
	const ExtPxSubchunk* subChunks = m_blastAsset.getPxAsset()->getSubchunks();
	const uint32_t* chunkIndices = actor.getChunkIndices();
	uint32_t chunkCount = actor.getChunkCount();
	for (uint32_t i = 0; i < chunkCount; i++)
	{
		const uint32_t chunkIndex = chunkIndices[i];
		m_chunkRenderables[chunkIndex]->setTransform(actor.getPhysXActor().getGlobalPose() * subChunks[chunks[chunkIndex].firstSubchunkIndex].transform);
	}
}

void BlastFamilyBoxes::onActorDestroyed(const ExtPxActor& actor)
{
	const uint32_t* chunkIndices = actor.getChunkIndices();
	uint32_t chunkCount = actor.getChunkCount();
	for (uint32_t i = 0; i < chunkCount; i++)
	{
		m_chunkRenderables[chunkIndices[i]]->setHidden(true);
	}

}
