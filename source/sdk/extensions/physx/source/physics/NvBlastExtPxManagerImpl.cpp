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


#include "NvBlastExtPxManagerImpl.h"
#include "NvBlastExtPxAssetImpl.h"
#include "NvBlastExtPxActorImpl.h"
#include "NvBlastExtPxCollisionBuilderImpl.h"
#include "NvBlastExtPxFamilyImpl.h"

#include "NvBlastAssert.h"

#include "NvBlastTkActor.h"
#include "NvBlastTkFamily.h"
#include "NvBlastTkGroup.h"
#include "NvBlastTkJoint.h"

#include "PxPhysics.h"
#include "PxRigidDynamic.h"
#include "PxJoint.h"


namespace Nv
{
namespace Blast
{


ExtPxManager* ExtPxManager::create(PxPhysics& physics, TkFramework& framework, ExtPxCreateJointFunction createFn, bool useUserData)
{
	return NVBLAST_NEW(ExtPxManagerImpl)(physics, framework, createFn, useUserData);
}

ExtPxCollisionBuilder* ExtPxManager::createCollisionBuilder(PxPhysics& physics, PxCooking& cooking)
{
	return NVBLAST_NEW(ExtPxCollisionBuilderImpl(&cooking, &physics.getPhysicsInsertionCallback()));
}

void ExtPxManagerImpl::release()
{
	NVBLAST_DELETE(this, ExtPxManagerImpl);
}

ExtPxFamily* ExtPxManagerImpl::createFamily(const ExtPxFamilyDesc& desc)
{
	NVBLAST_CHECK_ERROR(desc.pxAsset != nullptr, "Family creation: pxAsset is nullptr.", return nullptr);

	// prepare TkActorDesc (take NvBlastActorDesc from ExtPxFamilyDesc if it's not null, otherwise take from PxAsset)
	TkActorDesc tkActorDesc;
	const NvBlastActorDesc& actorDesc = desc.actorDesc ? *desc.actorDesc : desc.pxAsset->getDefaultActorDesc();
	(&tkActorDesc)->NvBlastActorDesc::operator=(actorDesc);
	tkActorDesc.asset = &desc.pxAsset->getTkAsset();

	// create tk actor
	TkActor* actor = m_framework.createActor(tkActorDesc);
	NVBLAST_CHECK_ERROR(actor != nullptr, "Family creation: tk actor creation failed.", return nullptr);

	// create px family
	ExtPxFamilyImpl* family = NVBLAST_NEW(ExtPxFamilyImpl)(*this, actor->getFamily(), *desc.pxAsset);

	if (desc.group)
	{
		desc.group->addActor(*actor);
	}

	return family;
}

bool ExtPxManagerImpl::createJoint(TkJoint& joint)
{
	if (!joint.userData && m_createJointFn)
	{
		const TkJointData data = joint.getData();
		ExtPxActorImpl* pxActor0 = data.actors[0] != nullptr ? reinterpret_cast<ExtPxActorImpl*>(data.actors[0]->userData) : nullptr;
		ExtPxActorImpl* pxActor1 = data.actors[1] != nullptr ? reinterpret_cast<ExtPxActorImpl*>(data.actors[1]->userData) : nullptr;
		if (!pxActor0 && !pxActor1)
		{
			for (int i = 0; i < 2; ++i)
			{
				if (data.actors[i] != nullptr)
				{
					m_incompleteJointMultiMap[data.actors[i]].pushBack(&joint);
				}
			}
			return false;
		}
		PxTransform lf0(data.attachPositions[0]);
		PxTransform lf1(data.attachPositions[1]);
		PxJoint* pxJoint = m_createJointFn(pxActor0,  lf0, pxActor1, lf1, m_physics, joint);
		if (pxJoint)
		{
			joint.userData = pxJoint;
			return true;
		}
	}
	return false;
}

void ExtPxManagerImpl::updateJoint(TkJoint& joint)
{
	const TkJointData& data = joint.getData();
	if (joint.userData)
	{
		ExtPxActorImpl* pxActors[2];
		for (int i = 0; i < 2; ++i)
		{
			if (data.actors[i] != nullptr)
			{
				pxActors[i] = reinterpret_cast<ExtPxActorImpl*>(data.actors[i]->userData);
				if (pxActors[i] == nullptr)
				{
					Array<TkJoint*>::type& joints = m_incompleteJointMultiMap[data.actors[i]];
					NVBLAST_ASSERT(joints.find(&joint) == joints.end());
					joints.pushBack(&joint);
					return;	// Wait until the TkActor is received to create this joint
				}
			}
			else
			{
				pxActors[i] = nullptr;
			}
		}
		NVBLAST_ASSERT(pxActors[0] || pxActors[1]);
		PxJoint* pxJoint = reinterpret_cast<PxJoint*>(joint.userData);
		pxJoint->setActors(pxActors[0] ? &pxActors[0]->getPhysXActor() : nullptr, pxActors[1] ? &pxActors[1]->getPhysXActor() : nullptr);
	}
	else
	{
		ExtPxActorImpl* pxActor0 = data.actors[0] != nullptr ? reinterpret_cast<ExtPxActorImpl*>(data.actors[0]->userData) : nullptr;
		ExtPxActorImpl* pxActor1 = data.actors[1] != nullptr ? reinterpret_cast<ExtPxActorImpl*>(data.actors[1]->userData) : nullptr;
		PxTransform lf0(data.attachPositions[0]);
		PxTransform lf1(data.attachPositions[1]);
		PxJoint* pxJoint = m_createJointFn(pxActor0, lf0, pxActor1, lf1, m_physics, joint);
		if (pxJoint)
		{
			joint.userData = pxJoint;
		}
	}
}

void ExtPxManagerImpl::destroyJoint(TkJoint& joint)
{
	if (joint.userData)
	{
		PxJoint* pxJoint = reinterpret_cast<PxJoint*>(joint.userData);
		pxJoint->release();
		joint.userData = nullptr;
	}
}



} // namespace Blast
} // namespace Nv
