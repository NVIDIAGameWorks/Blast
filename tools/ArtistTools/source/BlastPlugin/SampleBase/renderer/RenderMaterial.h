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


#ifndef RENDER_MATERIAL_H
#define RENDER_MATERIAL_H

#include "Utils.h"
#include "DirectXTex.h"

#include <string>
#include <vector>
#include <list>
#include <map>
#include <memory>


class IRenderMesh;
class ResourceManager;
struct TextureResource;
class Renderable;


class RenderMaterial
{
  public:

	enum BlendMode
	{
		BLEND_NONE,
		BLEND_ALPHA_BLENDING,
		BLEND_ADDITIVE
	};

	enum TextureType
	{
		TT_Diffuse,
		TT_Specular,
		TT_Normal,
		TT_Num
	};

	RenderMaterial(const char* materialName, ResourceManager& resourceProvider, const char* shaderFileName, const char* textureFileName = "", BlendMode blendMode = BLEND_NONE);
	RenderMaterial(const char* materialName, ResourceManager& resourceProvider, const char* shaderFileName, float r, float g, float b, float a = 1.0, BlendMode blendMode = BLEND_NONE);
	~RenderMaterial();

	void setBlending(BlendMode blendMode);
	BlendMode getBlending() const { return mBlendMode; }

	void reload();

	class Instance
	{
	public:
		Instance(RenderMaterial& material, ID3D11InputLayout* inputLayout, uint32_t shaderNum = 0) : mMaterial(material), mInputLayout(inputLayout), mShaderNum(shaderNum) {}
		~Instance() { SAFE_RELEASE(mInputLayout); }

		bool isValid();
		void bind(ID3D11DeviceContext& context, uint32_t slot, bool depthStencilOnly = false);
		RenderMaterial& getMaterial() const { return mMaterial; }
	private:
		RenderMaterial& mMaterial;
		ID3D11InputLayout* mInputLayout;
		uint32_t mShaderNum;
	};

// Add By Lixu Begin
	typedef Instance* InstancePtr;
	std::string getMaterialName(){ return mMaterialName; }
	void setMaterialName(std::string materialName){ mMaterialName = materialName; }
	std::string getTextureFileName(TextureType tt = TT_Diffuse){ return mTextureFileNames[tt]; }
	void setTextureFileName(std::string textureFileName, TextureType tt = TT_Diffuse);
	void getDiffuseColor(float& r, float& g, float& b, float& a);
	void setDiffuseColor(float r, float g, float b, float a = 1.0);
	void getSpecularColor(float& r, float& g, float& b, float& a);
	void setSpecularColor(float r, float g, float b, float a = 1.0);
	float getSpecularShininess() { return mSpecularShininess; }
	void setSpecularShininess(float specularShininess) { mSpecularShininess = specularShininess; }
	void addRelatedRenderable(Renderable* pRenderable){ mRelatedRenderables.push_back(pRenderable); }
	std::vector<Renderable*>& getRelatedRenderables(){ return mRelatedRenderables; }
	void clearRelatedRenderables(){ mRelatedRenderables.clear(); }
	bool isBadTexture(TextureType tt = TT_Diffuse);
	static RenderMaterial* getDefaultRenderMaterial();
// Add By Lixu End

	InstancePtr getMaterialInstance(const IRenderMesh* mesh);
	InstancePtr getMaterialInstance(const D3D11_INPUT_ELEMENT_DESC* elementDescs, uint32_t numElements);

  private:
	void initialize(ResourceManager& resourceCallback, const char* shaderFileName, const char* textureFileName, BlendMode blendMode);
	void initialize(ResourceManager&resourceProvider, std::vector<std::string> shaderFileNames, const char* textureFileName, BlendMode blendMode);

	void releaseReloadableResources();

	std::string mMaterialName;
	std::string mShaderFileName;
//	std::string mTextureFileName;

	struct ShaderGroup
	{
		ShaderGroup() : vs(nullptr), gs(nullptr), ps(nullptr), buffer(nullptr)
		{
		}
		~ShaderGroup()
		{
			Release();
		}
		void Release()
		{
			SAFE_RELEASE(vs);
			SAFE_RELEASE(gs);
			SAFE_RELEASE(ps);
			SAFE_RELEASE(buffer);
		}
		void Set(ID3D11DeviceContext* c, bool setPixelShader = true)
		{
			c->VSSetShader(vs, nullptr, 0);
			c->GSSetShader(gs, nullptr, 0);
			c->PSSetShader(setPixelShader  ? ps : nullptr, nullptr, 0);
		}
		bool IsValid()
		{
			return vs != nullptr;
		}
		ID3D11VertexShader* vs;
		ID3D11GeometryShader* gs;
		ID3D11PixelShader* ps;
		ID3DBlob* buffer;
	};

// Add By Lixu Begin
	std::map<const IRenderMesh*, Instance*> mRenderMeshToInstanceMap;
	std::vector<Renderable*> mRelatedRenderables;
//	float mr;
//	float mg;
//	float mb;
//	float ma;
// Add By Lixu End
//	const TextureResource*					mTexture;
//	ID3D11ShaderResourceView*               mTextureSRV;
	std::vector<std::string>                mShaderFilePathes;
	std::vector<ShaderGroup*>               mShaderGroups;
	ID3D11BlendState*                       mBlendState;
	BlendMode                               mBlendMode;

	std::string mTextureFileNames[TT_Num];
	const TextureResource* mTextureResources[TT_Num];
	ID3D11ShaderResourceView* m_TextureSRVs[TT_Num];
	float mDiffuseColor[4];
	float mSpecularColor[4];
	float mSpecularShininess;
};

#endif