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


#include "NvBlastGlobals.h"
#include "NvBlastExtPxTaskImpl.h"

#include "NvBlastTkGroup.h"

using namespace Nv::Blast;


uint32_t ExtGroupTaskManagerImpl::process(uint32_t workerCount)
{
	NVBLAST_CHECK_WARNING(m_group != nullptr, "ExtGroupTaskManager::process cannot process, no group set.", return 0);
	NVBLAST_CHECK_WARNING(m_sync.isDone(), "ExtGroupTaskManager::process group is already being processed.", return 0);

	// at least one task must start, even when dispatcher has none specified
	uint32_t dispatcherThreads = m_taskManager.getCpuDispatcher()->getWorkerCount();
	dispatcherThreads = dispatcherThreads > 0 ? dispatcherThreads : 1;

	// not expecting an arbitrary amount of tasks
	uint32_t availableTasks = TASKS_MAX_COUNT;

	// use workerCount tasks, unless dispatcher has less threads or less tasks are available
	uint32_t requestedTasks = workerCount > 0 ? workerCount : dispatcherThreads;
	requestedTasks = requestedTasks > dispatcherThreads ? dispatcherThreads : requestedTasks;
	requestedTasks = requestedTasks > availableTasks ? availableTasks : requestedTasks;

	// ensure the group has enough memory allocated for concurrent processing
	m_group->setWorkerCount(requestedTasks);

	// check if there is work to do
	uint32_t jobCount = m_group->startProcess();

	if (jobCount)
	{
		// don't start more tasks than jobs are available
		requestedTasks = requestedTasks > jobCount ? jobCount : requestedTasks;

		// common counter for all tasks
		m_counter.reset(jobCount);

		// set to busy state
		m_sync.setCount(requestedTasks);

		// set up tasks
		for (uint32_t i = 0; i < requestedTasks; i++)
		{
			m_tasks[i].setup(m_group, &m_counter, &m_sync);
			m_tasks[i].setContinuation(m_taskManager, nullptr);
			m_tasks[i].removeReference();
		}

		return requestedTasks;
	}

	// there was no work to be done
	return 0;
}


bool ExtGroupTaskManagerImpl::wait(bool block)
{
	if (block && !m_sync.isDone())
	{
		m_sync.wait();
	}
	if (m_sync.isDone())
	{
		return m_group->endProcess();
	}
	return false;
}


void ExtGroupTaskManagerImpl::setGroup(TkGroup* group)
{
	NVBLAST_CHECK_WARNING(m_sync.isDone(), "ExtGroupTaskManager::setGroup trying to change group while processing.", return);

	m_group = group;
}


ExtGroupTaskManager* ExtGroupTaskManager::create(physx::PxTaskManager& taskManager, TkGroup* group)
{
	return NVBLAST_NEW(ExtGroupTaskManagerImpl) (taskManager, group);
}


void ExtGroupTaskManagerImpl::release()
{
	NVBLAST_CHECK_WARNING(m_sync.isDone(), "ExtGroupTaskManager::release group is still being processed.", return);

	NVBLAST_DELETE(this, ExtGroupTaskManagerImpl);
}
