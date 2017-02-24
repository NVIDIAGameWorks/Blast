/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastExtPxManagerImpl.h"
#include "NvBlastExtPxAssetImpl.h"
#include "NvBlastExtPxActorImpl.h"
#include "NvBlastExtPxFamilyImpl.h"

#include "NvBlastAssert.h"

#include "NvBlastTkActor.h"
#include "NvBlastTkFamily.h"
#include "NvBlastTkGroup.h"
#include "NvBlastTkJoint.h"

#include "PxRigidDynamic.h"
#include "PxJoint.h"


namespace Nv
{
namespace Blast
{


ExtPxManager* ExtPxManager::create(PxPhysics& physics, TkFramework& framework, ExtPxCreateJointFunction createFn, bool useUserData)
{
	return NVBLASTEXT_NEW(ExtPxManagerImpl)(physics, framework, createFn, useUserData);
}

void ExtPxManagerImpl::release()
{
	NVBLASTEXT_DELETE(this, ExtPxManagerImpl);
}

ExtPxFamily* ExtPxManagerImpl::createFamily(const ExtPxFamilyDesc& desc)
{
	NVBLASTEXT_CHECK_ERROR(desc.pxAsset != nullptr, "Family creation: pxAsset is nullptr.", return nullptr);

	// create tk family
	TkActorDesc tkActorDesc;
	(&tkActorDesc)->NvBlastActorDesc::operator=(desc.actorDesc);
	tkActorDesc.asset = &desc.pxAsset->getTkAsset();
	TkActor* actor = m_framework.createActor(tkActorDesc);
	NVBLASTEXT_CHECK_ERROR(actor != nullptr, "Family creation: tk actor creation failed.", return nullptr);

	ExtPxFamilyImpl* family = NVBLASTEXT_NEW(ExtPxFamilyImpl)(*this, actor->getFamily(), *desc.pxAsset);

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
		NVBLAST_ASSERT(pxActor0 || pxActor1);
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
	if (joint.userData)
	{
		const TkJointData& data = joint.getData();
		ExtPxActorImpl* pxActors[2];
		for (int i = 0; i < 2; ++i)
		{
			if (data.actors[i] != nullptr)
			{
				pxActors[i] = reinterpret_cast<ExtPxActorImpl*>(data.actors[i]->userData);
				if (pxActors[i] == nullptr)
				{
					ExtArray<TkJoint*>::type& joints = m_incompleteJointMultiMap[data.actors[i]];
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
