// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2017 NVIDIA Corporation. All rights reserved.


#include "RenderMaterial.h"
#include <DirectXMath.h>
#include "ShaderUtils.h"
#include "Renderer.h"
#include "SampleManager.h"
#include "Light.h"
#include "D3D11TextureResource.h"
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												RenderMaterial
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RenderMaterial::RenderMaterial(const char* materialName, ResourceManager& resourceCallback, const char* shaderFileName,
	const char* textureFileName, BlendMode blendMode)
{
	mMaterialName = materialName;
	setDiffuseColor(0.5, 0.5, 0.5, 1.0);
	mTextureFileNames[TT_Diffuse] = textureFileName;

	this->initialize(resourceCallback, shaderFileName, textureFileName, blendMode);
}

RenderMaterial::RenderMaterial(const char* materialName, ResourceManager& resourceCallback, const char* shaderFileName, float r, float g, float b, float a, BlendMode blendMode)
{
	mMaterialName = materialName;
	setDiffuseColor(r, g, b, a);
	this->initialize(resourceCallback, shaderFileName, "", blendMode);
}

void RenderMaterial::initialize(ResourceManager& resourceCallback, const char* shaderFileName, const char* textureFileName, BlendMode blendMode)
{
	std::vector<std::string> v;
	v.push_back(shaderFileName);
	initialize(resourceCallback, v, textureFileName, blendMode);
}

void RenderMaterial::initialize(ResourceManager& resourceCallback, std::vector<std::string> shaderFileNames, const char* textureFileName, BlendMode blendMode)
{
	for (int i = 0; i < TT_Num; i++)
	{
		m_TextureSRVs[i] = nullptr;
		mTextureResources[i] = nullptr;
	}
	mBlendState = nullptr;

	memset(mSpecularColor, 0, sizeof(float) * 4);
	mSpecularShininess = 20;

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

	if (!mTextureFileNames[TT_Diffuse].empty())
	{
		mTextureResources[TT_Diffuse] = resourceCallback.requestTexture(mTextureFileNames[TT_Diffuse].c_str());
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

// Add By Lixu Begin
	for (std::map<const IRenderMesh*, Instance*>::iterator it = mRenderMeshToInstanceMap.begin();
		it != mRenderMeshToInstanceMap.end(); it++)
	{
		delete it->second;
	}
	mRenderMeshToInstanceMap.clear();
// Add By Lixu End

	for (int i = 0; i < TT_Num; i++)
	{
		SAFE_RELEASE(m_TextureSRVs[i]);
	}	
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
	for (int i = 0; i < TT_Num; i++)
	{
		if (mTextureResources[i])
		{
			V(DirectX::CreateShaderResourceView(device,
				mTextureResources[i]->image.GetImages(),
				mTextureResources[i]->image.GetImageCount(),
				mTextureResources[i]->metaData, &m_TextureSRVs[i]));
		}
	}
}



RenderMaterial::InstancePtr RenderMaterial::getMaterialInstance(const IRenderMesh* mesh)
{
	// look in cache
	auto it = mRenderMeshToInstanceMap.find(mesh);
	if (it != mRenderMeshToInstanceMap.end())
	{
// Add By Lixu Begin
		/*
		if (!(*it).second.expired())
		{
			return (*it).second.lock();
		}
		*/
		return it->second;
// Add By Lixu End
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

	GPUShaderResource* pResource = Light::GetEnvTextureSRV();
	ID3D11ShaderResourceView* pEnvTextureSRV = D3D11TextureResource::GetResource(pResource);

	context.OMSetBlendState(mMaterial.mBlendState, nullptr, 0xFFFFFFFF);
	context.PSSetShaderResources(slot, TT_Num, mMaterial.m_TextureSRVs);
	context.PSSetShaderResources(TT_Num, 1, &pEnvTextureSRV);
	context.IASetInputLayout(mInputLayout);
}

bool RenderMaterial::Instance::isValid()
{
	return mMaterial.mShaderGroups.size() > 0 && mMaterial.mShaderGroups[mShaderNum]->IsValid();
}

// Add By Lixu Begin
void RenderMaterial::setTextureFileName(std::string textureFileName, TextureType tt)
{ 
	if (mTextureFileNames[tt] == textureFileName)
	{
		return;
	}
	mTextureFileNames[tt] = textureFileName;

	mTextureResources[tt] = nullptr;
	SAFE_RELEASE(m_TextureSRVs[tt]);

	if (mTextureFileNames[tt].empty())
	{
		return;
	}

	std::string searchDir = mTextureFileNames[tt];
	size_t ind = searchDir.find_last_of('/');
	if (ind > 0)
		searchDir = searchDir.substr(0, ind);

	ResourceManager* pResourceManager = ResourceManager::ins();
	pResourceManager->addSearchDir(searchDir.c_str());
	mTextureResources[tt] = pResourceManager->requestTexture(mTextureFileNames[tt].c_str());
	if (mTextureResources[tt] == nullptr)
	{
		return;
	}

	ID3D11Device* device = GetDeviceManager()->GetDevice();
	DirectX::CreateShaderResourceView(device, 
		mTextureResources[tt]->image.GetImages(), 
		mTextureResources[tt]->image.GetImageCount(),
		mTextureResources[tt]->metaData, &m_TextureSRVs[tt]);
}

void RenderMaterial::setDiffuseColor(float r, float g, float b, float a)
{
	mDiffuseColor[0] = r;
	mDiffuseColor[1] = g;
	mDiffuseColor[2] = b;
	mDiffuseColor[3] = a;
}

void RenderMaterial::getDiffuseColor(float& r, float& g, float& b, float& a)
{
	r = mDiffuseColor[0];
	g = mDiffuseColor[1];
	b = mDiffuseColor[2];
	a = mDiffuseColor[3];
}

void RenderMaterial::setSpecularColor(float r, float g, float b, float a)
{
	mSpecularColor[0] = r;
	mSpecularColor[1] = g;
	mSpecularColor[2] = b;
	mSpecularColor[3] = a;
}

void RenderMaterial::getSpecularColor(float& r, float& g, float& b, float& a)
{
	r = mSpecularColor[0];
	g = mSpecularColor[1];
	b = mSpecularColor[2];
	a = mSpecularColor[3];
}

RenderMaterial* g_DefaultRenderMaterial = nullptr;
RenderMaterial* RenderMaterial::getDefaultRenderMaterial()
{
	if (g_DefaultRenderMaterial == nullptr)
	{
		ResourceManager* pResourceManager = ResourceManager::ins();
		g_DefaultRenderMaterial = new RenderMaterial("", *pResourceManager, "model_simple_textured_ex", 0.5, 0.5, 0.5);
	}
	return g_DefaultRenderMaterial;
}

bool RenderMaterial::isBadTexture(TextureType tt)
{
	return (nullptr == m_TextureSRVs[tt]);
}
// Add By Lixu End