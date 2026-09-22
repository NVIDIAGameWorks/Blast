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


#include "RenderMaterial.h"
#include <DirectXMath.h>
#include "ShaderUtils.h"
#include "Renderer.h"
#include "PxAssert.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                              RenderMaterial
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RenderMaterial::RenderMaterial(ResourceManager& resourceCallback, const char* shaderFileName,
    const char* textureFileName, BlendMode blendMode)
{
    this->initialize(resourceCallback, shaderFileName, textureFileName, blendMode);
}

void RenderMaterial::initialize(ResourceManager& resourceCallback, const char* shaderFileName, const char* textureFileName, BlendMode blendMode)
{
    std::vector<std::string> v;
    v.push_back(shaderFileName);
    initialize(resourceCallback, v, textureFileName, blendMode);
}

void RenderMaterial::initialize(ResourceManager& resourceCallback, std::vector<std::string> shaderFileNames, const char* textureFileName, BlendMode blendMode)
{
    mTextureSRV = nullptr;
    mTexture = nullptr;
    mBlendState = nullptr;
    mTextureFileName = textureFileName;

    for (uint32_t i = 0; i < shaderFileNames.size(); i++)
    {
        const ShaderFileResource* resource = resourceCallback.requestShaderFile(shaderFileNames[i].c_str());
        if (resource)
        {
            std::string shaderFilePath = resource->path;
            mShaderFilePathes.push_back(shaderFilePath);
        }
    }
    mShaderGroups.reserve(mShaderFilePathes.size());

    if (!mTextureFileName.empty())
    {
        mTexture = resourceCallback.requestTexture(mTextureFileName.c_str());
    }

    setBlending(blendMode);

    reload();
}

void RenderMaterial::releaseReloadableResources()
{
    for (std::vector<ShaderGroup*>::iterator it = mShaderGroups.begin(); it != mShaderGroups.end(); it++)
    {
        delete *it;
    }
    mShaderGroups.clear();

    SAFE_RELEASE(mTextureSRV);
}

RenderMaterial::~RenderMaterial()
{
    releaseReloadableResources();
    SAFE_RELEASE(mBlendState);
}

void RenderMaterial::setBlending(BlendMode blendMode)
{
    mBlendMode = blendMode;

    SAFE_RELEASE(mBlendState);

    D3D11_BLEND_DESC desc;
    ZeroMemory(&desc, sizeof(desc));

    switch (blendMode)
    {
    case BLEND_NONE:
        desc.RenderTarget[0].BlendEnable = FALSE;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        break;
    case BLEND_ALPHA_BLENDING:
        desc.AlphaToCoverageEnable = FALSE;
        desc.IndependentBlendEnable = TRUE;
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        break;
    case BLEND_ADDITIVE: // actually, is's additive by alpha
        desc.AlphaToCoverageEnable = FALSE;
        desc.IndependentBlendEnable = TRUE;
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        break;
    default:
        PX_ALWAYS_ASSERT_MESSAGE("Unknown blend mode");
    }

    ID3D11Device* device = GetDeviceManager()->GetDevice();
    V(device->CreateBlendState(&desc, &mBlendState));
}

void RenderMaterial::reload()
{
    releaseReloadableResources();

    // load shaders
    ID3D11Device* device = GetDeviceManager()->GetDevice();

    for (std::vector<std::string>::iterator it = mShaderFilePathes.begin(); it != mShaderFilePathes.end(); it++)
    {
        const char* shaderFilePath = (*it).c_str();
        ShaderGroup* shaderGroup = new ShaderGroup();
        V(createShaderFromFile(device, shaderFilePath, "VS", &(shaderGroup->vs), shaderGroup->buffer));
        createShaderFromFile(device, shaderFilePath, "PS", &shaderGroup->ps);
        createShaderFromFile(device, shaderFilePath, "GS", &shaderGroup->gs);
        mShaderGroups.push_back(shaderGroup);
    }

    // load texture
    if (mTexture)
    {
        V(DirectX::CreateShaderResourceView(device, mTexture->image.GetImages(), mTexture->image.GetImageCount(),
                                            mTexture->metaData, &mTextureSRV));
    }
}



RenderMaterial::InstancePtr RenderMaterial::getMaterialInstance(const IRenderMesh* mesh)
{
    // look in cache
    auto it = mRenderMeshToInstanceMap.find(mesh);
    if (it != mRenderMeshToInstanceMap.end())
    {
        if (!(*it).second.expired())
        {
            return (*it).second.lock();
        }
    }

    // create new
    const std::vector<D3D11_INPUT_ELEMENT_DESC>& descs = mesh->getInputElementDesc();
    RenderMaterial::InstancePtr instance = getMaterialInstance(&descs[0], (uint32_t)descs.size());
    mRenderMeshToInstanceMap[mesh] = instance;
    return instance;
}

RenderMaterial::InstancePtr RenderMaterial::getMaterialInstance(const D3D11_INPUT_ELEMENT_DESC* elementDescs, uint32_t numElements)
{
    ID3D11Device* device = GetDeviceManager()->GetDevice();

    for (uint32_t i = 0; i < mShaderGroups.size(); i++)
    {
        if (mShaderGroups[i]->buffer == NULL)
            continue;

        ID3D11InputLayout* inputLayout = NULL;
        device->CreateInputLayout(elementDescs, numElements, mShaderGroups[i]->buffer->GetBufferPointer(), mShaderGroups[i]->buffer->GetBufferSize(), &inputLayout);

        if (inputLayout)
        {
            RenderMaterial::InstancePtr materialInstance(new Instance(*this, inputLayout, i));
            return materialInstance;
        }
    }
    PX_ALWAYS_ASSERT();
    return NULL;
}

void RenderMaterial::Instance::bind(ID3D11DeviceContext& context, uint32_t slot, bool depthStencilOnly)
{
    mMaterial.mShaderGroups[mShaderNum]->Set(&context, !depthStencilOnly);

    context.OMSetBlendState(mMaterial.mBlendState, nullptr, 0xFFFFFFFF);
    context.PSSetShaderResources(slot, 1, &(mMaterial.mTextureSRV));
    context.IASetInputLayout(mInputLayout);
}

bool RenderMaterial::Instance::isValid()
{
    return mMaterial.mShaderGroups.size() > 0 && mMaterial.mShaderGroups[mShaderNum]->IsValid();
}
