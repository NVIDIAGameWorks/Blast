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
// Copyright (c) 2008-2014 NVIDIA Corporation. All rights reserved.

#ifndef NV_GPU_DISPATCHER_H
#define NV_GPU_DISPATCHER_H

#include "NvTaskDefine.h"
#include "NvTask.h"

/* forward decl to avoid including <cuda.h> */
typedef struct CUstream_st* CUstream;

namespace nvidia
{
    namespace cudamanager
    {
        struct NvGpuCopyDesc;
        class NvCudaContextManager;
    }

    namespace task
    {

NV_PUSH_PACK_DEFAULT

class NvTaskManager;

/** \brief A GpuTask dispatcher
 *
 * A NvGpuDispatcher executes GpuTasks submitted by one or more TaskManagers (one
 * or more scenes).  It maintains a CPU worker thread which waits on GpuTask
 * "groups" to be submitted.  The submission API is explicitly sessioned so that
 * GpuTasks are dispatched together as a group whenever possible to improve
 * parallelism on the GPU.
 *
 * A NvGpuDispatcher cannot be allocated ad-hoc, they are created as a result of
 * creating a NvCudaContextManager.  Every NvCudaContextManager has a NvGpuDispatcher
 * instance that can be queried.  In this way, each NvGpuDispatcher is tied to
 * exactly one CUDA context.
 *
 * A scene will use CPU fallback Tasks for GpuTasks if the NvTaskManager provided
 * to it does not have a NvGpuDispatcher.  For this reason, the NvGpuDispatcher must
 * be assigned to the NvTaskManager before the NvTaskManager is given to a scene.
 *
 * Multiple TaskManagers may safely share a single NvGpuDispatcher instance, thus
 * enabling scenes to share a CUDA context.
 *
 * Only failureDetected() is intended for use by the user.  The rest of the
 * nvGpuDispatcher public methods are reserved for internal use by only both
 * TaskManagers and GpuTasks.
 */
class NvGpuDispatcher
{
public:
    /** \brief Record the start of a simulation step
     *
     * A NvTaskManager calls this function to record the beginning of a simulation
     * step.  The NvGpuDispatcher uses this notification to initialize the
     * profiler state.
     */
    virtual void                startSimulation() = 0;

    /** \brief Record the start of a GpuTask batch submission
     *
     * A NvTaskManager calls this function to notify the NvGpuDispatcher that one or
     * more GpuTasks are about to be submitted for execution.  The NvGpuDispatcher
     * will not read the incoming task queue until it receives one finishGroup()
     * call for each startGroup() call.  This is to ensure as many GpuTasks as
     * possible are executed together as a group, generating optimal parallelism
     * on the GPU.
     */
    virtual void                startGroup() = 0;

    /** \brief Submit a GpuTask for execution
     *
     * Submitted tasks are pushed onto an incoming queue.  The NvGpuDispatcher
     * will take the contents of this queue every time the pending group count
     * reaches 0 and run the group of submitted GpuTasks as an interleaved
     * group.
     */
    virtual void                submitTask(NvTask& task) = 0;

    /** \brief Record the end of a GpuTask batch submission
     *
     * A NvTaskManager calls this function to notify the NvGpuDispatcher that it is
     * done submitting a group of GpuTasks (GpuTasks which were all make ready
     * to run by the same prerequisite dependency becoming resolved).  If no
     * other group submissions are in progress, the NvGpuDispatcher will execute
     * the set of ready tasks.
     */
    virtual void                finishGroup() = 0;

    /** \brief Add a CUDA completion prerequisite dependency to a task
     *
     * A GpuTask calls this function to add a prerequisite dependency on another
     * task (usually a CpuTask) preventing that task from starting until all of
     * the CUDA kernels and copies already launched have been completed.  The
     * NvGpuDispatcher will increment that task's reference count, blocking its
     * execution, until the CUDA work is complete.
     *
     * This is generally only required when a CPU task is expecting the results
     * of the CUDA kernels to have been copied into host memory.
     *
     * This mechanism is not at all not required to ensure CUDA kernels and
     * copies are issued in the correct order.  Kernel issue order is determined
     * by normal task dependencies.  The rule of thumb is to only use a blocking
     * completion prerequisite if the task in question depends on a completed
     * GPU->Host DMA.
     *
     * The NvGpuDispatcher issues a blocking event record to CUDA for the purposes
     * of tracking the already submitted CUDA work.  When this event is
     * resolved, the NvGpuDispatcher manually decrements the reference count of
     * the specified task, allowing it to execute (assuming it does not have
     * other pending prerequisites).
     */
    virtual void                addCompletionPrereq(NvBaseTask& task) = 0;

    /** \brief Retrieve the NvCudaContextManager associated with this
     * NvGpuDispatcher
     *
     * Every NvCudaContextManager has one NvGpuDispatcher, and every NvGpuDispatcher
     * has one NvCudaContextManager.
     */
    virtual cudamanager::NvCudaContextManager* getCudaContextManager() = 0;

    /** \brief Record the end of a simulation frame
     *
     * A NvTaskManager calls this function to record the completion of its
     * dependency graph.  If profiling is enabled, the NvGpuDispatcher will
     * trigger the retrieval of profiling data from the GPU at this point.
     */
    virtual void                stopSimulation() = 0;

    /** \brief Returns true if a CUDA call has returned a non-recoverable error
     *
     * A return value of true indicates a fatal error has occurred. To protect
     * itself, the NvGpuDispatcher enters a fall through mode that allows GpuTasks
     * to complete without being executed.  This allows simulations to continue
     * but leaves GPU content static or corrupted.
     *
     * The user may try to recover from these failures by deleting GPU content
     * so the visual artifacts are minimized.  But there is no way to recover
     * the state of the GPU actors before the failure.  Once a CUDA context is
     * in this state, the only recourse is to create a new CUDA context, a new
     * scene, and start over.
     *
     * This is our "Best Effort" attempt to not turn a soft failure into a hard
     * failure because continued use of a CUDA context after it has returned an
     * error will usually result in a driver reset.  However if the initial
     * failure was serious enough, a reset may have already occurred by the time
     * we learn of it.
     */
    virtual bool                failureDetected() const = 0;

    /** \brief Force the NvGpuDispatcher into failure mode
     *
     * This API should be used if user code detects a non-recoverable CUDA
     * error.  This ensures the NvGpuDispatcher does not launch any further
     * CUDA work.  Subsequent calls to failureDetected() will return true.
     */
    virtual void                forceFailureMode() = 0;

    /** \brief Returns a pointer to the current in-use profile buffer
     *
     * The returned pointer should be passed to all kernel launches to enable
     * CTA/Warp level profiling.  If a data collector is not attached, or CTA
     * profiling is not enabled, the pointer will be zero.
     */
    virtual void*               getCurrentProfileBuffer() const = 0;

    /** \brief Register kernel names with PlatformAnalyzer
     *
     * The returned uint16_t must be stored and used as a base offset for the ID
     * passed to the KERNEL_START|STOP_EVENT macros.
     */
    virtual uint16_t               registerKernelNames(const char**, uint16_t count) = 0;

    /** \brief Launch a copy kernel with arbitrary number of copy commands
     *
     * This method is intended to be called from Kernel GpuTasks, but it can
     * function outside of that context as well.
     *
     * If count is 1, the descriptor is passed to the kernel as arguments, so it
     * may be declared on the stack.
     *
     * If count is greater than 1, the kernel will read the descriptors out of
     * host memory.  Because of this, the descriptor array must be located in
     * page locked (pinned) memory.  The provided descriptors may be modified by
     * this method (converting host pointers to their GPU mapped equivalents)
     * and should be considered *owned* by CUDA until the current batch of work
     * has completed, so descriptor arrays should not be freed or modified until
     * you have received a completion notification.
     *
     * If your GPU does not support mapping of page locked memory (SM>=1.1),
     * this function degrades to calling CUDA copy methods.
     */
    virtual void                launchCopyKernel(cudamanager::NvGpuCopyDesc* desc, uint32_t count, CUstream stream) = 0;

    /** \brief Query pre launch task that runs before launching gpu kernels.
     *
     * This is part of an optional feature to schedule multiple gpu features 
     * at the same time to get kernels to run in parallel.
     * \note Do *not* set the continuation on the returned task, but use addPreLaunchDependent().
     */
    virtual NvBaseTask&         getPreLaunchTask() = 0;

    /** \brief Adds a gpu launch task that gets executed after the pre launch task.
     *
     * This is part of an optional feature to schedule multiple gpu features 
     * at the same time to get kernels to run in parallel.
     * \note Each call adds a reference to the pre-launch task. 
     */
    virtual void                addPreLaunchDependent(NvBaseTask& dependent) = 0;

    /** \brief Query post launch task that runs after the gpu is done.
     *
     * This is part of an optional feature to schedule multiple gpu features 
     * at the same time to get kernels to run in parallel.
     * \note Do *not* set the continuation on the returned task, but use addPostLaunchDependent().
     */
    virtual NvBaseTask&         getPostLaunchTask() = 0;
    
    /** \brief Adds a task that gets executed after the post launch task.
     *
     * This is part of an optional feature to schedule multiple gpu features 
     * at the same time to get kernels to run in parallel.
     * \note Each call adds a reference to the pre-launch task. 
     */
    virtual void                addPostLaunchDependent(NvBaseTask& dependent) = 0;

protected:
    /** \brief protected destructor
     *
     * GpuDispatchers are allocated and freed by their NvCudaContextManager.
     */
    virtual ~NvGpuDispatcher() {}
};

NV_POP_PACK

} } // end nvidia namespace


#endif
