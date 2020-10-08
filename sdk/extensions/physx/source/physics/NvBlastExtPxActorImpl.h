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
// Copyright (c) 2016-2020 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTEXTPXACTORIMPL_H
#define NVBLASTEXTPXACTORIMPL_H

#include "NvBlastExtPxActor.h"
#include "NvBlastArray.h"
#include "foundation/PxTransform.h"


using namespace physx;

namespace Nv
{
namespace Blast
{


// Forward declarations
class ExtPxFamilyImpl;

struct PxActorCreateInfo
{
	PxTransform	m_transform;
	PxVec3		m_scale;
	PxVec3		m_parentLinearVelocity;
	PxVec3		m_parentAngularVelocity;
	PxVec3		m_parentCOM;
};


class ExtPxActorImpl final : public ExtPxActor
{
public:
	//////// ctor ////////

	ExtPxActorImpl(ExtPxFamilyImpl* family, TkActor* tkActor, const PxActorCreateInfo& pxActorInfo);

	~ExtPxActorImpl()
	{
		release();
	}

	void release();


	//////// interface ////////

	virtual uint32_t					getChunkCount() const override
	{
		return static_cast<uint32_t>(m_chunkIndices.size());
	}

	virtual const uint32_t*				getChunkIndices() const override
	{
		return m_chunkIndices.begin();
	}

	virtual PxRigidDynamic&				getPhysXActor() const override
	{
		return *m_rigidDynamic;
	}

	virtual TkActor&					getTkActor() const override
	{
		return *m_tkActor;
	}

	virtual ExtPxFamily&				getFamily() const override;


private:
	//////// data ////////

	ExtPxFamilyImpl*					m_family;
	TkActor*							m_tkActor;
	PxRigidDynamic*						m_rigidDynamic;
	InlineArray<uint32_t, 4>::type		m_chunkIndices;
};



} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTPXACTORIMPL_H
