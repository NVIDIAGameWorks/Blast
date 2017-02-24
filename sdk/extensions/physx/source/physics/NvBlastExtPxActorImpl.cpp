/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastExtPxActorImpl.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlastExtPxManagerImpl.h"
#include "NvBlastExtPxFamilyImpl.h"

#include "PxRigidDynamic.h"
#include "PxPhysics.h"

#include "NvBlastTkActor.h"
#include "NvBlastTkAsset.h"

#include "PxRigidBodyExt.h"


namespace Nv
{
namespace Blast
{


ExtPxActorImpl::ExtPxActorImpl(ExtPxFamilyImpl* family, TkActor* tkActor, const PxActorCreateInfo& pxActorInfo)
	: m_family(family), m_tkActor(tkActor)
{
	const ExtPxChunk* pxChunks = m_family->m_pxAsset.getChunks();
	const ExtPxSubchunk* pxSubchunks = m_family->m_pxAsset.getSubchunks();
	const NvBlastChunk* chunks = m_tkActor->getAsset()->getChunks();
	uint32_t nodeCount = m_tkActor->getGraphNodeCount();

	PxFilterData simulationFilterData;	// Default constructor = {0,0,0,0}

	// get visible chunk indices list
	{
		auto& chunkIndices = m_family->m_indicesScratch;
		chunkIndices.resize(m_tkActor->getVisibleChunkCount());
		m_tkActor->getVisibleChunkIndices(chunkIndices.begin(), static_cast<uint32_t>(chunkIndices.size()));

		// fill visible chunk indices list with mapped to our asset indices
		m_chunkIndices.reserve(chunkIndices.size());
		for (const uint32_t chunkIndex : chunkIndices)
		{
			const ExtPxChunk& chunk = pxChunks[chunkIndex];
			if (chunk.subchunkCount == 0)
				continue;
			m_chunkIndices.pushBack(chunkIndex);
		}

		// Single lower-support chunk actors might be leaf actors, check for this and disable contact callbacks if so
		if (nodeCount <= 1)
		{
			PX_ASSERT(chunkIndices.size() == 1);
			if (chunkIndices.size() > 0)
			{
				const NvBlastChunk& chunk = chunks[chunkIndices[0]];
				if (chunk.firstChildIndex == chunk.childIndexStop)
				{
					simulationFilterData.word3 = ExtPxManager::LEAF_CHUNK;	// mark as leaf chunk if chunk has no children
				}
			}
		}
	}

	// create rigidDynamic and setup
	PxPhysics& physics = m_family->m_manager.m_physics;
	m_rigidDynamic = physics.createRigidDynamic(pxActorInfo.m_transform);
	if (m_family->m_pxActorDescTemplate != nullptr)
	{
		m_rigidDynamic->setActorFlags(static_cast<physx::PxActorFlags>(m_family->m_pxActorDescTemplate->flags));
	}

	// fill rigidDynamic with shapes
	PxMaterial* material = m_family->m_spawnSettings.material;
	for (uint32_t i = 0; i < m_chunkIndices.size(); ++i)
	{
		uint32_t chunkID = m_chunkIndices[i];
		const ExtPxChunk& chunk = pxChunks[chunkID];
		for (uint32_t c = 0; c < chunk.subchunkCount; c++)
		{
			const uint32_t subchunkIndex = chunk.firstSubchunkIndex + c;
			auto& subchunk = pxSubchunks[subchunkIndex];
			PxShape* shape = physics.createShape(subchunk.geometry, *material);
			shape->setLocalPose(subchunk.transform);

			const ExtPxShapeDescTemplate* pxShapeDesc = m_family->m_pxShapeDescTemplate;
			if (pxShapeDesc != nullptr)
			{
				shape->setFlags(static_cast<PxShapeFlags>(pxShapeDesc->flags));
				shape->setSimulationFilterData(pxShapeDesc->simulationFilterData);
				shape->setQueryFilterData(pxShapeDesc->queryFilterData);
				shape->setContactOffset(pxShapeDesc->contactOffset);
				shape->setRestOffset(pxShapeDesc->restOffset);
			}
			else
			{
				shape->setSimulationFilterData(simulationFilterData);
			}

			m_rigidDynamic->attachShape(*shape);

			PX_ASSERT_WITH_MESSAGE(m_family->m_subchunkShapes[subchunkIndex] == nullptr, "Chunk has some shapes(live).");
			m_family->m_subchunkShapes[subchunkIndex] = shape;
		}
	}

	// search for static chunk in actor's graph (make actor static if it contains static chunk)
	bool staticFound = false;
	if (nodeCount > 0)
	{
		auto& graphChunkIndices = m_family->m_indicesScratch;
		graphChunkIndices.resize(nodeCount);
		m_tkActor->getGraphNodeIndices(graphChunkIndices.begin(), static_cast<uint32_t>(graphChunkIndices.size()));
		const NvBlastSupportGraph graph = m_tkActor->getAsset()->getGraph();

		for (uint32_t i = 0; !staticFound && i < graphChunkIndices.size(); ++i)
		{
			uint32_t chunkIndex = graph.chunkIndices[graphChunkIndices[i]];
			const ExtPxChunk& chunk = pxChunks[chunkIndex];
			staticFound = chunk.isStatic;
		}
	}
	m_rigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, staticFound);

	// store pointer to actor in px userData
	m_family->m_manager.registerActor(m_rigidDynamic, this);

	// store pointer to actor in blast userData
	m_tkActor->userData = this;

	// update mass properties
	PxRigidBodyExt::updateMassAndInertia(*m_rigidDynamic, m_family->m_spawnSettings.density);

	// set initial velocities
	if (!(m_rigidDynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC))
	{
		m_rigidDynamic->setLinearVelocity(pxActorInfo.m_linearVelocity);
		m_rigidDynamic->setAngularVelocity(pxActorInfo.m_angularVelocity);
	}
}

void ExtPxActorImpl::release()
{
	if (m_rigidDynamic != nullptr)
	{
		m_family->m_manager.unregisterActor(m_rigidDynamic);
		m_rigidDynamic->release();
		m_rigidDynamic = nullptr;
	}

	const ExtPxChunk* pxChunks = m_family->m_pxAsset.getChunks();
	for (uint32_t chunkID : m_chunkIndices)
	{
		const ExtPxChunk& chunk = pxChunks[chunkID];
		for (uint32_t c = 0; c < chunk.subchunkCount; c++)
		{
			const uint32_t subchunkIndex = chunk.firstSubchunkIndex + c;
			m_family->m_subchunkShapes[subchunkIndex] = nullptr;
		}
	}
	m_chunkIndices.clear();

	m_tkActor->userData = nullptr;
}

ExtPxFamily& ExtPxActorImpl::getFamily() const
{
	return *m_family;
}


} // namespace Blast
} // namespace Nv
