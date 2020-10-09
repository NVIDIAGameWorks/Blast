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
// Copyright (c) 2008-2020 NVIDIA Corporation. All rights reserved.


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
