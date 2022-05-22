.. _pageextphysx:

Extensions (NvBlastExtPhysX)
----------------------------

NvBlastExtPhysX contains classes for easier use of Blast Toolkit with the PhysXSDK.
There are three of them:
* :ref:`extpxmanager` - Manager to keep Blast actors in sync with PhysXactors.
* :ref:`extimpactdamagemanager` - Manager to collect and apply impact damage caused by collision in PhysXscene.
* :ref:`extpxstresssolver` - Stress Solver to propagate stress through support graph and apply it as damage to Blast actors.

This library also contains an extension for synchronizing Blast state:
* :ref:`ExtSync` - Utility for writing Blast state to a buffer, to be read by a client.  This may be used for networking, for example.

It also provides classes for utilizing PhysXSDK Foundation capabilities:
* :ref:`ExtGroupTaskManager` - Multithreaded TkGroup processing using PxTask.
* :ref:`ExtCustomProfiler` - Serves Blast profiling zones to PxFoundation profiler (e.g. PVD) and platform specific profilers.

.. _extpxmanager:

ExtPxManager
============

**Physics Manager** - is a reference implementation for keeping Blast actors synced with PhysXactors. Its main job is to listen
for TkFamily events and update \a PxScene (by adding and removing PxActors) accordingly.

In order to use it, create an ExtPxManager.  If we have a physx::PxPhysics object m_physics and a TkFramework m_tkFramework, use

.. code-block:: text

    ExtPxManager* pxManager = ExtPxManager::create(m_physics, m_tkFramework);


For every \a TkAsset prepare \a ExtPxAsset, which contains \a TkAsset + collection of physics geometry for every chunk. Every chunk can contain any number of subchunks,
where each subchunk is basically a PxConvexMeshGeometry with transform. Also every chunk can be marked as static (\a isStatic flag). 
If an actor contains at least one static chunk in its support graph, it makes that actor kinematic (static). Otherwise the actor is dynamic.
Having zero subchunks makes the chunk invisible in the physics scene. It can be used for example to represent 'earth' as a special invisible static chunk and connect all near earth chunks to it.


To create an \a ExtPxFamily from an \a ExtPxAsset:

.. code-block:: text

    ExtPxFamilyDesc familyDesc;
    familyDesc.pxAsset = pxAsset;
    familyDesc.group = tkGroup;
    familyDesc.actorDesc.initialBondHealths = nullptr;
    familyDesc.actorDesc.initialSupportChunkHealths = nullptr;
    familyDesc.actorDesc.uniformInitialBondHealth = BOND_HEALTH_MAX;
    familyDesc.actorDesc.uniformInitialLowerSupportChunkHealth = 1.0f;
    ExtPxFamily* family = pxManager->createFamily(desc);


You can subscribe to family events in order to sync graphics (or anything else) with physics:

.. code-block:: text

    family->subscribe(listener);


The listener will be notified with all physics actors added and removed.

And finally spawn the family in some world position (the first actor/actors will be created and an event will be fired to the listener):

.. code-block:: text

    
    ExtPxSpawnSettings spawnSettings = {
    	&pxScene,
    	defaultPxMaterial,
    	RIGIDBODY_DENSITY
    };
    
    family->spawn(PxTransform(0, 0, 0), PxVec3(1, 1, 1), spawnSettings);


You can get a family's actors either from listening to events or by calling \a getActors(). 
Every \a ExtPxActor matches 1 <-> 1 with TkActor (which matches \a NvBlastActor accordingly).

.. code-block:: text

    ExtPxActor* actor = ....; 
    physx::PxRigidDynamic rigidDynamic = actor->getPxActor(); 


An ExtPxActor remains internally unchanged throughout its lifetime.
Use \a ExtPxActor \a getChunkIndices() and  \a getPxActor() to update your graphics representation. Sample code:

.. code-block:: text

    const uint32_t* chunkIndices;
    size_t chunkIndexCount;
    actor.getChunkIndices(chunkIndices, chunkIndexCount);
    for (uint32_t i = 0; i < chunkIndexCount; i++)
    {
    	uint32_t chunkIndex = chunkIndices[i];
    	for (Renderable* r : m_chunks[chunkIndex].renderables)
    	{
    		r->setTransform(actor.getPxActor()->getGlobalPose() * pxAsset.chunks[chunkIndex].convexes[0].transform);
    	}
    }


In order to use joints set a joint creation function with \a ExtPxManager::setCreateJointFunction(...). It will be called when new TkJoints are
being created. All the joint updates and removals will be handled by the manager internally.

.. _extimpactdamagemanager:

ExtImpactDamageManager
======================

**Impact Damage Manager** - is a reference implementation for fast and easy impact damage support. It is built on top of ExtPxManager.

In order to use it, create it as follows:

.. code-block:: text

    ExtImpactDamageManager* impactManager = ExtImpactDamageManager::create(pxManager);


Call its onContact method on every \a PxSimulationEventCallback \a onContact()

.. code-block:: text

    class EventCallback : public PxSimulationEventCallback
    {
    public:
    	EventCallback(ExtImpactDamageManager* manager) : m_manager(manager) {}
    
    	virtual void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, uint32_t nbPairs)
    	{
    		m_manager->onContact(pairHeader, pairs, nbPairs);
    	}
    
    private:
    	ExtImpactDamageManager*		m_manager;
    };


Call \a applyDamage() when you want the buffered damage to be applied:

.. code-block:: text

    impactManager->applyDamage();


**N.B.** for impact damage to work, you must enable contact notification with custom filter shader for PxScene. \a ExtImpactDamageManager has a reference filter shader implementation which can be used for that:

.. code-block:: text

    PxSceneDesc sceneDesc;
    sceneDesc.filterShader = ExtImpactDamageManager::FilterShader;

.. _extpxstresssolver:

ExtPxStressSolver
=================

**Stress Solver** - this wrapper class uses :ref:`pageextstress` to apply stress calculations to an ExtPxFamily.  See :ref:`pageextstress` for
the details of the underlying stress solver.

.. _pxstresssolverusage:

Usage
#####

In order to use it, instance an ExtPxStressSolver by providing \a ExtPxFamily:

.. code-block:: text

    ExtPxStressSolver* stressSolver = ExtPxStressSolver::create(family);


And then call update() every frame:

.. code-block:: text

    bool doDamage = true; // if you want to actually apply stress and damage actors
    stressSolver->update(doDamage);


By default it will apply scene gravity on static actors and centrifugal force on dynamic actors.

The underlying ExtStressSolver can be accessed using \a ExtPxStressSolver::getSolver().  For example, to apply impulse to a particular actor, \a applyImpulse(...) can be called for additional stress to apply:

.. code-block:: text

    stressSolver->getSolver().addForce(actor, position, impulse);


Finally, the stress solver (and its underlying ExtStressSolver) may be released using

.. code-block:: text

    stressSolver->release();


.. _extsync:

ExtSync
=======

**Synchronization Extension (NvBlastExtSync)** - is a reference implementation for synchronizing Blast state. 

The idea is that you can use it to write synchronization events to the buffer (on server for example) and then apply this buffer on
a client. TkFamily ID should be properly set for that.

3 types of events are supported:

* **ExtSyncEventType::Fracture**: Fracture event. Contains fracture commands information on particular TkFamily. Applied incrementally. Relatively small.
* **ExtSyncEventType::FamilySync**: Family sync event. Contains all necessary information to fully sync TkFamily state.
* **ExtSyncEventType::Physics**: Physics sync event. Contains all necessary information to fully sync ExtPxFamily state.

In order to use it, create ExtSync:

.. code-block:: text

    ExtSync* sync = ExtSync::create();


Then let ExtSync instance listen to family fracture commands and write them to internal buffer:

.. code-block:: text

    TkFamily* family = ...;
    family->addListener(*sync);
    
    // fracture family
    // ....


You can fully record TkFamily state or ExtPxFamily state at any moment by calling:

.. code-block:: text

    sync->syncFamily(tkFamily);
    // or
    sync->syncFamily(pxFamily);


Now you can take sync buffer:

.. code-block:: text

    const ExtSyncEvent*const* buffer;
    uint32_t size;
    sync->acquireSyncBuffer(buffer, size);
    
    m_savedBuffer.resize(size);
    for (uint32_t i = 0; i < size; ++i)
    {
    	m_savedBuffer[i] = buffer[i]->clone();
    }
    
    sync->releaseSyncBuffer();


On the client you can then apply this buffer:

.. code-block:: text

    sync->applySyncBuffer(tkFramework, m_savedBuffer.data(), m_savedBuffer.size(), group, pxManager);


ExtPxManager is required only if sync buffer contains ExtSyncEventType::Physics events.



.. _ExtGroupTaskManager:

ExtGroupTaskManager
===================

This class provides a basic implementation for multithreaded TkGroup processing using PxTask and a PxTaskManager from PxFoundation.

In the simplest use case, all worker threads provided by PxTaskManager are used to process the group.

.. code-block:: text

    // creating ExtGroupTaskManager from existing taskManager and tkGroup
    ExtGroupTaskManager* gtm = ExtGroupTaskManager::create(*taskManager, tkGroup);
    
    while (running)
    {
        // ... add TkActors to TkGroup and damage ...
    
        // start processing on all worker threads provided
        gtm->process();
    
        // ... do something else ...
    
        // wait for the group processing to finish
        gtm->wait();
    }
    
    // after use, release the ExtGroupTaskManager
    gtm->release();




Groups can be processed concurrently as well as follows. 

.. code-block:: text

    // creating ExtGroupTaskManager from existing taskManager and groups
    
    ExtGroupTaskManager* gtm0 = ExtGroupTaskManager::create(*taskManager, tkGroup0);
    ExtGroupTaskManager* gtm1 = ExtGroupTaskManager::create(*taskManager, tkGroup1);


TkActors are added to TkGroups and damaged as usual.

The PxTaskManager used in this example provides four worker threads of which each ExtGroupTaskManager uses two for its group.

.. code-block:: text

    uint32_t started = 0;
    if (gtm0->process(2) > 0) { started++; }
    if (gtm1->process(2) > 0) { started++; }


Note that ExtGroupTaskManager::wait() never returns true if no processing has started, as reported by ExtGroupTaskManager::process().
The illustrative busy loop is not recomended for actual use.

.. code-block:: text

    uint32_t completed = 0;
    while (completed < started)
    {
        if (gtm0->wait(false)) { completed++; }
        if (gtm1->wait(false)) { completed++; }
    }

.. _extcustomprofiler:

ExtCustomProfiler
=================

This Nv::Blast::ProfileCallback implementation forwards Blast profile events to the PxProfilerCallback attached to PxFoundation, typically a PhysXVisual Debugger (PVD) instance.
To use it, simply attach one to Blast.

.. code-block:: text

    static Nv::Blast::ExtCustomProfiler gBlastProfiler;
    NvBlastProfilerSetCallback(&gBlastProfiler);


For convenience, it also provides sending profile events to platform specific profilers. These are disabled by default.

.. code-block:: text

    gBlastProfiler.setPlatformEnabled(true);




