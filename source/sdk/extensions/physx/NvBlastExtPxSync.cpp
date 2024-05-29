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


#include "NvBlastExtPxSync.h"
#include "NvBlastAssert.h"
#include "NvBlast.h"
#include "NvBlastExtPxManager.h"
#include "NvBlastExtPxFamily.h"
#include "NvBlastExtPxActor.h"
#include "PxRigidDynamic.h"

#include <chrono>
using namespace std::chrono;

namespace Nv
{
namespace Blast
{

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                          ExtSyncImpl Definition
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ExtSyncImpl : public ExtSync
{
    NV_NOCOPY(ExtSyncImpl)

public:
    //////// ctor ////////

    ExtSyncImpl();
    
    virtual             ~ExtSyncImpl();


    //////// TkEventListener interface ////////

    virtual void        receive(const TkEvent* events, uint32_t eventCount) override;


    //////// ExtSync interface ////////

    virtual void        release() override;

    virtual void        syncFamily(const TkFamily& family) override;
    virtual void        syncFamily(const ExtPxFamily& family) override;

    virtual uint32_t    getSyncBufferSize() const override;
    virtual void        acquireSyncBuffer(const ExtSyncEvent*const*& buffer, uint32_t& size) const override;
    virtual void        releaseSyncBuffer() override;

    virtual void        applySyncBuffer(TkFramework& framework, const ExtSyncEvent** buffer, uint32_t size, TkGroup* groupForNewActors, ExtPxManager* manager) override;


private:
    //////// data ////////

    std::vector<ExtSyncEvent*>       m_syncEvents;
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                          ExtSyncEvent Implementation
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ExtSyncEvent::release()
{
    NVBLAST_DELETE(this, ExtSyncEvent);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                          ExtSyncImpl Implementation
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ExtSync* ExtSync::create()
{
    return NVBLAST_NEW(ExtSyncImpl) ();
}

void ExtSyncImpl::release()
{
    NVBLAST_DELETE(this, ExtSyncImpl);
}

ExtSyncImpl::ExtSyncImpl()
{
}

ExtSyncImpl::~ExtSyncImpl()
{
    releaseSyncBuffer();
}

void ExtSyncImpl::receive(const TkEvent* events, uint32_t eventCount)
{
    for (uint32_t i = 0; i < eventCount; ++i)
    {
        const TkEvent& tkEvent = events[i];
        if (tkEvent.type == TkEvent::FractureCommand)
        {
            const TkFractureCommands* fracEvent = tkEvent.getPayload<TkFractureCommands>();
            ExtSyncEventFracture* e = NVBLAST_NEW(ExtSyncEventFracture) ();
            e->timestamp = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
            e->familyID = fracEvent->tkActorData.family->getID();
            e->bondFractures.resize(fracEvent->buffers.bondFractureCount);
            e->chunkFractures.resize(fracEvent->buffers.chunkFractureCount);
            memcpy(e->bondFractures.data(), fracEvent->buffers.bondFractures, e->bondFractures.size() * sizeof(NvBlastBondFractureData));
            memcpy(e->chunkFractures.data(), fracEvent->buffers.chunkFractures, e->chunkFractures.size() * sizeof(NvBlastChunkFractureData));
            m_syncEvents.push_back(e);
        }
    }
}

void ExtSyncImpl::syncFamily(const TkFamily& family)
{
    ExtSyncEventFamilySync* e = NVBLAST_NEW(ExtSyncEventFamilySync) ();
    e->timestamp = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    e->familyID = family.getID();
    const NvBlastFamily* familyLL = family.getFamilyLL();
    const uint32_t size = NvBlastFamilyGetSize(familyLL, logLL);
    e->family = std::vector<char>((char*)familyLL, (char*)familyLL + size);
    m_syncEvents.push_back(e);
}

void ExtSyncImpl::syncFamily(const ExtPxFamily& family)
{
    const TkFamily& tkFamily = family.getTkFamily();

    syncFamily(tkFamily);

    ExtSyncEventPhysicsSync* e = NVBLAST_NEW(ExtSyncEventPhysicsSync) ();
    e->timestamp = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    e->familyID = tkFamily.getID();
    std::vector<ExtPxActor*> actors(family.getActorCount());
    family.getActors(actors.data(), static_cast<uint32_t>(actors.size()));
    e->data.reserve(actors.size());
    for (ExtPxActor* actor : actors)
    {
        ExtSyncEventPhysicsSync::ActorData data;
        data.transform = actor->getPhysXActor().getGlobalPose();
        data.actorIndex = actor->getTkActor().getIndex();
        e->data.push_back(data);
    }

    m_syncEvents.push_back(e);
}

uint32_t ExtSyncImpl::getSyncBufferSize() const
{
    return static_cast<uint32_t>(m_syncEvents.size());
}

void ExtSyncImpl::acquireSyncBuffer(const ExtSyncEvent* const*& buffer, uint32_t& size) const
{
    buffer = m_syncEvents.data();
    size = static_cast<uint32_t>(m_syncEvents.size());
}

void ExtSyncImpl::releaseSyncBuffer()
{
    for (uint32_t i = 0; i < m_syncEvents.size(); ++i)
    {
        NVBLAST_DELETE(m_syncEvents[i], ExtSyncEvent);
    }
    m_syncEvents.clear();
}

void ExtSyncImpl::applySyncBuffer(TkFramework& framework, const ExtSyncEvent** buffer, uint32_t size, TkGroup* groupForNewActors, ExtPxManager* manager)
{
    const TkType* familyType = framework.getType(TkTypeIndex::Family);
    NVBLAST_ASSERT(familyType);

    for (uint32_t i = 0; i < size; ++i)
    {
        const ExtSyncEvent* e = buffer[i];
        const NvBlastID& id = e->familyID;
        TkIdentifiable* object = framework.findObjectByID(id);
        if (object && object->getType() == *familyType)
        {
            TkFamily* family = static_cast<TkFamily*>(object);

            if (e->type == ExtSyncEventFracture::EVENT_TYPE)
            {
                const ExtSyncEventFracture* fractureEvent = e->getEvent<ExtSyncEventFracture>();
                const NvBlastFractureBuffers commands =
                {
                    static_cast<uint32_t>(fractureEvent->bondFractures.size()),
                    static_cast<uint32_t>(fractureEvent->chunkFractures.size()),
                    const_cast<NvBlastBondFractureData*>(fractureEvent->bondFractures.data()),
                    const_cast<NvBlastChunkFractureData*>(fractureEvent->chunkFractures.data())
                };
                family->applyFracture(&commands);
            }
            else if (e->type == ExtSyncEventFamilySync::EVENT_TYPE)
            {
                const ExtSyncEventFamilySync* familyEvent = e->getEvent<ExtSyncEventFamilySync>();
                family->reinitialize((NvBlastFamily*)familyEvent->family.data(), groupForNewActors);
            }
            else if (e->type == ExtSyncEventPhysicsSync::EVENT_TYPE && manager)
            {
                const ExtSyncEventPhysicsSync* physicsEvent = e->getEvent<ExtSyncEventPhysicsSync>();
                ExtPxFamily* pxFamily = manager->getFamilyFromTkFamily(*family);
                if (pxFamily)
                {
                    std::vector<ExtPxActor*> actors(pxFamily->getActorCount());
                    pxFamily->getActors(actors.data(), static_cast<uint32_t>(actors.size()));

                    for (auto data : physicsEvent->data)
                    {
                        for (ExtPxActor* physicsaActor : actors)
                        {
                            if (data.actorIndex == physicsaActor->getTkActor().getIndex())
                            {
                                physicsaActor->getPhysXActor().setGlobalPose(data.transform);
                            }
                        }
                    }
                }
            }
        }
    }
}

} // namespace Blast
} // namespace Nv
