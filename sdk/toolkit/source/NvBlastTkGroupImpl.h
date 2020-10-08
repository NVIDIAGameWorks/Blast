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


#ifndef NVBLASTTKGROUPIMPL_H
#define NVBLASTTKGROUPIMPL_H


#include "NvBlastTkTaskImpl.h"
#include "NvBlastTkGroup.h"
#include "NvBlastTkTypeImpl.h"


namespace Nv
{
namespace Blast
{

class TkActorImpl;
class TkFamilyImpl;

NVBLASTTK_IMPL_DECLARE(Group)
{
	~TkGroupImpl();

public:
	TkGroupImpl();

	NVBLASTTK_IMPL_DEFINE_IDENTIFIABLE('G', 'R', 'P', '\0');

	static TkGroupImpl*		create(const TkGroupDesc& desc);

	// Begin TkGroup
	virtual bool			addActor(TkActor& actor) override;

	virtual uint32_t		getActorCount() const override;

	virtual uint32_t		getActors(TkActor** buffer, uint32_t bufferSize, uint32_t indexStart = 0) const override;

	virtual uint32_t		startProcess() override;
	virtual bool			endProcess() override;

	virtual void			getStats(TkGroupStats& stats) const override;

	virtual void			setWorkerCount(uint32_t workerCount) override;
	virtual uint32_t		getWorkerCount() const override;

	virtual TkGroupWorker*	acquireWorker() override;
	virtual void			returnWorker(TkGroupWorker*) override;
	// End TkGroup

	// TkGroupImpl API

	/**
	Remove the actor from this group if the actor actually belongs to it and the group is not processing.

	\param[in]	actor		The TkActor to remove.

	\return					true if removing succeeded, false otherwise
	*/
	bool					removeActor(TkActor& actor);

	/**
	Add the actor to this group's job queue. 
	It is the caller's responsibility to add an actor only once. This condition is checked in debug builds.
	*/
	void					enqueue(TkActorImpl* tkActor);

	/**
	Atomically check if this group is processing actors. @see setProcessing()

	\return					true between startProcess() and endProcess() calls, false otherwise
	*/
	bool					isProcessing() const;

private:
	/**
	Atomically set the processing state. This function checks for the current state
	before changing it. @see isProcessing()

	\param[in]	value		the value of the new state

	\return					true if the new state could be set, false otherwise
	*/
	bool					setProcessing(bool value);

	/** 
	Get the group-family shared memory for the specified family. To be used when the memory is expected to already exist.
	*/
	SharedMemory*			getSharedMemory(TkFamilyImpl* family);
	void					releaseSharedMemory(TkFamilyImpl* fam, SharedMemory* mem);

	// functions to add/remove actors _without_ group-family memory management
	void					addActorInternal(TkActorImpl& tkActor);
	void					addActorsInternal(TkActorImpl** actors, uint32_t numActors);
	void					removeActorInternal(TkActorImpl& tkActor);


	uint32_t										m_actorCount;			//!< number of actors in this group

	HashMap<TkFamilyImpl*, SharedMemory*>::type		m_sharedMemory;			//!< memory sharable by actors in the same family in this group

	// it is assumed no more than the asset's number of bond and chunks fracture commands are produced
	SharedBlock<NvBlastChunkFractureData>			m_chunkTempDataBlock;	//!< chunk data for damage/fracture
	SharedBlock<NvBlastBondFractureData>			m_bondTempDataBlock;	//!< bond data for damage/fracture
	SharedBlock<NvBlastChunkFractureData>			m_chunkEventDataBlock;	//!< initial memory block for event data
	SharedBlock<NvBlastBondFractureData>			m_bondEventDataBlock;	//!< initial memory block for event data
	SharedBlock<char>								m_splitScratchBlock;	//!< split scratch memory 

	std::atomic<bool>								m_isProcessing;			//!< true while workers are processing

	Array<TkWorker>::type							m_workers;				//!< this group's workers

	Array<TkWorkerJob>::type						m_jobs;					//!< this group's process jobs

//#if NV_PROFILE
	TkGroupStats									m_stats;				//!< accumulated group's worker stats
//#endif

	std::mutex	m_workerMtx;

	friend class TkWorker;
};


NV_INLINE bool TkGroupImpl::isProcessing() const
{
	return m_isProcessing.load();
}


NV_INLINE void TkGroupImpl::getStats(TkGroupStats& stats) const
{
#if NV_PROFILE
	memcpy(&stats, &m_stats, sizeof(TkGroupStats));
#else
	NV_UNUSED(stats);
#endif
}


NV_INLINE uint32_t TkGroupImpl::getActorCount() const
{
	return m_actorCount;
}


NV_INLINE SharedMemory* TkGroupImpl::getSharedMemory(TkFamilyImpl* family)
{
	SharedMemory* mem = m_sharedMemory[family];
	NVBLAST_ASSERT(mem != nullptr);
	return mem;
}


NV_FORCE_INLINE void operator +=(NvBlastTimers& lhs, const NvBlastTimers& rhs)
{
	lhs.material += rhs.material;
	lhs.fracture += rhs.fracture;
	lhs.island += rhs.fracture;
	lhs.partition += rhs.partition;
	lhs.visibility += rhs.visibility;
}


} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTTKGROUPIMPL_H
