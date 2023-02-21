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
// Copyright (c) 2008-2023 NVIDIA Corporation. All rights reserved.


#ifndef RENDERABLE_H
#define RENDERABLE_H

#include "RenderMaterial.h"
#include <DirectXMath.h>
#include "PxMat44.h"
#include "PxVec3.h"
#include "PxVec4.h"

using namespace physx;

class Renderer;

/**
RenderMesh interface, used by Renderable
*/
class IRenderMesh
{
public:
    virtual ~IRenderMesh() {}
    virtual const std::vector<D3D11_INPUT_ELEMENT_DESC>& getInputElementDesc() const = 0;
    virtual void render(ID3D11DeviceContext& context) const = 0;
};

/**
Renderable, represents single object renderer by Renderer.
Basically Renderable = RenderMaterial + RenderMesh
*/
class Renderable
{
public:
    //////// public API ////////

    void setMaterial(RenderMaterial& material);

    PxMat44 getModelMatrix() const
    {
        return PxMat44(m_transform) * PxMat44(PxVec4(m_scale, 1));
    }

    void setTransform(PxTransform& transform)
    {
        m_transform = transform;
    }

    const PxTransform& getTransform() const
    {
        return m_transform;
    }

    void setScale(PxVec3 scale)
    {
        m_scale = scale;
    }

    const PxVec3& getScale() const
    {
        return m_scale;
    }

    void setColor(DirectX::XMFLOAT4 color)
    {
        m_color = color;
    }
    DirectX::XMFLOAT4 getColor() const
    {
        return m_color;
    }

    void setHidden(bool hidden)
    {
        m_hidden = hidden;
    }

    bool isHidden() const 
    { 
        return m_hidden;
    }

    bool isTransparent() const
    {
        return !(m_materialInstance->getMaterial().getBlending() == RenderMaterial::BLEND_NONE);
    }

    RenderMaterial& getMaterial() const { return m_materialInstance->getMaterial(); }

private:
    //////// methods used by Renderer ////////

    friend class Renderer;

    void render(Renderer& renderer) const
    {
        render(renderer, false);
    }

    void renderDepthStencilOnly(Renderer& renderer) const
    {
        render(renderer, true);
    }

    Renderable(IRenderMesh& mesh, RenderMaterial& material);

    void render(Renderer& renderer, bool depthStencilOnly) const;


    //////// internal data ////////

    DirectX::XMFLOAT4           m_color;
    PxTransform                 m_transform;
    PxVec3                      m_scale;

    RenderMaterial::InstancePtr m_materialInstance;
    IRenderMesh&                m_mesh;
    bool                        m_hidden;
};

#endif //RENDERABLE_H