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


#ifndef SKINNED_RENDER_MESH_H
#define SKINNED_RENDER_MESH_H

#include "Utils.h"
#include <DirectXMath.h>

#include <vector>
#include "Renderable.h"
#include "Mesh.h"

/**
SkinnedRenderMesh:
    bonde indices are passed as vertex input,
    bone transforms are stored in texture
    max bone meshes count: SkinnedRenderMesh::MeshesCountMax
*/
class SkinnedRenderMesh : public IRenderMesh
{
public:
    //////// ctor ////////

    SkinnedRenderMesh(const std::vector<const SimpleMesh*>& meshes);
    ~SkinnedRenderMesh();


    //////// const ////////

    static const uint32_t MeshesCountMax = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;


    //////// public API ////////

    void updateVisibleMeshes(const std::vector<uint32_t>& visibleMeshes);
    void updateVisibleMeshTransforms(std::vector<PxMat44>& transforms);


    //////// IRenderMesh implementation ////////

    virtual const std::vector<D3D11_INPUT_ELEMENT_DESC>& getInputElementDesc() const { return m_inputDesc; }
    virtual void render(ID3D11DeviceContext& context) const;

private:
    //////// internal data ////////

    struct MeshInfo
    {
        uint32_t firstIndex;
        uint32_t indicesCount;

        uint32_t firstVertex;
        uint32_t verticesCount;
    };

    std::vector<D3D11_INPUT_ELEMENT_DESC> m_inputDesc;

    ID3D11Device* m_device;

    ID3D11Buffer* m_vertexBuffer;
    ID3D11Buffer* m_boneIndexBuffer;
    ID3D11Buffer* m_indexBuffer;
    ID3D11Texture2D* m_boneTexture;
    ID3D11ShaderResourceView* m_boneTextureSRV;

    uint32_t m_indexCount;

    std::vector<MeshInfo> m_meshesInfo;
    std::vector<uint32_t> m_indices;
};



#endif //SKINNED_RENDER_MESH_H