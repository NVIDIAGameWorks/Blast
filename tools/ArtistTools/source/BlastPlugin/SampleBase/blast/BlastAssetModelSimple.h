/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef BLAST_ASSET_MODEL_SIMPLE_H
#define BLAST_ASSET_MODEL_SIMPLE_H

#include "BlastAssetModel.h"


class RenderMaterial;

class BlastAssetModelSimple : public BlastAssetModel
{
public:
	//////// ctor ////////

	BlastAssetModelSimple(TkFramework& framework, PxPhysics& physics, PxCooking& cooking, Renderer& renderer, const char* modelName);
	virtual ~BlastAssetModelSimple();


	//////// interface implementation ////////

	virtual BlastFamilyPtr createFamily(PhysXController& physXConroller, ExtPxManager& pxManager, const ActorDesc& desc);


	//////// data getters  ////////

	const std::vector<RenderMaterial*>& getRenderMaterials() const
	{
		return m_renderMaterials;
	}


private:
	//////// private internal data ////////

	std::vector<RenderMaterial*>	m_renderMaterials;
};

#endif //BLAST_ASSET_MODEL_SIMPLE_H