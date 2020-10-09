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
// Copyright (c) 2016-2020 NVIDIA Corporation. All rights reserved.


#include "NvBlastPreprocessor.h"

#include "NvBlastTkFrameworkImpl.h"
#include "NvBlastTkActorImpl.h"
#include "NvBlastTkGroupImpl.h"
#include "NvBlastTkAssetImpl.h"
#include "NvBlastTkFamilyImpl.h"
#include "NvBlastTkJointImpl.h"

#include "NvBlast.h"
#include "NvBlastAssert.h"
#include "NvBlastMemory.h"


namespace Nv
{
namespace Blast
{

TkActorImpl* TkActorImpl::create(const TkActorDesc& desc)
{
	const TkAssetImpl* asset = static_cast<const TkAssetImpl*>(desc.asset);

	TkFamilyImpl* family = TkFamilyImpl::create(asset);

	NvBlastFamily* familyLL = family->getFamilyLLInternal();
	Array<char>::type scratch((uint32_t)NvBlastFamilyGetRequiredScratchForCreateFirstActor(familyLL, logLL));
	NvBlastActor* actorLL = NvBlastFamilyCreateFirstActor(familyLL, &desc, scratch.begin(), logLL);
	if (actorLL == nullptr)
	{
		NVBLAST_LOG_ERROR("TkActorImpl::create: low-level actor could not be created.");
		return nullptr;
	}

	TkActorImpl* actor = family->addActor(actorLL);

	if (actor != nullptr)
	{
		// Add internal joints
		const uint32_t internalJointCount = asset->getJointDescCountInternal();
		const TkAssetJointDesc* jointDescs = asset->getJointDescsInternal();
		const NvBlastSupportGraph graph = asset->getGraph();
		TkJointImpl* joints = family->getInternalJoints();
		for (uint32_t jointNum = 0; jointNum < internalJointCount; ++jointNum)
		{
			const TkAssetJointDesc& assetJointDesc = jointDescs[jointNum];
			NVBLAST_ASSERT(assetJointDesc.nodeIndices[0] < graph.nodeCount && assetJointDesc.nodeIndices[1] < graph.nodeCount);
			TkJointDesc jointDesc;
			jointDesc.families[0] = jointDesc.families[1] = family;
			jointDesc.chunkIndices[0] = graph.chunkIndices[assetJointDesc.nodeIndices[0]];
			jointDesc.chunkIndices[1] = graph.chunkIndices[assetJointDesc.nodeIndices[1]];
			jointDesc.attachPositions[0] = assetJointDesc.attachPositions[0];
			jointDesc.attachPositions[1] = assetJointDesc.attachPositions[1];
			TkJointImpl* joint = new (joints + jointNum) TkJointImpl(jointDesc, family);
			actor->addJoint(joint->m_links[0]);
		}

		// Mark as damaged to trigger first split call. It could be the case that asset is already split into few actors initially.
		actor->markAsDamaged();
	}

	return actor;
}


//////// Member functions ////////

TkActorImpl::TkActorImpl()
	: m_actorLL(nullptr)
	, m_family(nullptr)
	, m_group(nullptr)
	, m_groupJobIndex(invalidIndex<uint32_t>())
	, m_flags(0)
	, m_jointCount(0)
{
#if NV_PROFILE
	NvBlastTimersReset(&m_timers);
#endif
}


TkActorImpl::~TkActorImpl()
{
}


void TkActorImpl::release()
{
	// Disassoaciate all joints

	// Copy joint array for safety against implementation of joint->setActor
	TkJointImpl** joints = reinterpret_cast<TkJointImpl**>(NvBlastAlloca(sizeof(TkJointImpl*)*getJointCountInternal()));
	TkJointImpl** stop = joints + getJointCountInternal();
	TkJointImpl** jointHandle = joints;
	for (JointIt j(*this); (bool)j; ++j)
	{
		*jointHandle++ = *j;
	}
	jointHandle = joints;
	while (jointHandle < stop)
	{
		NVBLAST_ASSERT(*jointHandle != nullptr);
		NVBLAST_ASSERT((*jointHandle)->getDataInternal().actors[0] == this || (*jointHandle)->getDataInternal().actors[1] == this);
		(*jointHandle++)->setActors(nullptr, nullptr);
	}
	NVBLAST_ASSERT(getJointCountInternal() == 0);

	if (m_group != nullptr)
	{
		m_group->removeActor(*this);
	}

	if (m_actorLL != nullptr)
	{
		NvBlastActorDeactivate(m_actorLL, logLL);
	}

	if (m_family != nullptr)
	{
		m_family->removeActor(this);

		// Make sure we dispatch any remaining events when this family is emptied, since it will no longer be done by any group
		if (m_family->getActorCountInternal() == 0)
		{
			m_family->getQueue().dispatch();
		}
	}
}


const NvBlastActor* TkActorImpl::getActorLL() const
{
	return m_actorLL;
}


TkFamily& TkActorImpl::getFamily() const
{
	return getFamilyImpl();
}


uint32_t TkActorImpl::getIndex() const
{
	return getIndexInternal();
}


TkGroup* TkActorImpl::getGroup() const
{
	return getGroupImpl();
}


TkGroup* TkActorImpl::removeFromGroup()
{
	if (m_group == nullptr)
	{
		NVBLAST_LOG_WARNING("TkActorImpl::removeFromGroup: actor not in a group.");
		return nullptr;
	}

	if (m_group->isProcessing())
	{
		NVBLAST_LOG_ERROR("TkActorImpl::removeFromGroup: cannot alter Group while processing.");
		return nullptr;
	}

	TkGroup* group = m_group;
	
	return m_group->removeActor(*this) ? group : nullptr;
}


NvBlastFamily* TkActorImpl::getFamilyLL() const
{ 
	return m_family->getFamilyLLInternal(); 
}


const TkAsset* TkActorImpl::getAsset() const
{ 
	return m_family->getAssetImpl(); 
}


uint32_t TkActorImpl::getVisibleChunkCount() const
{
	return NvBlastActorGetVisibleChunkCount(m_actorLL, logLL);
}


uint32_t TkActorImpl::getVisibleChunkIndices(uint32_t* visibleChunkIndices, uint32_t visibleChunkIndicesSize) const
{
	return NvBlastActorGetVisibleChunkIndices(visibleChunkIndices, visibleChunkIndicesSize, m_actorLL, logLL);
}


uint32_t TkActorImpl::getGraphNodeCount() const
{
	return NvBlastActorGetGraphNodeCount(m_actorLL, logLL);
}


uint32_t TkActorImpl::getGraphNodeIndices(uint32_t* graphNodeIndices, uint32_t graphNodeIndicesSize) const
{
	return NvBlastActorGetGraphNodeIndices(graphNodeIndices, graphNodeIndicesSize, m_actorLL, logLL);
}


const float* TkActorImpl::getBondHealths() const
{
	return NvBlastActorGetBondHealths(m_actorLL, logLL);
}


uint32_t TkActorImpl::getSplitMaxActorCount() const
{
	return NvBlastActorGetMaxActorCountForSplit(m_actorLL, logLL);
}


bool TkActorImpl::isDamaged() const
{
	NVBLAST_ASSERT(!m_flags.isSet(TkActorFlag::DAMAGED) || (m_flags.isSet(TkActorFlag::DAMAGED) && m_flags.isSet(TkActorFlag::PENDING)));
	return m_flags.isSet(TkActorFlag::DAMAGED);
}


void TkActorImpl::markAsDamaged()
{
	m_flags |= TkActorFlag::DAMAGED;
	makePending();
}


void TkActorImpl::makePending()
{
	if (m_group != nullptr && !isPending())
	{
		m_group->enqueue(this);
	}

	m_flags |= TkActorFlag::PENDING;
}


TkActorImpl::operator Nv::Blast::TkActorData() const
{
	TkActorData data = { m_family, userData, getIndex() };
	return data;
}


void TkActorImpl::damage(const NvBlastDamageProgram& program, const void* programParams)
{
	BLAST_PROFILE_SCOPE_L("TkActor::damage");

	if (m_group == nullptr)
	{
		NVBLAST_LOG_WARNING("TkActor::damage: actor is not in a group, cannot fracture.");
		return;
	}

	if (m_group->isProcessing())
	{
		NVBLAST_LOG_WARNING("TkActor::damage: group is being processed, cannot fracture this actor.");
		return;
	}

	if (NvBlastActorCanFracture(m_actorLL, logLL))
	{
		m_damageBuffer.pushBack(DamageData{ program, programParams});
		makePending();
	}
}


void TkActorImpl::generateFracture(NvBlastFractureBuffers* commands, const NvBlastDamageProgram& program, const void* programParams) const
{
	BLAST_PROFILE_SCOPE_L("TkActor::generateFracture");

	if (m_group && m_group->isProcessing())
	{
		NVBLAST_LOG_WARNING("TkActor::generateFracture: group is being processed, cannot fracture this actor.");
		return;
	}

	// const context, must make m_timers mutable otherwise
	NvBlastActorGenerateFracture(commands, m_actorLL, program, programParams, logLL, const_cast<NvBlastTimers*>(&m_timers));
}


void TkActorImpl::applyFracture(NvBlastFractureBuffers* eventBuffers, const NvBlastFractureBuffers* commands)
{
	BLAST_PROFILE_SCOPE_L("TkActor::applyFracture");

	if (m_group && m_group->isProcessing())
	{
		NVBLAST_LOG_WARNING("TkActor::applyFracture: group is being processed, cannot fracture this actor.");
		return;
	}

	NvBlastActorApplyFracture(eventBuffers, m_actorLL, commands, logLL, &m_timers);

	if (commands->chunkFractureCount > 0 || commands->bondFractureCount > 0)
	{
		markAsDamaged();

		TkFractureCommands* fevt = getFamilyImpl().getQueue().allocData<TkFractureCommands>();
		fevt->tkActorData = *this;
		fevt->buffers = *commands;
		getFamilyImpl().getQueue().addEvent(fevt);
		getFamilyImpl().getQueue().dispatch();
	}
}


uint32_t TkActorImpl::getJointCount() const
{
	return getJointCountInternal();
}


uint32_t TkActorImpl::getJoints(TkJoint** joints, uint32_t jointsSize) const
{
	uint32_t jointsWritten = 0;

	for (JointIt j(*this); (bool)j && jointsWritten < jointsSize; ++j)
	{
		joints[jointsWritten++] = *j;
	}

	return jointsWritten;
}


bool TkActorImpl::isBoundToWorld() const
{
	return NvBlastActorIsBoundToWorld(m_actorLL, logLL);
}


} // namespace Blast
} // namespace Nv
