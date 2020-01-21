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


#ifndef NVBLASTTKACTOR_H
#define NVBLASTTKACTOR_H

#include "NvBlastTkObject.h"
#include "NvBlastTypes.h"

// Forward declarations
struct NvBlastActor;
struct NvBlastFamily;


namespace Nv
{
namespace Blast
{

// Forward declarations
class TkAsset;
class TkFamily;
class TkGroup;
class TkJoint;


/**
The BlastTk entity which encapsulates an NvBlastActor.  Every TkActor represents a group
of chunks which may correspond to a single physical rigid body.  TkActors are created
using TkFramework::createActor.
*/
class TkActor : public TkObject
{
public:
	/**
	Access to underlying low-level actor.

	\return a pointer to the (const) low-level NvBlastActor object.
	*/
	virtual const NvBlastActor*	getActorLL() const = 0;

	/**
	Every actor is part of an actor family, even if that family contains a single actor.
	This function returns a reference to the actor's TkFamily.

	\return a pointer to the actor's TkFamily.
	*/
	virtual TkFamily&			getFamily() const = 0;

	/**
	Every actor has a unique index within a family.  This function returns that index.
	*/
	virtual uint32_t			getIndex() const = 0;

	/**
	Actors may be part of (no more than) one group.  See TkGroup for the functions to add and remove actors.
	This function returns a pointer to the actor's group, or NULL if it is not in a group.
	*/
	virtual TkGroup*			getGroup() const = 0;

	/**
	Remove this actor from its group, if it is in one.

	\return the actor's former group if successful, NULL otherwise.
	*/
	virtual TkGroup*			removeFromGroup() = 0;

	/**
	Every actor has an associated asset.

	\return a pointer to the (const) TkAsset object.
	*/
	virtual const TkAsset*		getAsset() const = 0;

	/**
	Get the number of visible chunks for this actor.  May be used in conjunction with getVisibleChunkIndices.

	NOTE: Wrapper function over low-level function call, see NvBlastActorGetVisibleChunkCount for details.

	\return	the number of visible chunk indices for the actor.
	*/
	virtual uint32_t			getVisibleChunkCount() const = 0;

	/**
	Retrieve a list of visible chunk indices for the actor into the given array.

	NOTE: Wrapper function over low-level function call, see NvBlastActorGetVisibleChunkIndices for details.

	\param[in] visibleChunkIndices		User-supplied array to be filled in with indices of visible chunks for this actor.
	\param[in] visibleChunkIndicesSize	The size of the visibleChunkIndices array.  To receive all visible chunk indices, the size must be at least that given by getVisibleChunkCount().

	\return	the number of indices written to visibleChunkIndices.  This will not exceed visibleChunkIndicesSize.
	*/
	virtual uint32_t			getVisibleChunkIndices(uint32_t* visibleChunkIndices, uint32_t visibleChunkIndicesSize) const = 0;

	/**
	Get the number of graph nodes for this actor.  May be used in conjunction with getGraphNodeIndices.

	NOTE: Wrapper function over low-level function call, see NvBlastActorGetGraphNodeCount for details.

	\return	the number of graph node indices for the actor.
	*/
	virtual uint32_t			getGraphNodeCount() const = 0;

	/**
	Retrieve a list of graph node indices for the actor into the given array.

	NOTE: Wrapper function over low-level function call, see NvBlastActorGetGraphNodeIndices for details.

	\param[in] graphNodeIndices		User-supplied array to be filled in with indices of graph nodes for this actor.
	\param[in] graphNodeIndicesSize	The size of the graphNodeIndices array.  To receive all graph node indices, the size must be at least that given by getGraphNodeCount().

	\return	the number of indices written to graphNodeIndices.  This will not exceed graphNodeIndicesSize.
	*/
	virtual uint32_t			getGraphNodeIndices(uint32_t* graphNodeIndices, uint32_t graphNodeIndicesSize) const = 0;

	/**
	Access the bond health data for an actor. 

	NOTE: Wrapper function over low-level function call, see NvBlastActorGetBondHealths for details.

	\return	the array of bond healths for the actor's family, or NULL if the actor is invalid.
	*/
	virtual const float*		getBondHealths() const = 0;

	/**
	Returns the upper-bound number of actors which can be created by splitting this actor.

	NOTE: Wrapper function over low-level function call, see NvBlastActorGetMaxActorCountForSplit for details.

	\return	the upper-bound number of actors which can be created by splitting this actor.
	*/
	virtual uint32_t			getSplitMaxActorCount() const = 0;

	/**
	Report whether this actor is in 'pending' state.  Being in 'pending' state leads to actor being processed by group.

	\return true iff actor is in 'pending' state.
	*/
	virtual bool				isPending() const = 0;

	/**
	Apply damage to this actor.

	Actual damage processing is deferred till the group worker process() call. Sets actor in 'pending' state.

	It's the user's responsibility to keep programParams pointer alive until the group endProcess() call.

	\param[in] program				A NvBlastDamageProgram containing damage shaders.
	\param[in] programParams		Parameters for the NvBlastDamageProgram.
	*/
	virtual void				damage(const NvBlastDamageProgram& program, const void* programParams) = 0;

	/**
	Creates fracture commands for the actor using an NvBlastMaterialFunction.

	Cannot be called during group processing, in that case a warning will be raised and function will do nothing.

	NOTE: Wrapper function over low-level function call, see NvBlastActorGenerateFracture for details.

	\param[in,out]	commands		Target buffers to hold generated commands.
									To avoid data loss, provide an entry for every support chunk and every bond in the original actor.
	\param[in]		program			A NvBlastDamageProgram containing damage shaders.
	\param[in]		programParams	Parameters for the NvBlastDamageProgram.
	*/
	virtual void				generateFracture(NvBlastFractureBuffers* commands, const NvBlastDamageProgram& program, const void* programParams) const = 0;

	/**
	Function applies the direct fracture and breaks graph bonds/edges as necessary. Sets actor in 'pending' state if any bonds or chunks were damaged. Dispatches FractureCommand events.

	NOTE: Calls NvBlastActorApplyFracture internally. see NvBlastActorApplyFracture for details.

	\param[in,out]	eventBuffers	Target buffers to hold applied fracture events. May be NULL, in which case events are not reported.
									To avoid data loss, provide an entry for every lower-support chunk and every bond in the original actor.
	\param[in]		commands		The fracture commands to process.
	*/
	virtual void				applyFracture(NvBlastFractureBuffers* eventBuffers, const NvBlastFractureBuffers* commands) = 0;

	/**
	The number of joints currently attached to this actor.

	\return the number of TkJoints that are currently attached to this actor.
	*/
	virtual uint32_t			getJointCount() const = 0;

	/**
	Retrieve an array of pointers (into the user-supplied buffer) to joints.

	\param[out]	joints		A user-supplied array of TkJoint pointers.
	\param[in]	jointsSize	The number of elements available to write into the joints array.

	\return the number of TkJoint pointers written to the joints array.
	*/
	virtual uint32_t			getJoints(TkJoint** joints, uint32_t jointsSize) const = 0;

	/**
	Whether or not this actor is bound to the world using a bond with an invalid chunk index to represent the NRF.

	NOTE: Wrapper function over low-level function call NvBlastActorIsBoundToWorld.

	\return true iff this actor contains the "world" support graph node, created when a bond contains the UINT32_MAX value for one of their chunkIndices.
	*/
	virtual bool				isBoundToWorld() const = 0;
};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTTKACTOR_H
