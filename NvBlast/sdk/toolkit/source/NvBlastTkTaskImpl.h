/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTTKTASKIMPL_H
#define NVBLASTTKTASKIMPL_H

#include "NvBlast.h"

#include "NvBlastTkFrameworkImpl.h"
#include "NvBlastTkEventQueue.h"
#include "NvBlastTkArray.h"

#include <atomic>
#include <mutex>
#include <condition_variable>

#include "task/PxTask.h"
#include "NvBlastAssert.h"

#include "NvBlastTkGroup.h" // TkGroupStats


namespace Nv
{
namespace Blast
{

class TkGroupImpl;
class TkActorImpl;
class TkFamilyImpl;


/**
Transient structure describing a job and its results.
*/
struct TkWorkerJob
{
	TkActorImpl*	m_tkActor;			//!< the actor to process
	TkActorImpl**	m_newActors;		//!< list of child actors created by splitting
	uint32_t		m_newActorsCount;	//!< the number of child actors created
};


/**
Counting synchronization object for waiting on TkWorkers to finish.
*/
class TaskSync
{
public:
	/**
	Initializes with an expected number of notifications.
	*/
	TaskSync(uint32_t count) : m_count(count) {}

	/**
	Blocks until the expected number of notifications happened.
	*/
	void wait()
	{
		std::unique_lock<std::mutex> lk(m_mutex);
		m_cv.wait(lk, [&]{ return m_count == 0; });
	}

	/**
	Decrement the wait() count by one.
	*/
	void notify()
	{
		PERF_SCOPE_H("TaskSync::notify");
		std::unique_lock<std::mutex> lk(m_mutex);
		m_count--;
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
A list of equally sized memory blocks sharable between tasks.
*/
template<typename T>
class SharedBlock
{
public:

	SharedBlock() : m_numElementsPerBlock(0), m_numBlocks(0), m_buffer(nullptr) {}

	/**
	Allocates one large memory block of elementsPerBlock*numBlocks elements.
	*/
	void allocate(uint32_t elementsPerBlock, uint32_t numBlocks)
	{
		NVBLAST_ASSERT(elementsPerBlock > 0 && numBlocks > 0);

		m_buffer = reinterpret_cast<T*>(NVBLASTTK_ALLOC(elementsPerBlock*numBlocks*sizeof(T), "SharedBlock"));
		m_numElementsPerBlock = elementsPerBlock;
		m_numBlocks = numBlocks;
	}

	/**
	Returns the pointer to the first element of a block of numElementsPerBlock() elements.
	*/
	T* getBlock(uint32_t id)
	{
		NVBLAST_ASSERT(id < m_numBlocks || 0 == m_numElementsPerBlock);
		return &m_buffer[id*m_numElementsPerBlock];
	}

	/**
	The number of elements available per block.
	*/
	uint32_t numElementsPerBlock() const 
	{
		return m_numElementsPerBlock; 
	}

	/**
	Frees the whole memory block.
	*/
	void release()
	{
		m_numBlocks = 0;
		m_numElementsPerBlock = 0;
		NVBLASTTK_FREE(m_buffer);
		m_buffer = nullptr;
	}

private:
	uint32_t	m_numElementsPerBlock;	//!< elements available in one block
	uint32_t	m_numBlocks;			//!< number of virtual blocks available
	T*			m_buffer;				//!< contiguous memory for all blocks
};


/**
A preallocated, shared array from which can be allocated from in tasks.
Intended to be used when the maximum amount of data (e.g. for a family) 
is known in advance. No further allocations take place on exhaustion.
Exhaustion asserts in debug builds and overflows otherwise.
*/
template<typename T>
class SharedBuffer
{
public:
	SharedBuffer() : m_capacity(0), m_used(0), m_buffer(nullptr) {}

	/**
	Atomically gets a pointer to the first element of an array of n elements.
	*/
	T* reserve(size_t n)
	{
		NVBLAST_ASSERT(m_used + n <= m_capacity);
		size_t start = m_used.fetch_add(n);
		return &m_buffer[start];
	}

	/**
	Preallocates memory for capacity elements.
	*/
	void allocate(size_t capacity)
	{
		NVBLAST_ASSERT(m_buffer == nullptr);
		m_buffer = reinterpret_cast<T*>(NVBLASTTK_ALLOC(capacity*sizeof(T), "SplitMemory"));
		m_capacity = capacity;
	}

	/**
	Preserves the memory allocated but resets to reserve from the beginning of the array.
	*/
	void reset()
	{
		m_used = 0;
	}

	/**
	Frees the preallocated array.
	*/
	void release()
	{
		NVBLAST_ASSERT(m_buffer != nullptr);
		NVBLASTTK_FREE(m_buffer);
		m_buffer = nullptr;
		m_capacity = m_used = 0;
	}

private:
	size_t				m_capacity;	//!< available elements in the buffer
	std::atomic<size_t>	m_used;		//!< used elements in the buffer
	T*					m_buffer;	//!< the memory containing T's
};


/**
Allocates from a preallocated, externally owned memory block initialized with.
When blocks run out of space, new ones are allocated and owned by this class.
*/
template<typename T>
class LocalBuffer
{
public:
	/**
	Returns the pointer to the first element of an array of n elements.
	Allocates a new block of memory when exhausted, its size being the larger of n and capacity set with initialize().
	*/
	T* allocate(size_t n)
	{
		if (m_used + n > m_capacity)
		{
			allocateNewBlock(n > m_capacity ? n : m_capacity);
		}

		size_t index = m_used;
		m_used += n;
		return &m_currentBlock[index];
	}

	/**
	Release the additionally allocated memory blocks.
	The externally owned memory block remains untouched.
	*/
	void clear()
	{
		for (void* block : m_memoryBlocks)
		{
			NVBLASTTK_FREE(block);
		}
		m_memoryBlocks.clear();
	}

	/**
	Set the externally owned memory block to start allocating from,
	with a size of capacity elements.
	*/
	void initialize(T* block, size_t capacity)
	{
		m_currentBlock = block;
		m_capacity = capacity;
		m_used = 0;
	}

private:
	/**
	Allocates space for capacity elements.
	*/
	void allocateNewBlock(size_t capacity)
	{
		PERF_SCOPE_L("Local Buffer allocation");
		m_capacity = capacity;
		m_currentBlock = static_cast<T*>(NVBLASTTK_ALLOC(capacity*sizeof(T), "Blast LocalBuffer"));
		m_memoryBlocks.pushBack(m_currentBlock);
		m_used = 0;
	}

	TkInlineArray<void*, 4>::type	m_memoryBlocks; //!< storage for memory blocks
	T*								m_currentBlock;	//!< memory block used to allocate from
	size_t							m_used;			//!< elements used in current block
	size_t							m_capacity;		//!< elements available in current block
};


/**
Holds the memory used by TkWorker for each family in each group.
*/
class SharedMemory
{
public:
	SharedMemory() : m_eventsMemory(0), m_eventsCount(0), m_refCount(0) {}

	/**
	Reserves n entries from preallocated memory.
	*/
	NvBlastActor** reserveNewActors(size_t n)
	{
		return m_newActorBuffers.reserve(n);
	}

	/**
	Reserves n entries from preallocated memory.
	*/
	TkActor** reserveNewTkActors(size_t n)
	{
		return m_newTkActorBuffers.reserve(n);
	}

	/**
	Allocates buffers to hold 
	*/
	void allocate(TkFamilyImpl&);

	/**
	Resets the internal buffers to reserve from their beginning.
	Preserves the allocated memory.
	*/
	void reset()
	{
		m_newActorBuffers.reset();
		m_newTkActorBuffers.reset();
	}

	/**
	Increments the reference count.
	*/
	void addReference()			{ m_refCount++; }

	/**
	Increments the reference count by n.
	*/
	void addReference(size_t n) { m_refCount += n; }

	/**
	Decrements the reference count.
	Returns true if the count reached zero.
	*/
	bool removeReference()
	{
		m_refCount--;
		return !isUsed();
	}

	/**
	Checks if the reference count is not zero. 
	*/
	bool isUsed()
	{
		return m_refCount > 0;
	}

	/**
	Release the internal buffers' memory.
	*/
	void release()
	{
		m_newActorBuffers.release();
		m_newTkActorBuffers.release();
	}

	TkEventQueue				m_events;				//!< event queue shared across a group's actors of the same family
	uint32_t					m_eventsMemory;			//!< expected memory size for event data
	uint32_t					m_eventsCount;			//!< expected number of events

private:
	size_t						m_refCount;				//!< helper for usage and releasing memory

	SharedBuffer<NvBlastActor*>	m_newActorBuffers;		//!< memory for splitting
	SharedBuffer<TkActor*>		m_newTkActorBuffers;	//!< memory for split events
};


/**
Shared job queue from which TkWorkers atomically fetch the next job.
*/
template <typename T>
class TkAtomicQueue
{
public:
	/**
	Initialize for a new batch of jobs.
	*/
	void init(TkWorkerJob* jobs, uint32_t numJobs)
	{
		m_jobs = jobs;
		m_maxCount = numJobs;
		m_current = 0;
	}

	/**
	Fetch a pointer to the next job. Returns nullptr when exhausted.
	*/
	T* next()
	{
		size_t index = m_current.fetch_add(1, std::memory_order_relaxed);
		if (index < m_maxCount)
		{
			return &m_jobs[index];
		}
		return nullptr;
	}

private:
	T*					m_jobs;			//!< the list of jobs
	size_t				m_maxCount;		//!< number of jobs available in the list
	std::atomic<size_t>	m_current;		//!< current job counter
};

typedef TkAtomicQueue<TkWorkerJob> TkAtomicJobQueue;


/**
Thread worker fracturing and splitting actors sequentially.
The list of actual jobs is provided by the group owning this worker.
*/
class TkWorker : public physx::PxLightCpuTask
{
public:
	void		run();
	void		release();
	const char*	getName() const { return "TkWorker"; }

	uint32_t								m_id;			//!< this worker's id
	TkGroupImpl*							m_group;		//!< the group owning this worker

	LocalBuffer<NvBlastChunkFractureData>	m_chunkBuffer;	//!< memory manager for chunk event data
	LocalBuffer<NvBlastBondFractureData>	m_bondBuffer;	//!< memory manager for bonds event data

#if NV_PROFILE
	TkGroupStats	m_stats;
#endif
};
}
}

#endif // NVBLASTTKTASKIMPL_H
