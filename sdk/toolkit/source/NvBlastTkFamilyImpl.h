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


#ifndef NVBLASTTKFAMILYIMPL_H
#define NVBLASTTKFAMILYIMPL_H

#include "NvBlastTkCommon.h"

#include "NvBlastTkFamily.h"
#include "NvBlastTkTypeImpl.h"
#include "NvBlastTkActorImpl.h"

#include "NvBlastTkEventQueue.h"
#include "NvBlastHashSet.h"
#include "NvBlastHashMap.h"

#include "NvBlast.h"
#include "NvBlastAssert.h"
#include "NvBlastDLink.h"


// Forward declarations
struct NvBlastFamily;

namespace Nv
{
namespace Blast
{

// Forward declarations
class TkGroupImpl;
class TkAssetImpl;


NVBLASTTK_IMPL_DECLARE(Family)
{
public:
	TkFamilyImpl();
	TkFamilyImpl(const NvBlastID& id);
	~TkFamilyImpl();

	NVBLASTTK_IMPL_DEFINE_IDENTIFIABLE('A', 'C', 'T', 'F');

	// Begin TkFamily
	virtual const NvBlastFamily*	getFamilyLL() const override;

	virtual uint32_t				getActorCount() const override;

	virtual uint32_t				getActors(TkActor** buffer, uint32_t bufferSize, uint32_t indexStart = 0) const override;

	virtual void					addListener(TkEventListener& l) override { m_queue.addListener(l); }

	virtual void					removeListener(TkEventListener& l) override { m_queue.removeListener(l); }

	virtual void					applyFracture(const NvBlastFractureBuffers* commands) override { applyFractureInternal(commands); }

	virtual const TkAsset*			getAsset() const override;

	virtual void					reinitialize(const NvBlastFamily* newFamily, TkGroup* group) override;
	// End TkFamily

	// Public methods
	static TkFamilyImpl*			create(const TkAssetImpl* asset);

	const TkAssetImpl*				getAssetImpl() const;

	NvBlastFamily*					getFamilyLLInternal() const;

	uint32_t						getActorCountInternal() const;

	TkActorImpl*					addActor(NvBlastActor* actorLL);

	void							applyFractureInternal(const NvBlastFractureBuffers* commands);

	void							removeActor(TkActorImpl* actorLL);

	TkEventQueue&					getQueue() { return m_queue; }

	TkActorImpl*					getActorByActorLL(const NvBlastActor* actorLL);

	void							updateJoints(TkActorImpl* actor, TkEventQueue* alternateQueue = nullptr);

	Array<TkActorImpl>::type&		getActorsInternal();

	uint32_t						getInternalJointCount() const;

	TkJointImpl*					getInternalJoints() const;

	TkJointImpl**					createExternalJointHandle(const NvBlastID& otherFamilyID, uint32_t chunkIndex0, uint32_t chunkIndex1);

	bool							deleteExternalJointHandle(TkJointImpl*& joint, const NvBlastID& otherFamilyID, uint32_t chunkIndex0, uint32_t chunkIndex1);

	void							releaseJoint(TkJointImpl& joint);

	TkActorImpl*					getActorByChunk(uint32_t chunkIndex);

	typedef physx::shdfnd::Pair<uint32_t, uint32_t>	ExternalJointKey;	//!< The chunk indices within the TkFamily objects joined by the joint.  These chunks will be support chunks.

	TkJointImpl*					findExternalJoint(const TkFamilyImpl* otherFamily, ExternalJointKey key) const;

private:
	TkActorImpl*					getActorByIndex(uint32_t index);

	struct JointSet
	{
		NvBlastID										m_familyID;
		HashMap<ExternalJointKey, TkJointImpl*>::type	m_joints;
	};

	typedef HashMap<NvBlastID, uint32_t>::type	FamilyIDMap;

	NvBlastFamily*				m_familyLL;
	Array<TkActorImpl>::type	m_actors;
	uint32_t					m_internalJointCount;
	Array<uint8_t>::type		m_internalJointBuffer;
	Array<JointSet*>::type		m_jointSets;
	FamilyIDMap					m_familyIDMap;
	const TkAssetImpl*			m_asset;

	TkEventQueue				m_queue;
};


//////// TkFamilyImpl inline methods ////////

NV_INLINE const TkAssetImpl* TkFamilyImpl::getAssetImpl() const
{
	return m_asset;
}


NV_INLINE NvBlastFamily* TkFamilyImpl::getFamilyLLInternal() const
{ 
	return m_familyLL; 
}


NV_INLINE uint32_t TkFamilyImpl::getActorCountInternal() const
{
	NVBLAST_ASSERT(m_familyLL != nullptr);

	return NvBlastFamilyGetActorCount(m_familyLL, logLL);
}


NV_INLINE TkActorImpl* TkFamilyImpl::getActorByIndex(uint32_t index)
{
	NVBLAST_ASSERT(index < m_actors.size());
	return &m_actors[index];
}


NV_INLINE TkActorImpl* TkFamilyImpl::getActorByActorLL(const NvBlastActor* actorLL)
{
	uint32_t index = NvBlastActorGetIndex(actorLL, logLL);
	return getActorByIndex(index);
}


NV_INLINE Array<TkActorImpl>::type& TkFamilyImpl::getActorsInternal()
{
	return m_actors;
}


NV_INLINE uint32_t TkFamilyImpl::getInternalJointCount() const
{
	return m_internalJointCount;
}


NV_INLINE TkJointImpl* TkFamilyImpl::getInternalJoints() const
{
	return const_cast<TkJointImpl*>(reinterpret_cast<const TkJointImpl*>(m_internalJointBuffer.begin()));
}


NV_INLINE void TkFamilyImpl::releaseJoint(TkJointImpl& joint)
{
	NVBLAST_ASSERT(joint.m_owner == this);
	NVBLAST_ASSERT(&joint >= getInternalJoints() && &joint < getInternalJoints() + getInternalJointCount() * sizeof(TkJointImpl));

	joint.~TkJointImpl();
	joint.m_owner = nullptr;
}


//////// Inline global functions ////////

NV_INLINE const NvBlastID& getFamilyID(const TkActor* actor)
{
	return actor != nullptr ? static_cast<const TkActorImpl*>(actor)->getFamilyImpl().getIDInternal() : *reinterpret_cast<const NvBlastID*>("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
}

NV_INLINE const NvBlastID& getFamilyID(const TkFamilyImpl* family)
{
	return family != nullptr ? family->getIDInternal() : *reinterpret_cast<const NvBlastID*>("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
}

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTTKFAMILYIMPL_H
