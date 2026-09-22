// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2016-2024 NVIDIA Corporation. All rights reserved.


#include "NvBlastExtPxStressSolverImpl.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlastExtPxFamily.h"
#include "NvBlastExtPxActor.h"
#include "NvBlastAssert.h"
#include "NvBlastIndexFns.h"

#include "NvBlastTkAsset.h"
#include "NvBlastTkActor.h"
#include "NvBlastTkFamily.h"

#include "NvBlastPxSharedHelpers.h"

#include "PxScene.h"
#include "PxRigidDynamic.h"

#define USE_PHYSX_NODE_INFO 1


namespace Nv
{
namespace Blast
{

using namespace physx;


ExtPxStressSolverImpl::ExtPxStressSolverImpl(ExtPxFamily& family, ExtStressSolverSettings settings)
    : m_family(family)
{
    NvBlastFamily* familyLL = const_cast<NvBlastFamily*>(family.getTkFamily().getFamilyLL());
    NVBLAST_ASSERT(familyLL);
    m_solver = ExtStressSolver::create(*familyLL, settings);

    const TkAsset* tkAsset = m_family.getTkFamily().getAsset();
    const ExtPxAsset& asset = m_family.getPxAsset();
    const ExtPxChunk* chunks = asset.getChunks();
    const ExtPxSubchunk* subChunks = asset.getSubchunks();
    const NvBlastSupportGraph graph = tkAsset->getGraph();
    const uint32_t chunkCount = tkAsset->getChunkCount();

    TkActor* tkActor;
    m_family.getTkFamily().getActors(&tkActor, 1);
    const float* bondHealths = tkActor->getBondHealths();

#if USE_PHYSX_NODE_INFO
    // traverse graph and fill node info,
    // essentially it does the same as m_solver->setAllNodesInfoFromLL() but fills mass, volume, transform from physx
    // and it also uses ExtPxChunk isStatic flag in addition to 'world' node in LL
    for (uint32_t node0 = 0; node0 < graph.nodeCount; ++node0)
    {
        uint32_t chunkIndex0 = graph.chunkIndices[node0];
        const ExtPxChunk* chunk0 = chunkIndex0 < chunkCount ? &chunks[chunkIndex0] : nullptr;
        bool isChunkStatic = true;

        if (chunk0)
        {
            isChunkStatic = chunk0->isStatic;
            for (uint32_t adjacencyIndex = graph.adjacencyPartition[node0]; adjacencyIndex < graph.adjacencyPartition[node0 + 1]; adjacencyIndex++)
            {
                uint32_t bondIndex = graph.adjacentBondIndices[adjacencyIndex];
                if (bondHealths[bondIndex] <= 0.0f)
                    continue;
                uint32_t node1 = graph.adjacentNodeIndices[adjacencyIndex];
                uint32_t chunkIndex1 = graph.chunkIndices[node1];
                if (chunkIndex1 < chunkCount)
                {
                    const ExtPxChunk& chunk1 = chunks[chunkIndex1];

                    if (chunk1.subchunkCount == 0 || chunk1.isStatic)
                    {
                        isChunkStatic |= chunk1.isStatic;
                        continue;
                    }
                }
                else
                {
                    isChunkStatic = true;
                    break;
                }
            }
        }

        // fill node info
        float mass;
        float volume;
        PxVec3 localPos;
        if (chunk0 && chunk0->subchunkCount > 0)
        {
            const ExtPxSubchunk& subChunk = subChunks[chunk0->firstSubchunkIndex];
            PxVec3 localCenterOfMass;
            PxMat33 intertia;
            PxVec3 scale = subChunk.geometry.scale.scale;
            subChunk.geometry.convexMesh->getMassInformation(mass, intertia, localCenterOfMass);
            mass *= scale.x * scale.y * scale.z;
            const PxTransform& chunk0LocalTransform = subChunk.transform;
            localPos = chunk0LocalTransform.transform(localCenterOfMass);
            volume = mass / 1.0f; // unit density
        }
        else
        {
            mass = 0.0f;
            volume = 0.0f;
            localPos = PxVec3(PxZero);
            isChunkStatic = true;
        }
        if (isChunkStatic)
        {
            mass = 0.0f;
        }
        m_solver->setNodeInfo(node0, mass, volume, fromPxShared(localPos));
    }
#else
    m_solver->setAllNodesInfoFromLL();
#endif

    // notify initial actor's created
    InlineArray<ExtPxActor*, 4>::type actors;;
    actors.resize(m_family.getActorCount());
    m_family.getActors(actors.begin(), actors.size());
    for (const auto actor : actors)
    {
        onActorCreated(m_family, *actor);
    }

    m_family.subscribe(*this);
}

ExtPxStressSolverImpl::~ExtPxStressSolverImpl()
{
    m_family.unsubscribe(*this);
    m_solver->release();
}

ExtPxStressSolver* ExtPxStressSolver::create(ExtPxFamily& family, ExtStressSolverSettings settings)
{
    return NVBLAST_NEW(ExtPxStressSolverImpl) (family, settings);
}

void ExtPxStressSolverImpl::release()
{
    NVBLAST_DELETE(this, ExtPxStressSolverImpl);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                  Update Wrapper
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ExtPxStressSolverImpl::update(bool doDamage)
{
    for (auto it = m_actors.getIterator(); !it.done(); ++it)
    {
        const ExtPxActor* actor = *it;

        PxRigidDynamic& rigidDynamic = actor->getPhysXActor();
        const bool isStatic = rigidDynamic.getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC;
        if (isStatic)
        {
            PxVec3 gravity = rigidDynamic.getScene()->getGravity();
            PxVec3 localGravity = rigidDynamic.getGlobalPose().rotateInv(gravity);

            m_solver->addGravity(*actor->getTkActor().getActorLL(), fromPxShared(localGravity));
        }
        else
        {
            PxVec3 localCenterMass = rigidDynamic.getCMassLocalPose().p;
            PxVec3 localAngularVelocity = rigidDynamic.getGlobalPose().rotateInv(rigidDynamic.getAngularVelocity());
            m_solver->addCentrifugalAcceleration(*actor->getTkActor().getActorLL(), fromPxShared(localCenterMass), fromPxShared(localAngularVelocity));
        }
    }

    m_solver->update();

    if (doDamage && m_solver->getOverstressedBondCount() > 0)
    {
        for (auto it = m_actors.getIterator(); !it.done(); ++it)
        {
            const ExtPxActor* actor = *it;
            NvBlastFractureBuffers commands;
            m_solver->generateFractureCommands(*actor->getTkActor().getActorLL(), commands);
            if (commands.bondFractureCount > 0)
            {
                m_family.getTkFamily().applyFracture(&commands);
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                  Actors
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ExtPxStressSolverImpl::onActorCreated(ExtPxFamily& /*family*/, ExtPxActor& actor)
{
    if (m_solver->notifyActorCreated(*actor.getTkActor().getActorLL()))
    {
        m_actors.insert(&actor);
    }
}

void ExtPxStressSolverImpl::onActorDestroyed(ExtPxFamily& /*family*/, ExtPxActor& actor)
{
    m_solver->notifyActorDestroyed(*actor.getTkActor().getActorLL());
    m_actors.erase(&actor);
}


} // namespace Blast
} // namespace Nv
