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


#ifndef NVBLASTEXTPXTASKIMPL_H
#define NVBLASTEXTPXTASKIMPL_H

#include "NvBlastExtPxTask.h"
#include "task/PxTask.h"
#include "NvBlastTkGroup.h"

#include <atomic>
#include <mutex>
#include <condition_variable>

namespace Nv
{
namespace Blast
{

/**
Counting synchronization object for waiting on TkWorkers to finish.
*/
class ExtTaskSync
{
public:
	/**
	Initializes with an expected number of notifications.
	*/
	ExtTaskSync(uint32_t count) : m_count(count) {}

	/**
	Blocks until the expected number of notifications happened.
	*/
	void wait()
	{
		std::unique_lock<std::mutex> lk(m_mutex);
		m_cv.wait(lk, [&] { return m_count == 0; });
	}

	/**
	Decrement the wait() count by one.
	*/
	void notify()
	{
		//PERF_SCOPE_H("TaskSync::notify");
		std::unique_lock<std::mutex> lk(m_mutex);
		if (m_count > 0)
		{
			m_count--;
		}
		if (m_count == 0)
		{
			lk.unlock();
			m_cv.notify_one();
		}
	}

	/**
	Peek if notifications are pending.
	*/
	bool isDone()
	{
		std::unique_lock<std::mutex> lk(m_mutex);
		return m_count == 0;
	}

	/**
	Sets the expected number of notifications for wait() to unblock.
	*/
	void setCount(uint32_t count)
	{
		m_count = count;
	}

private:
	std::mutex				m_mutex;
	std::condition_variable	m_cv;
	uint32_t				m_count;
};


/**
Common job counter for all tasks.
*/
class ExtAtomicCounter
{
public:
	ExtAtomicCounter() : m_current(0), m_maxCount(0) {}

	bool isValid(uint32_t val)
	{
		return val < m_maxCount;
	}

	uint32_t next()
	{
		return m_current.fetch_add(1);
	}

	void reset(uint32_t maxCount)
	{
		m_maxCount = maxCount;
		m_current = 0;
	}
private:
	std::atomic<uint32_t> m_current;
	uint32_t m_maxCount;
};


/**
A task running one group job after the other until done. Synchronizes atomically with its siblings.
*/
class ExtGroupWorkerTask : public physx::PxLightCpuTask
{
public:
	ExtGroupWorkerTask() : PxLightCpuTask(), m_group(nullptr), m_counter(nullptr), m_sync(nullptr)
	{
	}

	void setup(TkGroup* group, ExtAtomicCounter* counter, ExtTaskSync* sync)
	{
		m_group = group;
		m_counter = counter;
		m_sync = sync;
	}

	virtual void run() override
	{
		Nv::Blast::TkGroupWorker* worker = m_group->acquireWorker();
		uint32_t jobID = m_counter->next();
		while (m_counter->isValid(jobID))
		{
			worker->process(jobID);
			jobID = m_counter->next();
		}
		m_group->returnWorker(worker);
	}

	virtual void release() override
	{
		PxLightCpuTask::release();

		// release the sync last
		m_sync->notify();
	}

	virtual const char* getName() const override { return "BlastGroupWorkerTask"; }

private:
	TkGroup* m_group;
	ExtAtomicCounter* m_counter;
	ExtTaskSync* m_sync;
};


/**
Implements ExtGroupTaskManager
*/
class ExtGroupTaskManagerImpl : public ExtGroupTaskManager
{
public:
	ExtGroupTaskManagerImpl(physx::PxTaskManager& taskManager, TkGroup* group)
		: m_taskManager(taskManager), m_sync(0), m_group(group) {}

	// ExtGroupTaskManager API
	virtual void setGroup(TkGroup*) override;
	virtual uint32_t process(uint32_t) override;
	virtual void release() override;
	virtual bool wait(bool block) override;

private:
	static const uint32_t		TASKS_MAX_COUNT = 16;
	physx::PxTaskManager&		m_taskManager;
	ExtAtomicCounter			m_counter;
	ExtGroupWorkerTask			m_tasks[TASKS_MAX_COUNT];
	ExtTaskSync					m_sync;
	TkGroup*					m_group;
};

} // namespace Blast
} // namespace Nv

#endif // NVBLASTEXTPXTASKIMPL_H
