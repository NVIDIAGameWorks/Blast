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


#ifndef DAMAGE_TOOL_CONTROLLER_H
#define DAMAGE_TOOL_CONTROLLER_H

#include "SampleManager.h"
#include "NvBlastTypes.h"
#include <DirectXMath.h>
#include <functional>
#include "foundation/PxVec2.h"
#include "foundation/PxVec3.h"


class Renderable;
class RenderMaterial;
class BlastFamily;

namespace Nv
{
namespace Blast
{
class ExtPxActor;
}
}



class DamageToolController : public ISampleController
{
public:
    DamageToolController();
    virtual ~DamageToolController();

    virtual LRESULT MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual void Animate(double dt);
    void drawUI();


    virtual void onInitialize();
    virtual void onSampleStart();
    virtual void onSampleStop();

    bool isDamageMode() const
    {
        return m_damageMode;
    }

private:
    DamageToolController& operator= (DamageToolController&);


    //////// private methods ////////

    void changeDamageRadius(float dr);

    void setDamageMode(bool enabled);


    //////// used controllers ////////

    Renderer& getRenderer() const
    {
        return getManager()->getRenderer();
    }

    PhysXController& getPhysXController() const
    {
        return getManager()->getPhysXController();
    }

    BlastController& getBlastController() const
    {
        return getManager()->getBlastController();
    }


    //////// internal data ////////

    RenderMaterial*   m_toolRenderMaterial;
    Renderable*       m_sphereToolRenderable;
    DirectX::XMFLOAT4 m_toolColor;
    Renderable*       m_lineToolRenderable;

    float             m_damage;
    float             m_explosiveImpulse;
    float             m_stressForceFactor;

    struct Damager
    {
        Damager() : damageWhilePressed(false), radius(5.0f), radiusLimit(1000.0f)
        {
        }

        enum PointerType
        {
            Sphere,
            Line
        };

        struct DamageData
        {
            physx::PxVec3 origin;
            physx::PxVec3 hitPosition;
            physx::PxVec3 hitNormal;
            physx::PxVec3 weaponDir;
            physx::PxVec3 previousWeaponDir;
        };

        typedef std::function<void(const Damager* damager, Nv::Blast::ExtPxActor* actor, BlastFamily& family, const DamageData& damageData)> ExecuteFn;

        const char*             uiName;
        NvBlastDamageProgram    program;
        PointerType             pointerType;
        DirectX::XMFLOAT4       pointerColor;
        float                   radius;
        float                   radiusLimit;
        bool                    damageWhilePressed;
        ExecuteFn               executeFunction;
    };

    std::vector<Damager>     m_damagers;
    std::vector<const char*> m_damagerNames;
    uint32_t                 m_damagerIndex;

    bool                     m_damageMode;

    physx::PxVec2            m_lastMousePos;
    bool                     m_isMousePressed;
    uint32_t                 m_damageCountWhilePressed;
    physx::PxVec3            m_previousPickDir;
    bool                     m_prevWasHit;
};

#endif