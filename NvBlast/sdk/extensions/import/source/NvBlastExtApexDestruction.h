/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTEXTAPEXDESTRUCTION_H
#define NVBLASTEXTAPEXDESTRUCTION_H

#include "ApexUsingNamespace.h"
#include "NvBlastExtScopedResource.h"
#include "PsUtilities.h"
#include <string>
#include <NvBlastTypes.h>
#include <PxFileBuf.h>

namespace physx
{
	class PxFoundation;
}


namespace Nv
{
namespace Blast
{

namespace ApexImporter
{
	/**
		Class for working with APEX Destruction assets.
	*/
class ApexDestruction
{
	PX_NOCOPY(ApexDestruction)

public:
	ApexDestruction(NvBlastLog log = NULL);

	ApexDestruction(nvidia::apex::ApexSDK* apexSdk, nvidia::apex::ModuleDestructible* moduleDestructible, NvBlastLog log = NULL);
	~ApexDestruction();

	//////////////////////////////////////////////////////////////////////////////
	/**
		ApexDestruction initialization. If APEX SDK and ModuleDestructible was provided to constructor, they will be used. 
		Otherwise, PhysXSDK and APEX SDK will be initialized.
	*/
	bool initialize();

	/**
		/return Return True if tool initialized sucessfully.
	*/
	bool isValid();

	/**
		Load Apex Destructible asset from PxFileBuf stream
		\param[in] stream Apex asset stream
		/return Return DestructibleAsset* if success, otherwise nullptr is returned.
	*/
	nvidia::apex::DestructibleAsset*	loadAsset(physx::PxFileBuf* stream);

	/**
		/return Return PxFoundation.
	*/
	physx::PxFoundation*				foundation() { return m_Foundation.get(); }
	/**
		/return Return PxPhysics.
	*/
	physx::PxPhysics*					physxSDK() { return m_PhysxSDK.get(); }
	/**
		/return Return PxCooking.
	*/
	physx::PxCooking*					cooking() { return m_Cooking.get(); }
	/**
		/return Return ApexSDK.
	*/
	nvidia::apex::ApexSDK*				apexSDK() { return m_ApexSDK.get(); }

	/**
		/return Return ModuleDestructible.
	*/
	nvidia::apex::ModuleDestructible*	destructibleModule() { return m_DestructibleModule.get(); }

private:
	bool hasLegacyModule;
	NvBlastLog														m_log;
	//////////////////////////////////////////////////////////////////////////////

protected:
	ScopedResource<physx::PxFoundation>								m_Foundation;
	ScopedResource<physx::PxPhysics>								m_PhysxSDK;
	ScopedResource<physx::PxCooking>								m_Cooking;
	ScopedResource<nvidia::apex::ApexSDK>							m_ApexSDK;
	ScopedResource<nvidia::apex::ModuleDestructible, ApexReleaser>	m_DestructibleModule;

};

} // namespace ApexImporter

} // namespace Blast
} // namespace Nv


#endif // NVBLASTEXTAPEXDESTRUCTION_H
