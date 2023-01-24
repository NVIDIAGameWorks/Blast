.. _pageextstress:

Stress Solver (NvBlastExtStress)
--------------------------------


The Blast stress solver extension provides an implementation of a fast and easy to use stress solver which works directly with the bond graph. 
It simulates iteratively spreading forces across bonds that connect nodes of the support graph (where nodes typically represent chunks) 
and allows bond limits to be specified for tension, compression, and shear independently. 

The most common usage is applying gravity force on a static structure so that it will fall apart at some point when it cannot hold anymore.  For gravity to work correctly 
there must be at least one node in the graph with 0 mass.  That causes the system to treat the node as being fixed and having infinite mass so it can't move to reach a converged state.  
You can use part of the destructible itself, or add a dummy node and bonds connecting it to the nodes that should support the structure.  The other option is to add a supporting force to all 
nodes at the "base" of the graph (where the graph connects to the thing supporting it, normally the bottom, but could be the top for a hanging structure).  
Without this, the algorithm will stabilize with no internal force due to gravity, effectively treating it like the entire structure is in freefall.

It also can be used as another way to apply impact damage, which can give the visually pleasant result of an actor breaking in a weak place instead of the place of contact.  This is accomplished 
by adding the impulse of collisions to the nearest node in the graph.

Dynamic actors are also supported, you could for example add centrifugal force so that rotating an object fast enough will break bonds.  Keep in mind that for gravity to work with dynamic actors, 
an opposing force or static node(s) must be added to the system to provide resistance to the force.

.. _stresssolverfeatures:

Features
========

* Requires only core \a NvBlast
* Supports both static and dynamic actors
* Propagates both linear and angular momentum
* Graph complexity selection (reduces support graph to smaller size to trade-off speed for quality)
* Apply stress damage on Blast actor
* Debug Render

.. _stresssolvertuning:

Settings Tuning
===============

Computation time is linearly proportional to the \a maxSolverIterationsPerFrame setting. Higher values will converge to a solution sooner, but at processing cost.
\a graphReductionLevel should be considered experimental at this point, it has not been heavily tested.

Debug render can help a lot for tuning, consider using \a stressSolver->fillDebugRender(...) for that.

.. _stresssolverusage:

Usage 
=====

In order to use the stress solver, create an instance with \a ExtStressSolver::create(...).

.. code-block:: text

    ExtStressSolver* stressSolver = ExtStressSolver::create(family, settings);


\a ExtStressSolverSettings are passed in create function, but also can be changed at any time with \a stressSolver->setSettings(...).

It fully utilizes the fact that it knows the initial support graph structure and does a maximum of processing 
in the \a create(...) method call. After that, all actor split calls are synchronized internally and efficiently so only the actual stress propagation takes most of computational time.

You need to provide physics specific information (mass, volume, position) for every node in support graph since Blast itself is physics agnostic.  
There are two ways to do it.  One way is to call \a stressSolver->setNodeInfo(...) for every graph node.  The other way is to call stressSolver->setAllNodesInfoFromLL() once: 
all the data will be populated using NvBlastAsset chunk's data, in particular \a volume and \a centroid.  All 'world' nodes are considered static.

.. code-block:: text

    stressSolver->setAllNodesInfoFromLL(density);


The stress solver needs to keep track for actor create/destroy events in order to update its internal stress graph accordingly.  
So you need to call \a stressSolver->notifyActorCreated(actor) and \a stressSolver->notifyActorDestroyed(actor) every time an actor is created or destroyed, 
including the initial actor the family had when the stress solver was created.  There is no need to track actors which contain only one or less graph nodes.  
In that case \a notifyActorCreated(actor) returns 'false' as a hint.  It means that the stress solver will ignore them, as for those actors applying forces does not make any sense.

A typical update loop looks like this:

-# Apply all forces, use \a stressSolver->addForce(...), stressSolver->addGravity(...), \a stressSolver->addCentrifugalAcceleration(...)
-# Call \a stressSolver->update(). This is where all expensive computation takes place.
-# If \a stressSolver->getOverstressedBondCount() > 0, use one of \a stressSolver->generateFractureCommands() methods to get bond fracture commands and apply them on actors.
-# If split happened, call relevant stressSolver->notifyActorCreated(actor) and stressSolver->notifyActorDestroyed(actor)

..
   Example code:
   
   .. code-block:: text
   
        void MyStressSolverImpl::update(bool doDamage)
        {
            // Add external forces to the system first.
            // Collisions should be added external to this function if they should be considered as well.
            for (auto it = m_actors.getIterator(); !it.done(); ++it)
            {
                const ExtPxActor* pxActor = *it;
                const NvBlastActor* actor = *pxActor->getTkActor().getActorLL();
        
                PxRigidDynamic& rigidDynamic = pxActor->getPhysXActor();
                const bool isStatic = rigidDynamic.getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC;
                if (isStatic)
                {
                    NvcVec3 gravity = fromNvShared(rigidDynamic.getScene()->getGravity());
                    NvcVec3 localGravity = fromNvShared(rigidDynamic.getGlobalPose().rotateInv(gravity));
        
                    m_solver->addGravity(*actor, localGravity);
                }
                else
                {
                    NvcVec3 localCenterMass = fromNvShared(rigidDynamic.getCMassLocalPose().p);
                    NvcVec3 localAngularVelocity = fromNvShared(rigidDynamic.getGlobalPose().rotateInv(rigidDynamic.getAngularVelocity()));
                    m_solver->addCentrifugalAcceleration(*actor, localCenterMass, localAngularVelocity);
                }
            }
        
            m_solver->update();
        
            if (doDamage && m_solver->getOverstressedBondCount() > 0)
            {
                NvBlastActorSplitEvent splitEvent;
                for (auto it = m_actors.getIterator(); !it.done(); ++it)
                {
                    const ExtPxActor* pxActor = *it;
                    const NvBlastActor* actor = *pxActor->getTkActor().getActorLL();
            
                    NvBlastFractureBuffers commands;
                    m_solver->generateFractureCommands(*actor, commands);
                    if (commands.bondFractureCount > 0)
                    {
                        NvBlastActorApplyFracture(&commands, actor, &commands, logFn, nullptr);

                        // The Actor may be split into all its smallest pieces.
                        const uint32_t maxNewActorCount = NvBlastActorGetMaxActorCountForSplit(actor, logFn);
                        std::vector<NvBlastActor*> newActors(maxNewActorCount, nullptr);
                        splitEvent.newActors = newActors.data();

                        // Split the actor into connected islands.
                        std::vector<char> scratch(NvBlastActorGetRequiredScratchForSplit(actor, logFn));
                        const uint32_t createdActorCount = NvBlastActorSplit(&splitEvent, actor, maxNewActorCount, scratch.data(), logFn, nullptr);

                        // First remove the actor being deleted if it is valid.
                        if (splitEvent.deletedActor)
                        {
                            onActorDestroyed(m_family, *splitEvent.deletedActor);
                        }

                        // Then add all the new actors.
                        for (uint32_t a = 0; a < createdActorCount; a++)
                        {
                            const NvBlastActor* newActor = newActors[a];
                            onActorCreated(m_family, *newActor);
                        }
                    }
                }
            }
        }

