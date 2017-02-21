/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef BLAST_ASSET_MODEL_H
#define BLAST_ASSET_MODEL_H

#include "BlastAsset.h"
#include "BlastModel.h"


namespace physx
{
class PxPhysics;
class PxCooking;
}

namespace Nv
{
namespace Blast
{
class TkFramework;
}
}


class BlastAssetModel : public BlastAsset
{
public:
	//////// ctor ////////

	BlastAssetModel(TkFramework& framework, PxPhysics& physics, PxCooking& cooking, Renderer& renderer, const char* modelName);
	virtual ~BlastAssetModel();


	//////// data getters  ////////

	const BlastModel& getModel() const
	{
// Add By Lixu Begin
		return *m_model;
// Add By Lixu End
	}

private:
	//////// private internal data ////////

	BlastModelPtr	m_model;
};

#endif //BLAST_ASSET_MODEL_H