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


#ifndef NVBLASTEXTPXTASK_H
#define NVBLASTEXTPXTASK_H

#include "NvBlastTypes.h"


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
class TkGroup;


/**
Uses a physx::PxTaskManager to process a TkGroup concurrently.
*/
class NV_DLL_EXPORT ExtGroupTaskManager
{
protected:
	virtual ~ExtGroupTaskManager() {}

public:
	/**
	Construct using existing physx::PxTaskManager and TkGroup. The TkGroup can be set later with setGroup().
	*/
	static ExtGroupTaskManager* create(physx::PxTaskManager&, TkGroup* = nullptr);

	/**
	Set the group to process. Cannot be changed while a group being processed.
	*/
	virtual void setGroup(TkGroup*) = 0;

	/**
	Start processing the group.
	The parallelizing strategy is to have all worker tasks running concurrently.
	The number of started tasks may be smaller than the requested value,
	when the task manager's dispatcher thread count or the number of group jobs are
	smaller.

	\param[in]	workerCount		The number of worker tasks to start, 
								0 uses the dispatcher's worker thread count.

	\return						The number of worker tasks started.
								If 0, processing did not start and wait() will never return true.
	*/
	virtual uint32_t process(uint32_t workerCount = 0) = 0;

	/**
	Wait for the group to end processing. When processing has finished, TkGroup::endProcess is executed.

	\param[in]	block			true:	does not return until the group has been processed.
								false:	return immediately if workers are still processing the group.

	\return						true if group processing was completed (and the group was actually processing)
	*/
	virtual bool wait(bool block = true) = 0;

	/**
	Release this object.
	*/
	virtual void release() = 0;
};


} // namespace Blast
} // namespace Nv

#endif // NVBLASTEXTPXTASK_H
