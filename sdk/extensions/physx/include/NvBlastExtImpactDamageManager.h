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


#ifndef NVBLASTEXTIMPACTDAMAGEMANAGER_H
#define NVBLASTEXTIMPACTDAMAGEMANAGER_H

#include "PxFiltering.h"
#include "NvPreprocessor.h"

// Forward declarations
namespace physx
{
struct PxContactPair;
struct PxContactPairHeader;
}


namespace Nv
{
namespace Blast
{

// Forward declarations
class ExtPxActor;
class ExtPxManager;


/**
Custom Damage Function
*/
typedef bool(*ExtImpactDamageFunction)(void* data, ExtPxActor* actor, physx::PxShape* shape, physx::PxVec3 worldPos, physx::PxVec3 worldForce);


/**
Impact Damage Manager Settings.
*/
struct ExtImpactSettings
{
	bool					isSelfCollissionEnabled;	//!<	family's self collision enabled.
	bool					shearDamage;				//!<	use shear damage program (otherwise simple radial damage is used)
	float					impulseMinThreshold;		//!<	min impulse value to apply impact damage.
	float					impulseMaxThreshold;		//!<	max impulse value, damage is interpolated value between min and max impulses.
	float					damageMax;					//!<	max damage to be applied (if impulse is >= impulseMaxThreshold).
	float					damageRadiusMax;			//!<	max penetration depth (if impulse is >= impulseMaxThreshold).
	float					damageAttenuation;			//!<	penetration attenuation ([0..1], where 1 means damage attenuates linearly from 0 to max penetration depth).
	ExtImpactDamageFunction damageFunction;				//!<	custom damage function, can be nullptr, default internal one will be used in that case.
	void*					damageFunctionData;			//!<	data to be passed in custom damage function.


	ExtImpactSettings() :
		isSelfCollissionEnabled(false),
		shearDamage(true),
		impulseMinThreshold(0.0f),
		impulseMaxThreshold(1000000.0f),
		damageMax(100.f),
		damageRadiusMax(5.0f),
		damageAttenuation(1.f),
		damageFunction(nullptr),
		damageFunctionData(nullptr)
	{}
};


/**
Impact Damage Manager.

Requires ExtPxManager.
Call onContact from PxSimulationEventCallback onContact to accumulate damage.
Call applyDamage to apply accumulated damage.
*/
class NV_DLL_EXPORT ExtImpactDamageManager
{
public:
	//////// manager creation ////////

	/**
	Create a new ExtImpactDamageManager.

	\param[in]	pxManager	The ExtPxManager instance to be used by impact damage manager.
	\param[in]	settings		The settings to be set on ExtImpactDamageManager.

	\return the new ExtImpactDamageManager if successful, NULL otherwise.
	*/
	static ExtImpactDamageManager*	create(ExtPxManager* pxManager, ExtImpactSettings settings = ExtImpactSettings());

	/**
	Release this manager.
	*/
	virtual void					release() = 0;


	//////// interface ////////

	/**
	Set ExtImpactDamageManager settings.

	\param[in]	settings		The settings to be set on ExtImpactDamageManager.
	*/
	virtual void					setSettings(const ExtImpactSettings& settings) = 0;

	/**
	This method is equal to PxSimulationEventCallback::onContact.

	User should implement own PxSimulationEventCallback onContact and call this method in order ExtImpactDamageManager to work correctly.
	
	Contacts will be processed and impact damage will be accumulated.

	\param[in] pairHeader Information on the two actors whose shapes triggered a contact report.
	\param[in] pairs The contact pairs of two actors for which contact reports have been requested. @see PxContactPair.
	\param[in] nbPairs The number of provided contact pairs.

	@see PxSimulationEventCallback
	*/
	virtual void					onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, uint32_t nbPairs) = 0;


	/**
	Apply accumulated impact damage.
	*/
	virtual void					applyDamage() = 0;


	//////// filter shader ////////

	/**
	Custom implementation of PxSimulationFilterShader, enables necessary information to be passed in onContact().
	Set it in your PxScene PxSceneDesc in order to impact damage to work correctly or implement your own.

	@see PxSimulationFilterShader
	*/
	static physx::PxFilterFlags		FilterShader(
		physx::PxFilterObjectAttributes attributes0,
		physx::PxFilterData filterData0,
		physx::PxFilterObjectAttributes attributes1,
		physx::PxFilterData filterData1,
		physx::PxPairFlags& pairFlags,
		const void* constantBlock,
		uint32_t constantBlockSize);

};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTIMPACTDAMAGEMANAGER_H
