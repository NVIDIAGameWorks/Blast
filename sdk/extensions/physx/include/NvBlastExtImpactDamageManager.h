/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

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
	bool					isSelfCollissionEnabled;	//!<	family's self collision enabled
	float					fragility;					//!<	global fragility factor
	ExtImpactDamageFunction damageFunction;				//!<	custom damage function, can be nullptr, default internal one will be used in that case.
	void*					damageFunctionData;			//!<	data to be passed in custom damage function


	ExtImpactSettings() :
		isSelfCollissionEnabled(false),
		fragility(1.0f),
		damageFunction(nullptr)
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
