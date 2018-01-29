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
// Copyright (c) 2016-2018 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTEXTPXMANAGER_H
#define NVBLASTEXTPXMANAGER_H

#include "NvBlastTypes.h"
#include "PxConvexMeshGeometry.h"
#include "PxTransform.h"
#include "NvPreprocessor.h"


// Forward declarations
namespace physx
{
class PxPhysics;
class PxRigidDynamic;
class PxJoint;

namespace general_PxIOStream2
{
class PxFileBuf;
}
}


namespace Nv
{
namespace Blast
{

// Forward declarations
class ExtPxActor;
class ExtPxAsset;
class ExtPxFamily;
class ExtPxListener;
class TkFamily;
class TkFramework;
class TkGroup;
class TkJoint;


/**
Family Desc.

Used to create Physics Family.
*/
struct ExtPxFamilyDesc
{
	ExtPxAsset*				pxAsset;		//!< px asset to create from, pointer will be stored in family.
	const NvBlastActorDesc*	actorDesc;		//!< actor descriptor to be used when creating TkActor. If nullptr, default NvBlastActorDesc from ExtPxAsset will be used.
	TkGroup*				group;			//!< if not nullptr, created TkActor will be placed in group
};


/**
Function pointer for PxJoint creation.

It will be called when new joints are being created. It should return valid PxJoint pointer or nullptr.
*/
typedef physx::PxJoint*(*ExtPxCreateJointFunction)(ExtPxActor* actor0, const physx::PxTransform& localFrame0, ExtPxActor* actor1, const physx::PxTransform& localFrame1, physx::PxPhysics& physics, TkJoint& joint);


/**
Physics Manager.

Used to create and manage Physics Families.
*/
class NV_DLL_EXPORT ExtPxManager
{
public:
	//////// manager creation ////////

	/**
	Create a new ExtPxManager.

	\param[in]	physics			The PxPhysics instance to be used by ExtPxManager.
	\param[in]	framework		The TkFramework instance to be used by ExtPxManager.
	\param[in]	createFn		The function to be used when creating joints, can be nullptr.
	\param[in]	useUserData		Flag if ExtPxManager is allowed to override PxActor's userData, it will store pointer to PxActor there.
	It is recommended as fastest way. If set to 'false' HashMap will be used.

	\return the new ExtPxManager if successful, NULL otherwise.
	*/
	static ExtPxManager*		create(physx::PxPhysics& physics, TkFramework& framework, ExtPxCreateJointFunction createFn = nullptr, bool useUserData = true);

	/**
	Release this manager.
	*/
	virtual void				release() = 0;


	//////// impact ////////

	/**
	Simulation Filter data to be set on leaf chunk actors
	*/
	enum FilterDataAttributes
	{
		LEAF_CHUNK = 1,
	};


	//////// interface ////////

	/**
	Create a px family from the given descriptor.

	\param[in]	desc			The family descriptor (see ExtPxFamilyDesc).

	\return the created family, if the descriptor was valid and memory was available for the operation.  Otherwise, returns NULL.
	*/
	virtual ExtPxFamily*		createFamily(const ExtPxFamilyDesc& desc) = 0;

	/**
	Create a px joint associated with TkJoint.

	ExtPxCreateJointFunction will be called after this call.
	ExtPxCreateJointFunction must be set, nothing will happen otherwise.

	\param[in]	joint			TkJoint to be used to create px joint.

	\return true iff Joint was created.
	*/
	virtual bool				createJoint(TkJoint& joint) = 0;

	/**
	Destroy a px joint associated with TkJoint.

	\param[in]	joint			TkJoint to be used to destroy px joint.
	*/
	virtual void				destroyJoint(TkJoint& joint) = 0;

	/**
	Set ExtPxCreateJointFunction to be used when new joints are being created.\

	\param[in]	createFn		Create function pointer to set, can be nullptr.
	*/
	virtual void				setCreateJointFunction(ExtPxCreateJointFunction createFn) = 0;

	/**
	The number of families currently in this manager.

	\return the number of ExtPxFamily that currently exist in this manger.
	*/
	virtual uint32_t			getFamilyCount() const = 0;

	/**
	Retrieve an array of pointers (into the user-supplied buffer) to families.

	\param[out]	buffer			A user-supplied array of ExtPxFamily pointers.
	\param[in]	bufferSize		The number of elements available to write into buffer.

	\return the number of ExtPxFamily pointers written to the buffer.
	*/
	virtual uint32_t			getFamilies(ExtPxFamily** buffer, uint32_t bufferSize) const = 0;

	/**
	Look up an associated ExtPxFamily by TkFamily pointer.

	\param[in]	family			The TkFamily pointer to look up.

	\return pointer to the ExtPxFamily object if it exists, NULL otherwise.
	*/
	virtual ExtPxFamily*		getFamilyFromTkFamily(TkFamily& family) const = 0;

	/**
	Look up an associated ExtPxActor by PxRigidDynamic pointer.

	\param[in]	pxActor			The PxRigidDynamic pointer to look up.

	\return pointer to the ExtPxActor object if it exists, NULL otherwise.
	*/
	virtual ExtPxActor*			getActorFromPhysXActor(const physx::PxRigidDynamic& pxActor) const = 0;

	/**
	Get a PxPhysics object pointer used upon manager creation.

	\return a pointer to the (const) PxPhysics object.
	*/
	virtual physx::PxPhysics&	getPhysics() const = 0;

	/**
	Get a TkFramework object pointer used upon manager creation.

	\return a pointer to the TkFramework object.
	*/
	virtual TkFramework&		getFramework() const = 0;

	/**
	Get if useUserData was set upon manager creation.

	\return true iff PxActor userData is used by manager.
	*/
	virtual bool				isPxUserDataUsed() const = 0;

	/**
	Limits the total number of actors that can exist at a given time.  A value of zero disables this (gives no limit).

	\param[in]	limit			If not zero, the maximum number of actors that will be allowed to exist.
	*/
	virtual void				setActorCountLimit(uint32_t limit) = 0;

	/**
	Retrieve the limit to the total number of actors that can exist at a given time.  A value of zero disables this (gives no limit).

	\return the limit to the total number of actors that can exist at a given time (or zero if there is no limit).
	*/
	virtual uint32_t			getActorCountLimit() = 0;

	/**
	The total number of PxActors generated by Blast.

	\return the total number of PxActors generated by Blast.
	*/
	virtual uint32_t			getPxActorCount() const = 0;

	/**
	Add a user implementation of ExtPxListener to this family's list of listeners.

	\param[in]	listener		The event listener to add.
	*/
	virtual void				subscribe(ExtPxListener& listener) = 0;

	/**
	Remove a user implementation of ExtPxListener from this family's list of listeners.

	\param[in]	listener		The event listener to remove.
	*/
	virtual void				unsubscribe(ExtPxListener& listener) = 0;
};


} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTPXMANAGER_H
