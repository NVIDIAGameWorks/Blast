/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/


#include "BlastAssetModelSimple.h"
#include "BlastFamilyModelSimple.h"
#include "CustomRenderMesh.h"
#include "Renderer.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												BlastAssetModelSimple
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BlastAssetModelSimple::BlastAssetModelSimple(TkFramework& framework, PxPhysics& physics, PxCooking& cooking, Renderer& renderer, const char* modelName)
	: BlastAssetModel(framework, physics, cooking, renderer, modelName)
{
	// prepare materials
// Add By Lixu Begin
	for (const BlastModel::Material& material : getModel().materials)
	{
		if (material.diffuseTexture.empty())
			m_renderMaterials.push_back(RenderMaterial::getDefaultRenderMaterial());
		else
			m_renderMaterials.push_back(new RenderMaterial(material.name.c_str(), renderer.getResourceManager(), "model_simple_textured_ex", material.diffuseTexture.c_str()));
	}
// Add By Lixu End
	validate();
}


BlastAssetModelSimple::~BlastAssetModelSimple()
{
	// release materials
	for (RenderMaterial* r : m_renderMaterials)
	{
		if (r == RenderMaterial::getDefaultRenderMaterial())
		{
			continue;
		}
		SAFE_DELETE(r);
	}
}


BlastFamilyPtr BlastAssetModelSimple::createFamily(PhysXController& physXConroller, ExtPxManager& pxManager, const ActorDesc& desc)
{
	return BlastFamilyPtr(new BlastFamilyModelSimple(physXConroller, pxManager, m_renderer, *this, desc));
}
