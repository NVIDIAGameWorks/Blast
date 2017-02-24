/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTEXTPXACTORIMPL_H
#define NVBLASTEXTPXACTORIMPL_H

#include "NvBlastExtPxActor.h"
#include "NvBlastExtArray.h"
#include "PxTransform.h"


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
	PxVec3		m_linearVelocity;
	PxVec3		m_angularVelocity;
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
	ExtInlineArray<uint32_t, 4>::type	m_chunkIndices;
};



} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTPXACTORIMPL_H
