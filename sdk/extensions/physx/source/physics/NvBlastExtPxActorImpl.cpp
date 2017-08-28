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
// Copyright (c) 2016-2017 NVIDIA Corporation. All rights reserved.


#include "NvBlastExtPxActorImpl.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlastExtPxManagerImpl.h"
#include "NvBlastExtPxFamilyImpl.h"

#include "PxRigidDynamic.h"
#include "PxPhysics.h"

#include "NvBlastAssert.h"

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
			NVBLAST_ASSERT(chunkIndices.size() == 1);
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

			NVBLAST_ASSERT_WITH_MESSAGE(m_family->m_subchunkShapes[subchunkIndex] == nullptr, "Chunk has some shapes(live).");
			m_family->m_subchunkShapes[subchunkIndex] = shape;
		}
	}

	// search for static chunk in actor's graph (make actor static if it contains static chunk)
	bool staticFound = m_tkActor->isBoundToWorld();
	if (nodeCount > 0)
	{
		auto& graphNodeIndices = m_family->m_indicesScratch;
		graphNodeIndices.resize(nodeCount);
		m_tkActor->getGraphNodeIndices(graphNodeIndices.begin(), static_cast<uint32_t>(graphNodeIndices.size()));
		const NvBlastSupportGraph graph = m_tkActor->getAsset()->getGraph();

		for (uint32_t i = 0; !staticFound && i < graphNodeIndices.size(); ++i)
		{
			const uint32_t chunkIndex = graph.chunkIndices[graphNodeIndices[i]];
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
		const PxVec3 COM = m_rigidDynamic->getGlobalPose().transform(m_rigidDynamic->getCMassLocalPose().p);
		const PxVec3 linearVelocity = pxActorInfo.m_parentLinearVelocity + pxActorInfo.m_parentAngularVelocity.cross(COM - pxActorInfo.m_parentCOM);
		const PxVec3 angularVelocity = pxActorInfo.m_parentAngularVelocity;
		m_rigidDynamic->setLinearVelocity(linearVelocity);
		m_rigidDynamic->setAngularVelocity(angularVelocity);
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
