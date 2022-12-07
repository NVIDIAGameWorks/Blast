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
// Copyright (c) 2008-2022 NVIDIA Corporation. All rights reserved.


#ifndef BLAST_CONTROLLER_H
#define BLAST_CONTROLLER_H

#include "SampleManager.h"
#include "BlastFamily.h"
#include "DebugRenderBuffer.h"
#include "PxSimulationEventCallback.h"
#include "NvBlastExtPxImpactDamageManager.h"

using namespace physx;

class BlastAsset;
class BlastReplay;

namespace physx
{
class PxTaskManager;
}
namespace Nv
{
namespace Blast
{
class TkFramework;
class ExtGroupTaskManager;
class ExtSerialization;
}
}


struct BlastTimers
{
    double blastDamageMaterial;
    double blastDamageFracture;
    double blastSplitIsland;
    double blastSplitPartition;
    double blastSplitVisibility;
};

/**
Blast Controller. Entry point for all blast related code, keeps blast actors and controls them.
*/
class BlastController : public ISampleController
{
public:
    //////// ctor ////////

    BlastController();
    virtual ~BlastController();

    void reinitialize();

    //////// controller callbacks ////////

    virtual void onSampleStart();
    virtual void onSampleStop();

    virtual void Animate(double dt);
    void drawUI();


    //////// public API ////////

    bool overlap(const PxGeometry& geometry, const PxTransform& pose, std::function<void(ExtPxActor*, BlastFamily&)> hitCall);

    bool stressDamage(ExtPxActor *actor, PxVec3 position, PxVec3 force);

    void deferDamage(ExtPxActor *actor, BlastFamily& family, const NvBlastDamageProgram& program, const void* damageDesc, uint32_t damageDescSize);
    void immediateDamage(ExtPxActor *actor, BlastFamily& family, const NvBlastDamageProgram& program, const void* damageDesc);
    NvBlastFractureBuffers& getFractureBuffers(ExtPxActor* actor);

    BlastFamilyPtr spawnFamily(BlastAsset* blastAsset, const BlastAsset::ActorDesc& desc);
    void removeFamily(BlastFamilyPtr actor);
    void removeAllFamilies();


    //////// public getters/setters ////////

    TkFramework& getTkFramework() const
    {
        return *m_tkFramework;
    }

    TkGroup* getTkGroup() const
    {
        return m_tkGroup;
    }

    ExtPxManager& getExtPxManager() const
    {
        return *m_extPxManager;
    }

    ExtImpactDamageManager* getExtImpactDamageManager() const
    {
        return m_extImpactDamageManager;
    }

    BlastReplay* getReplay() const
    {
        return m_replay;
    }

    uint32_t getActorCount() const;

    uint32_t getTotalVisibleChunkCount() const;

    size_t getFamilySize() const;

    size_t getBlastAssetsSize() const
    {
        return m_blastAssetsSize;
    }

    const BlastTimers& getLastBlastTimers() const
    {
        return m_lastBlastTimers;
    }

    bool getImpactDamageEnabled() const
    {
        return m_impactDamageEnabled;
    }

    void setImpactDamageEnabled(bool enabled, bool forceUpdate = false);

    ExtStressSolverSettings& getStressSolverSettings()
    {
        return m_extStressSolverSettings;
    }

    float getLastStressDelta() const;

    void notifyPhysXControllerRelease();

    ExtSerialization*   getExtSerialization() const
    {
        return m_extSerialization;
    }

    //////// public variables for UI ////////

    BlastFamily::DebugRenderMode debugRenderMode;
    float debugRenderScale;


    //////// Filter shader enum ////////

    enum FilterDataAttributes
    {
        SUPPRESS_CONTACT_NOTIFY = 1,
    };

private:
    //////// impact damage event callback ////////

    class EventCallback : public PxSimulationEventCallback
    {
    public:
        EventCallback(ExtImpactDamageManager* manager) : m_manager(manager) {}

        // implemented
        virtual void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, uint32_t nbPairs)
        {
            m_manager->onContact(pairHeader, pairs, nbPairs);
        }

    private:
        // unused
        void onConstraintBreak(PxConstraintInfo*, PxU32) {}
        void onWake(PxActor**, PxU32) {}
        void onSleep(PxActor**, PxU32) {}
        void onTrigger(PxTriggerPair*, PxU32) {}
        void onAdvance(const PxRigidBody*const*, const PxTransform*, const PxU32) {}

        // data
        ExtImpactDamageManager*     m_manager;
    };


    //////// private methods ////////

    void updateDraggingStress();

    void updateImpactDamage();

    void refreshImpactDamageSettings();

    void fillDebugRender();

    void recalculateAssetsSize();

    static bool customImpactDamageFunction(void* data, ExtPxActor* actor, PxShape* shape, PxVec3 position, PxVec3 force);


    //////// used controllers ////////

    Renderer& getRenderer() const
    {
        return getManager()->getRenderer();
    }

    PhysXController& getPhysXController() const
    {
        return getManager()->getPhysXController();
    }


    //////// buffer for damage ////////

    class FixedBuffer
    {
    public:
        FixedBuffer(const uint32_t size)
        {
            m_buffer.resize(size);
            m_index = 0;
        }

        void* push(const void* data, uint32_t size)
        {
            if (m_index + size > m_buffer.size())
                return nullptr;

            void* dst = &m_buffer[m_index];
            memcpy(dst, data, size);
            m_index += size;
            return dst;
        }

        void clear()
        {
            m_index = 0;
        }

    private:
        std::vector<char> m_buffer;
        uint32_t          m_index;
    };


    //////// internal data ////////

    PxTaskManager*                       m_taskManager;
    TkFramework*                         m_tkFramework;
    TkGroup*                             m_tkGroup;
    ExtPxManager*                        m_extPxManager;
    ExtImpactDamageManager*              m_extImpactDamageManager;
    ExtImpactSettings                    m_extImpactDamageManagerSettings;
    EventCallback*                       m_eventCallback;
    ExtStressSolverSettings              m_extStressSolverSettings;
    ExtGroupTaskManager*                 m_extGroupTaskManager;
    ExtSerialization*                    m_extSerialization;

    std::vector<BlastFamilyPtr>          m_families;
    DebugRenderBuffer                    m_debugRenderBuffer;

    FixedBuffer                          m_damageDescBuffer;
    FixedBuffer                          m_damageParamsBuffer;

    NvBlastFractureBuffers               m_fractureBuffers;
    std::vector<char>                    m_fractureData;

    bool                                 m_impactDamageEnabled;
    bool                                 m_impactDamageUpdatePending;
    bool                                 m_impactDamageToStressEnabled;

    float                                m_impactDamageToStressFactor;
    float                                m_draggingToStressFactor;

    bool                                 m_rigidBodyLimitEnabled;
    uint32_t                             m_rigidBodyLimit;

    BlastReplay*                         m_replay;

    BlastTimers                          m_lastBlastTimers;

    size_t                               m_blastAssetsSize;
};


#endif
