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


#ifndef NVBLASTEXTPXACTOR_H
#define NVBLASTEXTPXACTOR_H

#include "NvBlastTypes.h"


// Forward declarations
namespace physx
{
	class PxRigidDynamic;
}


namespace Nv
{
namespace Blast
{

// Forward declarations
class ExtPxFamily;
class TkActor;


/**
Actor.

Corresponds one to one to PxRigidDynamic and ExtActor.
*/
class ExtPxActor
{
public:
	/**
	Get the number of visible chunks for this actor.  May be used in conjunction with getChunkIndices().

	\return	the number of visible chunk indices for the actor.
	*/
	virtual uint32_t				getChunkCount() const = 0;

	/**
	Access actor's array of chunk indices. Use getChunkCount() to get a size of this array.

	\return	a pointer to an array of chunk indices of an actor.
	*/
	virtual const uint32_t*			getChunkIndices() const = 0;

	/**
	Every actor has corresponding PxActor.

	/return a pointer to PxRigidDynamic actor.
	*/
	virtual physx::PxRigidDynamic&	getPhysXActor() const = 0;

	/**
	Every actor has corresponding TkActor.

	/return a pointer to TkActor actor.
	*/
	virtual TkActor&				getTkActor() const = 0;

	/**
	Every actor has corresponding ExtPxFamily.

	/return a pointer to ExtPxFamily family.
	*/
	virtual ExtPxFamily&			getFamily() const = 0;
};


} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTPXACTOR_H
