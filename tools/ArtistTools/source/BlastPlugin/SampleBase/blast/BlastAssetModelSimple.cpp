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


#include "BlastAssetModelSimple.h"
#include "BlastFamilyModelSimple.h"
#include "CustomRenderMesh.h"
#include "Renderer.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												BlastAssetModelSimple
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BlastAssetModelSimple::BlastAssetModelSimple(TkFramework& framework, PxPhysics& physics, PxCooking& cooking, ExtSerialization& serialization, Renderer& renderer, const char* modelName)
	: BlastAssetModel(framework, physics, cooking, serialization, renderer, modelName)
{
	_init(renderer);
}

BlastAssetModelSimple::BlastAssetModelSimple(ExtPxAsset* pExtPxAsset, BlastModel* pBlastModel, Renderer& renderer)
	: BlastAssetModel(pExtPxAsset, pBlastModel, renderer)
{
	_init(renderer);
}

BlastAssetModelSimple::~BlastAssetModelSimple()
{
	m_renderMaterialNames.clear();
}

void BlastAssetModelSimple::_init(Renderer& renderer)
{
	for (const BlastModel::Material& material : getModel().materials)
	{
		std::string name = material.name;
		if (name != "" && !BlastProject::ins().isGraphicsMaterialNameExist(name.c_str()))
		{
			BlastProject::ins().addGraphicsMaterial(name.c_str());
			BlastProject::ins().reloadDiffuseTexture(name.c_str(), material.diffuseTexture.c_str());
			BlastProject::ins().reloadDiffuseColor(name.c_str(), material.r, material.g, material.b, material.a);
		}
		m_renderMaterialNames.push_back(name);
	}
}

BlastFamilyPtr BlastAssetModelSimple::createFamily(PhysXController& physXConroller, ExtPxManager& pxManager, const ActorDesc& desc)
{
	return BlastFamilyPtr(new BlastFamilyModelSimple(physXConroller, pxManager, m_renderer, *this, desc));
}
