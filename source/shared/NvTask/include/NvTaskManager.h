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

#ifndef NV_TASK_MANAGER_H
#define NV_TASK_MANAGER_H

#include "NvTaskDefine.h"
#include "NvSimpleTypes.h"
#include "NvErrorCallback.h"

namespace nvidia
{
    namespace task
    {

NV_PUSH_PACK_DEFAULT

class NvBaseTask;
class NvTask;
class NvLightCpuTask;
typedef unsigned int NvTaskID;

/**
\brief Identifies the type of each heavyweight NvTask object

\note This enum type is only used by NvTask and GpuTask objects, LightCpuTasks do not use this enum.

@see NvTask
@see NvLightCpuTask
*/
struct NvTaskType
{
    /**
     * \brief Identifies the type of each heavyweight NvTask object
     */
    enum Enum
    {
        TT_CPU,             //!< NvTask will be run on the CPU
        TT_GPU,             //!< NvTask will be run on the GPU
        TT_NOT_PRESENT,     //!< Return code when attempting to find a task that does not exist
        TT_COMPLETED        //!< NvTask execution has been completed
    };
};

class NvCpuDispatcher;
class NvGpuDispatcher;

/** 
 \brief The NvTaskManager interface
 
 A NvTaskManager instance holds references to user-provided dispatcher objects, when tasks are
 submitted the NvTaskManager routes them to the appropriate dispatcher and handles task profiling if enabled. 


 @see CpuDispatcher
 @see NvGpuDispatcher
 
*/
class NvTaskManager
{
public:

    /**
    \brief Set the user-provided dispatcher object for CPU tasks

    \param[in] ref The dispatcher object.

    @see CpuDispatcher
    */
    virtual void     setCpuDispatcher(NvCpuDispatcher& ref) = 0;

    /**
    \brief Set the user-provided dispatcher object for GPU tasks

    \param[in] ref The dispatcher object.

    @see NvGpuDispatcher
    */
    virtual void     setGpuDispatcher(NvGpuDispatcher& ref) = 0;
    
    /**
    \brief Get the user-provided dispatcher object for CPU tasks

    \return The CPU dispatcher object.

    @see CpuDispatcher
    */
    virtual NvCpuDispatcher*            getCpuDispatcher() const = 0;

    /**
    \brief Get the user-provided dispatcher object for GPU tasks

    \return The GPU dispatcher object.

    @see NvGpuDispatcher
    */
    virtual NvGpuDispatcher*            getGpuDispatcher() const = 0;

    /**
    \brief Reset any dependencies between Tasks

    \note Will be called at the start of every frame before tasks are submitted.

    @see NvTask
    */
    virtual void    resetDependencies() = 0;
    
    /**
    \brief Called by the owning scene to start the task graph.

    \note All tasks with with ref count of 1 will be dispatched.

    @see NvTask
    */
    virtual void    startSimulation() = 0;

    /**
    \brief Called by the owning scene at the end of a simulation step to synchronize the NvGpuDispatcher

    @see NvGpuDispatcher
    */
    virtual void    stopSimulation() = 0;

    /**
    \brief Called by the worker threads to inform the NvTaskManager that a task has completed processing

    \param[in] task The task which has been completed
    */
    virtual void    taskCompleted(NvTask& task) = 0;

    /**
    \brief Retrieve a task by name

    \param[in] name The unique name of a task
    \return The ID of the task with that name, or TT_NOT_PRESENT if not found
    */
    virtual NvTaskID  getNamedTask(const char* name) = 0;

    /**
    \brief Submit a task with a unique name.

    \param[in] task The task to be executed
    \param[in] name The unique name of a task
    \param[in] type The type of the task (default TT_CPU)
    \return The ID of the task with that name, or TT_NOT_PRESENT if not found

    */
    virtual NvTaskID  submitNamedTask(NvTask* task, const char* name, NvTaskType::Enum type = NvTaskType::TT_CPU) = 0;

    /**
    \brief Submit an unnamed task.

    \param[in] task The task to be executed
    \param[in] type The type of the task (default TT_CPU)

    \return The ID of the task with that name, or TT_NOT_PRESENT if not found
    */
    virtual NvTaskID  submitUnnamedTask(NvTask& task, NvTaskType::Enum type = NvTaskType::TT_CPU) = 0;

    /**
    \brief Retrieve a task given a task ID

    \param[in] id The ID of the task to return, a valid ID must be passed or results are undefined

    \return The task associated with the ID
    */
    virtual NvTask*   getTaskFromID(NvTaskID id) = 0;

    /**
    \brief Release the NvTaskManager object, referenced dispatchers will not be released
    */
    virtual void        release() = 0;

    /**
    \brief Construct a new NvTaskManager instance with the given [optional] dispatchers
    */
    static NvTaskManager* createTaskManager(NvErrorCallback& errorCallback, NvCpuDispatcher* = 0, NvGpuDispatcher* = 0);
    
protected:
    virtual ~NvTaskManager() {}

    /*! \cond PRIVATE */

    virtual void finishBefore(NvTask& task, NvTaskID taskID) = 0;
    virtual void startAfter(NvTask& task, NvTaskID taskID) = 0;

    virtual void addReference(NvTaskID taskID) = 0;
    virtual void decrReference(NvTaskID taskID) = 0;
    virtual int32_t getReference(NvTaskID taskID) const = 0;

    virtual void decrReference(NvLightCpuTask&) = 0;
    virtual void addReference(NvLightCpuTask&) = 0;

    virtual void emitStartEvent(NvBaseTask&, uint32_t threadId=0) = 0;
    virtual void emitStopEvent(NvBaseTask&, uint32_t threadId=0) = 0;

    /*! \endcond */

    friend class NvBaseTask;
    friend class NvTask;
    friend class NvLightCpuTask;
    friend class NvGpuWorkerThread;
};

NV_POP_PACK

} } // end nvidia namespace


#endif
