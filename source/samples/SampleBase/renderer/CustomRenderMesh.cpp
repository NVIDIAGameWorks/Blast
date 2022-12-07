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


#include "CustomRenderMesh.h"


CustomRenderMesh::CustomRenderMesh()
    : m_indexBuffer(nullptr)
{
}

CustomRenderMesh::CustomRenderMesh(const void* vertices, uint32_t numVertices, uint32_t vertexSize, std::vector<D3D11_INPUT_ELEMENT_DESC>& inputDesc, const uint16_t* faces, uint32_t numFaces)
    : m_indexBuffer(nullptr)
{
    initialize(vertices, numVertices, vertexSize, inputDesc, faces, numFaces);
}

void CustomRenderMesh::initialize(const void* vertices, uint32_t numVertices, uint32_t vertexSize, std::vector<D3D11_INPUT_ELEMENT_DESC>& inputDesc, const uint16_t* faces, uint32_t numFaces)
{
    ID3D11Device* device = GetDeviceManager()->GetDevice();

    m_inputDesc = inputDesc;
    m_numVertices = numVertices;
    m_vertexSize = vertexSize;
    m_numFaces = numFaces;

    // VB
    {
        D3D11_SUBRESOURCE_DATA vertexBufferData;

        ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
        vertexBufferData.pSysMem = vertices;

        D3D11_BUFFER_DESC bufferDesc;

        memset(&bufferDesc, 0, sizeof(D3D11_BUFFER_DESC));
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.ByteWidth = vertexSize * numVertices;
        bufferDesc.CPUAccessFlags = 0;
        bufferDesc.MiscFlags = 0;
        bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;

        V(device->CreateBuffer(&bufferDesc, &vertexBufferData, &m_vertexBuffer));
    }

    // IB
    if (faces != nullptr)
    {
        D3D11_SUBRESOURCE_DATA indexBufferData;

        ZeroMemory(&indexBufferData, sizeof(indexBufferData));
        indexBufferData.pSysMem = faces;

        D3D11_BUFFER_DESC bufferDesc;

        memset(&bufferDesc, 0, sizeof(D3D11_BUFFER_DESC));
        bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bufferDesc.ByteWidth = sizeof(uint16_t) * numFaces;
        bufferDesc.CPUAccessFlags = 0;
        bufferDesc.MiscFlags = 0;
        bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;

        V(device->CreateBuffer(&bufferDesc, &indexBufferData, &m_indexBuffer));
    }
}

CustomRenderMesh::~CustomRenderMesh()
{
    SAFE_RELEASE(m_vertexBuffer);
    SAFE_RELEASE(m_indexBuffer);
}


void CustomRenderMesh::render(ID3D11DeviceContext& context) const
{
    context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    UINT strides[1] = { m_vertexSize };
    UINT offsets[1] = { 0 };
    context.IASetVertexBuffers(0, 1, &m_vertexBuffer, strides, offsets);

    context.IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R16_UINT, 0);

    if (m_indexBuffer)
        context.DrawIndexed(m_numFaces, 0, 0);
    else
        context.Draw(m_numVertices, 0);
}

