/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef BLAST_ASSET_BOXES_H
#define BLAST_ASSET_BOXES_H

#include "BlastAsset.h"
#include "AssetGenerator.h"
#include "PxConvexMesh.h"


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


class BlastAssetBoxes : public BlastAsset
{
public:
	struct Desc
	{
		CubeAssetGenerator::Settings generatorSettings;
		float staticHeight;
		bool jointAllBonds;
	};

	BlastAssetBoxes(TkFramework& framework, PxPhysics& physics, PxCooking& cooking, Renderer& renderer, const Desc& desc);
	virtual ~BlastAssetBoxes();

	BlastFamilyPtr createFamily(PhysXController& physXConroller, ExtPxManager& pxManager, const ActorDesc& desc);

private:
	PxConvexMesh*	m_boxMesh;
	GeneratorAsset	m_generatorAsset;
};



#endif //BLAST_ASSET_BOXES_H