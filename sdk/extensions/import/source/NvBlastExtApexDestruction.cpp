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
// Copyright (c) 2016-2017 NVIDIA Corporation. All rights reserved.


#include "NvBlastExtApexDestruction.h"


#include "PxPhysicsAPI.h"
#include "Apex.h"
#include "NvBlastPxCallbacks.h"
#include <ModuleDestructible.h>
#include <DestructibleAsset.h>
#include "NullRenderer.h"
#include "PsString.h"

using namespace nvidia;
using namespace apex;

//////////////////////////////////////////////////////////////////////////////

NullRenderResourceManager gNullRenderer;

/////////////////////////////////////////////////////////////////////////////

namespace Nv
{
namespace Blast
{
using namespace ApexImporter;

ApexDestruction::ApexDestruction(NvBlastLog log)
{
	m_log = log;
	initialize();
}

ApexDestruction::ApexDestruction(ApexSDK* apexSdk, ModuleDestructible* moduleDestructible, NvBlastLog log)
{
	m_log = log;

	m_Foundation.reset(&apexSdk->getPhysXSDK()->getFoundation(), false);
	m_PhysxSDK.reset(apexSdk->getPhysXSDK(), false);
	m_Cooking.reset(apexSdk->getCookingInterface(), false);
	m_ApexSDK.reset(apexSdk, false);
	for (uint32_t i = 0; i < apexSdk->getNbModules(); ++i)
	{
		if (!physx::shdfnd::strcmp(apexSdk->getModules()[i]->getName(), "Legacy"))
		{
			hasLegacyModule = true;
		}
	}
	m_DestructibleModule.reset(moduleDestructible, false);
}


bool ApexDestruction::isValid()
{
	return m_PhysxSDK && m_Cooking && m_ApexSDK && m_DestructibleModule && hasLegacyModule;
}

bool ApexDestruction::initialize()
{
	if (isValid())
	{
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////

	m_Foundation.reset(PxCreateFoundation(PX_FOUNDATION_VERSION, NvBlastGetPxAllocatorCallback(), NvBlastGetPxErrorCallback()));
	if (!m_Foundation)
	{
		if (m_log)
		{
			m_log(NvBlastMessage::Error, "Error: failed to create Foundation\n", __FILE__, __LINE__);
		}
		return false;
	}
	physx::PxTolerancesScale scale;
	m_PhysxSDK.reset(PxCreatePhysics(PX_PHYSICS_VERSION, *m_Foundation, scale, true));
	if (!m_PhysxSDK)
	{
		if (m_log)
		{
			m_log(NvBlastMessage::Error, "Error: failed to create PhysX\n", __FILE__, __LINE__);
		}
		return false;
	}

#if 0
	if (!PxInitExtensions(*mPhysxSDK, 0))
	{
		if (m_log)
		{
			m_log(Error, "Error: failed to init PhysX extensions\n", __FILE__, __LINE__);
		}
		return false;
	}
#endif

	physx::PxCookingParams cookingParams(scale);
	cookingParams.buildGPUData = true;
	m_Cooking.reset(PxCreateCooking(PX_PHYSICS_VERSION, m_PhysxSDK->getFoundation(), cookingParams));
	if (!m_Cooking)
	{
		m_log(NvBlastMessage::Error, "Error: failed to create PhysX Cooking\n", __FILE__, __LINE__);
		return false;
	}

	//////////////////////////////////////////////////////////////////////////////

	ApexSDKDesc apexSDKDesc;
	apexSDKDesc.physXSDK = m_PhysxSDK.get();
	apexSDKDesc.cooking = m_Cooking.get();
	apexSDKDesc.renderResourceManager = &gNullRenderer;
	apexSDKDesc.resourceCallback = nullptr;
	apexSDKDesc.foundation = &m_PhysxSDK->getFoundation();

	m_ApexSDK.reset(CreateApexSDK(apexSDKDesc, nullptr, APEX_SDK_VERSION));
	if (!m_ApexSDK)
	{
		if (m_log)
		{
			m_log(NvBlastMessage::Error, "Error: failed to create APEX\n", __FILE__, __LINE__);
		}
		return false;
	}

	//////////////////////////////////////////////////////////////////////////////

	m_DestructibleModule.reset(static_cast<nvidia::apex::ModuleDestructible*>(m_ApexSDK->createModule("Destructible")), ApexReleaser(*m_ApexSDK));
	if (!m_DestructibleModule)
	{
		if (m_log)
		{
			m_log(NvBlastMessage::Error, "Error: failed to create ModuleDestructible\n", __FILE__, __LINE__);
		}
		return false;
	}

	if (!m_ApexSDK->createModule("Legacy"))
	{
		if (m_log)
		{
			m_log(NvBlastMessage::Error, "Error: failed to create Legacy module\n", __FILE__, __LINE__);
		}
		return false;
	};


	float massScaleExponenent = 1.f;
	float massScale = 1.f;

	NvParameterized::Interface* params = m_DestructibleModule->getDefaultModuleDesc();
	NvParameterized::Handle paramsHandle(params);
	paramsHandle.getParameter("scaledMassExponent");
	paramsHandle.setParamF32(massScaleExponenent);
	paramsHandle.getParameter("massScale");
	paramsHandle.setParamF32(massScale);
	m_DestructibleModule->init(*params);

	return true;
}

DestructibleAsset* ApexDestruction::loadAsset(physx::PxFileBuf* stream)
{
	DestructibleAsset* asset = nullptr;

	if (stream && stream->isOpen())
	{
		NvParameterized::Serializer::SerializeType serType = apexSDK()->getSerializeType(*stream);
		NvParameterized::Serializer::ErrorType serError;
		NvParameterized::Serializer* ser = apexSDK()->createSerializer(serType);
		PX_ASSERT(ser);

		NvParameterized::Serializer::DeserializedData data;
		serError = ser->deserialize(*stream, data);

		if (serError == NvParameterized::Serializer::ERROR_NONE && data.size() == 1)
		{
			NvParameterized::Interface* params = data[0];
			if (!physx::shdfnd::strcmp(params->className(), "DestructibleAssetParameters"))
			{
				asset = static_cast<DestructibleAsset*>(apexSDK()->createAsset(params, ""));
			}
			else
			{
				m_log(NvBlastMessage::Error, "Error: deserialized data is not an APEX Destructible\n", __FILE__, __LINE__);
			}
		}
		else
		{
			m_log(NvBlastMessage::Error, "Error: failed to deserialize\n", __FILE__, __LINE__);
		}
		ser->release();
	}

	if (!asset)
	{
		char message[255] = { 0 };
		sprintf(message, "Error: failed to load asset...\n");
		m_log(NvBlastMessage::Error, message, __FILE__, __LINE__);
	}

	return asset;
}

ApexDestruction::~ApexDestruction()
{
}

} // namespace Blast
} // namespace Nv
