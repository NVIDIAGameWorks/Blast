/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTTKGROUP_H
#define NVBLASTTKGROUP_H

#include "NvBlastTkIdentifiable.h"


// Forward declarations
namespace physx
{
class PxTaskManager;
}


namespace Nv
{
namespace Blast
{

// Forward declarations
class TkActor;


/**
Descriptor for a TkGroup.  It uses the PxShared PxTaskManager interface to dispatch PxLightCpuTask.
@see TkWorker
*/
struct TkGroupDesc
{
	physx::PxTaskManager* pxTaskManager;	//!< User-defined task manager
};


/**
Used to collect internal counters using TkGroup::getStats (for profile builds only)
@see TkGroup::getStats()
*/
struct TkGroupStats
{
	NvBlastTimers	timers;					//!< Accumulated time spent in blast low-level functions, see NvBlastTimers
	uint32_t		processedActorsCount;	//!< Accumulated number of processed actors in all TkWorker
	int64_t			workerTime;				//!< Accumulated time spent executing TkWorker::run. Unit is ticks, see NvBlastTimers.
};


/**
A group is a processing unit, to which the user may add TkActors.  New actors generated from splitting a TkActor
are automatically put into the same group.  However, any actor may be removed from its group and placed into
another group (or no group) by the user's choice.

When the group's process function is called, all actors' damage buffers will be processed and turned into fracture events
and the actor is split if applicable.
This work is done in separate (possibly multiple) threads.  The sync function waits for the processing threads to finish
and dispatches events for processing that actually occurred.
*/
class TkGroup : public TkIdentifiable
{
public:
	/**
	Add the actor to this group, if the actor does not currently belong to a group.

	\param[in]	actor	The actor to add.

	\return true if successful, false otherwise.
	*/
	virtual bool		addActor(TkActor& actor) = 0;

	/**
	The number of actors currently in this group.

	\return the number of TkActors that currently exist in this group.
	*/
	virtual uint32_t	getActorCount() const = 0;

	/**
	Retrieve an array of pointers (into the user-supplied buffer) to actors.

	\param[out]	buffer		A user-supplied array of TkActor pointers.
	\param[in]	bufferSize	The number of elements available to write into buffer.
	\param[in]	indexStart	The starting index of the actor.

	\return the number of TkActor pointers written to the buffer.
	*/
	virtual uint32_t	getActors(TkActor** buffer, uint32_t bufferSize, uint32_t indexStart = 0) const = 0;

	/**
	TkActors that have been damaged with applyFracture() such that they may be split into separate
	actors are split by this function. TkActors that have damage queued through the actor's damage() function
	will be fractured and split by this function.
	Fracture and splitting work will be run on different threads provided through TkGroupDesc::pxTaskManager.  
	All work is done asynchronously, and the results are gathered by the sync() function.

	Note: The number of threads provided by pxTaskManager must not change over the group's lifetime.

	\return true if processing may be launched (this group is not currently processing), false otherwise.
	*/
	virtual bool		process() = 0;

	/**
	If all threads spawned by process() have finished, and sync() has not yet been called since, then this
	function gathers the results of the split operations on the actors in this group.  Events will be dispatched
	to notify listeners of new and deleted actors.

	\param[in]	block	If true, this function waits until all threads have completed execution, then performs the gather and dispatch work.
						If false, this function will perform the gather and dispatch work only if threads have completed execution, otherwise it returns immediately.

	\return true if gather and dispatch work have been performed, false otherwise.
	*/
	virtual bool		sync(bool block = true) = 0;

	/**
	For profile builds only, request stats of the last successful processing. Inactive in other builds.
	The times and counters reported account for all the TkWorker (accumulated) taking part in the processing.

	\param[in]	stats	The struct to be filled in.
	*/
	virtual void		getStats(TkGroupStats& stats) const = 0;
};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTTKGROUP_H
