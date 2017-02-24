/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef BLAST_ASSET_H
#define BLAST_ASSET_H

#include <memory>
#include "PxTransform.h"
#include "NvBlastTypes.h"


using namespace physx;

class Renderer;
class BlastFamily;
class PhysXController;

namespace Nv
{
namespace Blast
{
class ExtPxFamily;
class ExtPxAsset;
class ExtPxManager;
class TkGroup;
}
}

using namespace Nv::Blast;

typedef std::shared_ptr<BlastFamily> BlastFamilyPtr;


class BlastAsset
{
public:
	//////// ctor ////////

	BlastAsset(Renderer& renderer);
	virtual ~BlastAsset() {}


	//////// desc ////////

	/**
	Descriptor with actor initial settings.
	*/
	struct ActorDesc
	{
		NvBlastID			id;
		PxTransform			transform;
		TkGroup*			group;
	};


	//////// abstract ////////

	virtual BlastFamilyPtr createFamily(PhysXController& physXConroller, ExtPxManager& pxManager, const ActorDesc& desc) = 0;


	//////// data getters  ////////

	const ExtPxAsset* getPxAsset() const
	{ 
		return m_pxAsset;
	}

	size_t getBlastAssetSize() const;


protected:
	//////// internal operations ////////

	void validate();


	//////// input data ////////

	Renderer&			m_renderer;


	//////// internal data ////////

	ExtPxAsset*			m_pxAsset;
};



#endif //BLAST_ASSET_H