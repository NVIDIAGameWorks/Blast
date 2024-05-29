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
// Copyright (c) 2008-2024 NVIDIA Corporation. All rights reserved.


#include "Renderable.h"
#include "Renderer.h"
#include "RenderUtils.h"

const DirectX::XMFLOAT4 DEFAULT_COLOR(0.5f, 0.5f, 0.5f, 1.0f);

Renderable::Renderable(IRenderMesh& mesh, RenderMaterial& material) : m_mesh(mesh), m_scale(1, 1, 1), m_color(DEFAULT_COLOR), m_hidden(false), m_transform(PxIdentity)
{
    setMaterial(material);
}

void Renderable::setMaterial(RenderMaterial& material)
{
    m_materialInstance = material.getMaterialInstance(&m_mesh);
}

void Renderable::render(Renderer& renderer, bool depthStencilOnly) const
{
    if (!m_materialInstance->isValid())
    {
        PX_ALWAYS_ASSERT();
        return;
    }

    m_materialInstance->bind(*renderer.m_context, 0, depthStencilOnly);

    // setup object CB
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        renderer.m_context->Map(renderer.m_objectCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        Renderer::CBObject* objectBuffer = (Renderer::CBObject*)mappedResource.pData;
        objectBuffer->world = PxMat44ToXMMATRIX(getModelMatrix());
        objectBuffer->color = getColor();
        renderer.m_context->Unmap(renderer.m_objectCB, 0);
    }

    m_mesh.render(*renderer.m_context);
}
