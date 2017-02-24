/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTTKASSET_H
#define NVBLASTTKASSET_H

#include "NvBlastTkSerializable.h"
#include "NvBlastTypes.h"
#include "PxVec3.h"

// Forward declarations
struct NvBlastAsset;


namespace Nv
{
namespace Blast
{

/**
A descriptor stored by a TkAsset for an internal joint.  Internal joints are created when a TkAsset is instanced into a TkActor.
*/
struct TkAssetJointDesc
{
	uint32_t		nodeIndices[2];		//!< The graph node indices corresponding to the support chunks joined by a joint
	physx::PxVec3	attachPositions[2];	//!< The joint's attachment positions in asset-local space
};


/**
The static data associated with a destructible actor.  TkAsset encapsulates an NvBlastAsset.  In addition to the NvBlastAsset,
the TkAsset stores joint descriptors (see TkAssetJointDesc).
*/
class TkAsset : public TkSerializable
{
public:
	/**
	Access to underlying low-level asset.

	\return a pointer to the (const) low-level NvBlastAsset object.
	*/
	virtual const NvBlastAsset*			getAssetLL() const = 0;

	/**
	Get the number of chunks in this asset.

	NOTE: Wrapper function over low-level function call, see NvBlastAssetGetChunkCount for details.

	\return	the number of chunks in the asset.
	*/
	virtual uint32_t					getChunkCount() const = 0;

	/**
	Get the number of leaf chunks in the given asset.

	NOTE: Wrapper function over low-level function call, see NvBlastAssetGetLeafChunkCount for details.

	\return	the number of leaf chunks in the asset.
	*/
	virtual uint32_t					getLeafChunkCount() const = 0;

	/**
	Get the number of bonds in the given asset.

	NOTE: Wrapper function over low-level function call, see NvBlastAssetGetBondCount for details.

	\return	the number of bonds in the asset.
	*/
	virtual uint32_t					getBondCount() const = 0;

	/**
	Access an array of chunks of the given asset.
		
	NOTE: Wrapper function over low-level function call, see NvBlastAssetGetChunks for details.

	\return	a pointer to an array of chunks of the asset.
	*/
	virtual const NvBlastChunk*			getChunks() const = 0;

	/**
	Access an array of bonds of the given asset.

	NOTE: Wrapper function over low-level function call, see NvBlastAssetGetBonds for details.

	\return	a pointer to an array of bonds of the asset.
	*/
	virtual const NvBlastBond*			getBonds() const = 0;

	/**
	Access an support graph for the given asset.

	NOTE: Wrapper function over low-level function call, see NvBlastAssetGetSupportGraph for details.

	\return	a struct of support graph for the given asset.
	*/
	virtual const NvBlastSupportGraph	getGraph() const = 0;

	/**
	Retrieve the size (in bytes) of the LL asset.

	NOTE: Wrapper function over low-level function call, see NvBlastAssetGetSize for details.

	\return	the size of the data block (in bytes).
	*/
	virtual uint32_t					getDataSize() const = 0;

	/**
	The number of internal TkJoint objects that will be created when this asset is instanced into a TkActor
	(see TkFramework::createActor).  These joints will not trigger TkJointUpdateEvent events
	until this actor is split into actors such that a joint connects two actors.  At this time the actor's family
	will dispatch a TkJointUpdateEvent::External event during a call to TkGroup::sync() (see TkGroup).

	\return the number of descriptors for internal joints.
	*/
	virtual uint32_t					getJointDescCount() const = 0;

	/**
	The descriptors for the internal joints created when this asset is instanced.  (See getJointDescCount.)

	\return a pointer to the array of descriptors for internal joints.
	*/
	virtual const TkAssetJointDesc*		getJointDescs() const = 0;
};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTTKASSET_H
