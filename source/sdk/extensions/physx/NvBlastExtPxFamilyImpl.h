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


#ifndef NVBLASTEXTPXFAMILYIMPL_H
#define NVBLASTEXTPXFAMILYIMPL_H

#include "NvBlastExtPxFamily.h"
#include "NvBlastArray.h"
#include "NvBlastHashSet.h"
#include "foundation/PxTransform.h"
#include "NvBlastTkEvent.h"


using namespace physx;


namespace Nv
{
namespace Blast
{

// Forward declarations
class ExtPxManagerImpl;
class ExtPxActorImpl;
struct PxActorCreateInfo;


class ExtPxFamilyImpl final : public ExtPxFamily, TkEventListener
{
    NV_NOCOPY(ExtPxFamilyImpl)

public:
    friend ExtPxActorImpl;
    friend ExtPxManagerImpl;

    //////// ctor ////////

    ExtPxFamilyImpl(ExtPxManagerImpl& manager, TkFamily& tkFamily, ExtPxAsset& pxAsset);
    ~ExtPxFamilyImpl();

    virtual void release() override;


    //////// ExtPxFamily interface ////////

//  virtual bool                            spawn(const PxTransform& pose, const ExtPxSpawnSettings& settings) override;
    virtual bool                            spawn(const physx::PxTransform& pose, const physx::PxVec3& scale, const ExtPxSpawnSettings& settings) override;
    virtual bool                            despawn() override;


    virtual uint32_t                        getActorCount() const override
    {
        return m_actors.size();
    }

    virtual uint32_t                        getActors(ExtPxActor** buffer, uint32_t bufferSize) const override
    {
        uint32_t index = 0;
        for (auto it = const_cast<ExtPxFamilyImpl*>(this)->m_actors.getIterator(); !it.done() && index < bufferSize; ++it)
        {
            buffer[index++] = *it;
        }
        return index;
    }

    virtual TkFamily&                       getTkFamily() const override
    {
        return m_tkFamily;
    }

    virtual const physx::PxShape* const*    getSubchunkShapes() const override
    {
        return m_subchunkShapes.begin();
    }

    virtual ExtPxAsset&                     getPxAsset() const override
    {
        return m_pxAsset;
    }

    virtual void                            setMaterial(PxMaterial& material) override
    {
        m_spawnSettings.material = &material;
    }

    virtual void                            setPxShapeDescTemplate(const ExtPxShapeDescTemplate* pxShapeDesc) override
    {
        m_pxShapeDescTemplate = pxShapeDesc;
    }

    virtual const ExtPxShapeDescTemplate*   getPxShapeDescTemplate() const override
    {
        return m_pxShapeDescTemplate;
    }

    virtual void                            setPxActorDesc(const ExtPxActorDescTemplate* pxActorDesc) override
    {
        m_pxActorDescTemplate = pxActorDesc;
    }

    virtual const ExtPxActorDescTemplate*   getPxActorDesc() const override
    {
        return m_pxActorDescTemplate;
    }

    virtual const NvBlastExtMaterial*       getMaterial() const override
    {
        return m_material;
    }

    virtual void                            setMaterial(const NvBlastExtMaterial* material) override
    {
        m_material = material;
    }

    virtual void                            subscribe(ExtPxListener& listener) override
    {
        m_listeners.pushBack(&listener);
    }

    virtual void                            unsubscribe(ExtPxListener& listener) override
    {
        m_listeners.findAndReplaceWithLast(&listener);
    }

    virtual void                            postSplitUpdate() override;

    //////// TkEventListener interface ////////

    virtual void                            receive(const TkEvent* events, uint32_t eventCount) override;


    //////// events dispatch ////////

    void                                    dispatchActorCreated(ExtPxActor& actor);
    void                                    dispatchActorDestroyed(ExtPxActor& actor);


private:
    //////// private methods ////////

    void                                    createActors(TkActor** tkActors, const PxActorCreateInfo* pxActorInfos, uint32_t count);
    void                                    destroyActors(ExtPxActor** actors, uint32_t count);

    //////// data ////////

    ExtPxManagerImpl&                       m_manager;
    TkFamily&                               m_tkFamily;
    ExtPxAsset&                             m_pxAsset;
    ExtPxSpawnSettings                      m_spawnSettings;
    const ExtPxShapeDescTemplate*           m_pxShapeDescTemplate;
    const ExtPxActorDescTemplate*           m_pxActorDescTemplate;
    const NvBlastExtMaterial*               m_material;
    bool                                    m_isSpawned;
    PxTransform                             m_initialTransform;
    PxVec3                                  m_initialScale;
    HashSet<ExtPxActor*>::type              m_actors;
    Array<TkActor*>::type                   m_culledActors;
    InlineArray<ExtPxListener*, 4>::type    m_listeners;
    Array<PxShape*>::type                   m_subchunkShapes;
    Array<TkActor*>::type                   m_newActorsBuffer;
    Array<PxActorCreateInfo>::type          m_newActorCreateInfo;
    Array<PxActor*>::type                   m_physXActorsBuffer;
    Array<ExtPxActor*>::type                m_actorsBuffer;
    Array<uint32_t>::type                   m_indicesScratch;
};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTPXFAMILYIMPL_H
