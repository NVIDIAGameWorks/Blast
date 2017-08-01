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


#ifndef BLAST_ASSET_H
#define BLAST_ASSET_H

#include <memory>
#include <vector>
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

// Add By Lixu Begin
typedef BlastFamily* BlastFamilyPtr;
// Add By Lixu End


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

	std::vector<uint32_t> getChunkIndexesByDepth(uint32_t depth) const;
	
	float getBondHealthMax() const
	{
		return m_bondHealthMax;
	}

	float getSupportChunkHealthMax() const
	{
		return m_bondHealthMax;
	}

	void initialize();

protected:
	//////// internal operations ////////



	//////// input data ////////

	Renderer&			m_renderer;


	//////// internal data ////////

	ExtPxAsset*			m_pxAsset;
	float				m_bondHealthMax;
	float				m_supportChunkHealthMax;
};



#endif //BLAST_ASSET_H