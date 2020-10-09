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
// Copyright (c) 2008-2020 NVIDIA Corporation. All rights reserved.


#include "BlastAsset.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlastTkAsset.h"
#include "NvBlastExtDamageShaders.h"
#include <algorithm>


BlastAsset::BlastAsset(Renderer& renderer)
	: m_renderer(renderer), m_bondHealthMax(1.0f), m_supportChunkHealthMax(1.0f), m_damageAccelerator(nullptr)
{
}

BlastAsset::~BlastAsset()
{
	if (m_damageAccelerator)
	{
		m_damageAccelerator->release();
	}
}

void BlastAsset::initialize()
{
	// calc max healths
	const auto& actorDesc = m_pxAsset->getDefaultActorDesc();
	if (actorDesc.initialBondHealths)
	{
		m_bondHealthMax = FLT_MIN;
		const uint32_t bondCount = m_pxAsset->getTkAsset().getBondCount();
		for (uint32_t i = 0; i < bondCount; ++i)
		{
			m_bondHealthMax = std::max<float>(m_bondHealthMax, actorDesc.initialBondHealths[i]);
		}
	}
	else
	{
		m_bondHealthMax = actorDesc.uniformInitialBondHealth;
	}

	if(actorDesc.initialSupportChunkHealths)
	{
		m_supportChunkHealthMax = FLT_MIN;
		const uint32_t nodeCount = m_pxAsset->getTkAsset().getGraph().nodeCount;
		for (uint32_t i = 0; i < nodeCount; ++i)
		{
			m_supportChunkHealthMax = std::max<float>(m_supportChunkHealthMax, actorDesc.initialSupportChunkHealths[i]);
		}
	}
	else
	{
		m_supportChunkHealthMax = actorDesc.uniformInitialLowerSupportChunkHealth;
	}

	m_damageAccelerator = NvBlastExtDamageAcceleratorCreate(m_pxAsset->getTkAsset().getAssetLL(), 3);
}

size_t BlastAsset::getBlastAssetSize() const
{
	return m_pxAsset->getTkAsset().getDataSize();
}
