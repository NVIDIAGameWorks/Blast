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
// Copyright (c) 2016-2018 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTTKGROUP_H
#define NVBLASTTKGROUP_H

#include "NvBlastTkIdentifiable.h"


namespace Nv
{
namespace Blast
{

// Forward declarations
class TkActor;


/**
Descriptor for a TkGroup.  TkGroup uses a number of TkGroupWorker to process its actors.
@see TkGroupWorker, TkGroup::setWorkerCount
*/
struct TkGroupDesc
{
	uint32_t		workerCount;			//!< The number of expected TkWorkers to process the TkGroup concurrently.
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
A worker as provided by TkGroup::acquireWorker(). It manages the necessary memory for parallel processing.
The group can be processed concurrently by calling process() from different threads using a different TkGroupWorker each.

TkActors that have been damaged with applyFracture() such that they may be split into separate
actors are split by this function. TkActors that have damage queued through the actor's damage() function
will be fractured and split by this function.
*/
class TkGroupWorker
{
public:
	/**
	Process a job of this worker's TkGroup.

	/param[in]	jobId	a job id in the range (0, TkGroup::startProcess()]
	*/
	virtual void	process(uint32_t jobId) = 0;
};


/**
A group is a processing unit, to which the user may add TkActors.  New actors generated from splitting a TkActor
are automatically put into the same group.  However, any actor may be removed from its group and placed into
another group (or no group) by the user's choice.

When the group's process function is called, all actors' damage buffers will be processed and turned into fracture events
and the actor is split if applicable.

This work can be done in multiple threads with the help of TkGroupWorker:  
Instead of calling the process function, commence the procedure with startProcess which returns the number of jobs to process.  
Each concurrent thread uses an acquired TkGroupWorker to process the jobs.
Over the whole procedure, each job must be processed once and only once.  
Jobs can be processed in any order.  TkGroupWorkers can be returned and acquired later by another task.
After processing every job and returning all the workers to the group, endProcess concludes the procedure.
*/
class TkGroup : public TkIdentifiable
{
public:
	/**
	Add the actor to this group, if the actor does not currently belong to a group.

	\param[in]	actor	The actor to add.

	\return true if successful, false otherwise.
	*/
	virtual bool			addActor(TkActor& actor) = 0;

	/**
	The number of actors currently in this group.

	\return the number of TkActors that currently exist in this group.
	*/
	virtual uint32_t		getActorCount() const = 0;

	/**
	Retrieve an array of pointers (into the user-supplied buffer) to actors.

	\param[out]	buffer		A user-supplied array of TkActor pointers.
	\param[in]	bufferSize	The number of elements available to write into buffer.
	\param[in]	indexStart	The starting index of the actor.

	\return the number of TkActor pointers written to the buffer.
	*/
	virtual uint32_t		getActors(TkActor** buffer, uint32_t bufferSize, uint32_t indexStart = 0) const = 0;

	/**
	Lock this group for processing concurrently with TkGroupWorker.  The group is unlocked again with the endProcess() function.

	\return The number of jobs to process. TkGroupWorker::process must be called once for each jobID from 0 to this number-1.
			See TkGroup::process for a single threaded example.
	*/
	virtual uint32_t		startProcess() = 0;

	/**
	Unlock this group after all jobs were processed with TkGroupWorker.  All workers must have been returned with returnWorker().
	This function gathers the results of the split operations on the actors in this group.  Events will be dispatched
	to notify listeners of new and deleted actors.

	Note that groups concurrently dispatching events for the same TkFamily require synchronization in the TkFamily's Listener.
	However, concurrent use of endProcess is not recommended in this version. It should be called from the main thread.

	\return		true	if the group was processing
	*/
	virtual bool			endProcess() = 0;

	/**
	Set the expected number of concurrent worker threads that will process this group concurrently.
	*/
	virtual void			setWorkerCount(uint32_t workerCount) = 0;

	/**
	\return The total amount of workers allocated for this group.
	*/
	virtual uint32_t		getWorkerCount() const = 0;

	/**
	Acquire one worker to process the group concurrently on a thread.
	The worker must be returned with returnWorker() before endProcess() is called on its group.

	\return A worker for this group (at most getWorkerCount) or nullptr if none is available.
	*/
	virtual TkGroupWorker*	acquireWorker() = 0;

	/**
	Return a worker previously acquired with acquireWorker() to this TkGroup.

	\param[in] The TkGroupWorker previously acquired from this TkGroup.
	*/
	virtual void			returnWorker(TkGroupWorker*) = 0;

	/**
	Helper function to process the group synchronously on a single thread.
	*/
            void			process();

	/**
	For profile builds only, request stats of the last successful processing. Inactive in other builds.
	The times and counters reported account for all the TkWorker (accumulated) taking part in the processing.

	\param[in]	stats	The struct to be filled in.
	*/
	virtual void			getStats(TkGroupStats& stats) const = 0;
};

} // namespace Blast
} // namespace Nv


NV_INLINE void Nv::Blast::TkGroup::process()
{
	uint32_t jobCount = startProcess();
	if (jobCount > 0)
	{
		TkGroupWorker* worker = acquireWorker();
		for (uint32_t i = 0; i < jobCount; i++)
		{
			worker->process(i);
		}
		returnWorker(worker);
	}
	endProcess();
}


#endif // ifndef NVBLASTTKGROUP_H
