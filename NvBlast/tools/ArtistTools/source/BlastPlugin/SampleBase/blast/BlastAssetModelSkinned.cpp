/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "BlastAssetModelSkinned.h"
#include "BlastFamilyModelSkinned.h"
#include "RenderMaterial.h"
#include "Renderer.h"


BlastAssetModelSkinned::BlastAssetModelSkinned(TkFramework& framework, PxPhysics& physics, PxCooking& cooking, Renderer& renderer, const char* modelName)
	: BlastAssetModel(framework, physics, cooking, renderer, modelName)
{
// Add By Lixu Begin
	int index = 0;
	char materialName[50];
	for (const BlastModel::Material& material : getModel().materials)
	{
		sprintf(materialName, "%s_Material%d", modelName, index++);

		if (material.diffuseTexture.empty())
			m_renderMaterials.push_back(new RenderMaterial(modelName, renderer.getResourceManager(), "model_skinned"));
		else 
			m_renderMaterials.push_back(new RenderMaterial(modelName, renderer.getResourceManager(), "model_skinned_textured", material.diffuseTexture.c_str()));
	}
// Add By Lixu End
	validate();
}

BlastAssetModelSkinned::~BlastAssetModelSkinned()
{
	for (RenderMaterial* r : m_renderMaterials)
	{
		SAFE_DELETE(r);
	}
}

BlastFamilyPtr BlastAssetModelSkinned::createFamily(PhysXController& physXConroller, ExtPxManager& pxManager, const ActorDesc& desc)
{
	return BlastFamilyPtr(new BlastFamilyModelSkinned(physXConroller, pxManager, m_renderer, *this, desc));
}
