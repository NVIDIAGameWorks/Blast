/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTEXTPXASSET_H
#define NVBLASTEXTPXASSET_H

#include "NvBlastTkFramework.h"
#include "PxConvexMeshGeometry.h"
#include "PxTransform.h"
#include "NvBlastPreprocessor.h"


// Forward declarations
namespace physx
{
class PxCooking;

namespace general_PxIOStream2
{
class PxFileBuf;
}
}


namespace Nv
{
namespace Blast
{


/**
Descriptor for PxAsset creation.

PxAsset creates TkAsset internally, so TkAssetDesc must be filled.
In addition it needs physics chunks data. Every chunk can have any amount of Convexes (Subchunks).
*/
struct ExtPxAssetDesc : public TkAssetDesc
{
	/**
	Physics Subchunk.

	Represents convex and it's position.
	*/
	struct SubchunkDesc
	{
		physx::PxTransform			transform;			//!<	convex local transform
		physx::PxConvexMeshGeometry	geometry;			//!<	convex geometry
	};

	/**
	Physics Chunk.

	Contains any amount of subchunks. Empty subchunks array makes chunk invisible.
	*/
	struct ChunkDesc
	{
		SubchunkDesc*	subchunks;			//!< array of subchunks for chunk, can be empty
		uint32_t		subchunkCount;		//!< size array of subchunks for chunk, can be 0
		bool			isStatic;			//!< is chunk static. Static chunk makes PxActor Kinematic.
	};

	ChunkDesc*	pxChunks;		//!< array of chunks in asset, should be of size chunkCount (@see NvBlastAssetDesc)
};


/**
Physics Subchunk. 

Represents convex and it's local position.
*/
struct ExtPxSubchunk
{
	physx::PxTransform			transform;			//!<	convex local transform
	physx::PxConvexMeshGeometry	geometry;			//!<	convex geometry
};


/**
Physics Chunk.

Contains any amount of subchunks.
*/
struct ExtPxChunk
{
	uint32_t	firstSubchunkIndex;	//!<	first Subchunk index in Subchunk's array in ExtPhyicsAsset
	uint32_t	subchunkCount;		//!<	Subchunk count. Can be 0.
	bool		isStatic;			//!<	is chunk static (kinematic)?.
};


/**
Asset.

Keeps all the static data needed for physics.
*/
class NV_DLL_EXPORT ExtPxAsset
{
public:

	/**
	Create a new ExtPxAsset.

	\param[in]	desc			The ExtPxAssetDesc descriptor to be used, @see ExtPxAssetDesc.
	\param[in]	framework		The TkFramework instance to be used to create TkAsset.

	\return the new ExtPxAsset if successful, NULL otherwise.
	*/
	static ExtPxAsset*				create(const ExtPxAssetDesc& desc, TkFramework& framework);


	/*
		Factory method for deserialization

		Doesn't specify chunks or subchunks as they'll be fed in during deserialization to avoid copying stuff around.
	
	*/
	static ExtPxAsset*				create(TkAsset* asset);


	/**
	Deserialize an ExtPxAsset object from the given stream.

	\param[in]	stream			User-defined stream object.
	\param[in]	framework		The TkFramework instance to be used to deserialize TkAsset.
	\param[in]	physics			The PxPhysics instance to be to deserialize PxConvexMesh(s).

	\return pointer the deserialized ExtPxAsset object if successful, or NULL if unsuccessful.
	*/
	static ExtPxAsset*				deserialize(physx::general_PxIOStream2::PxFileBuf& stream, TkFramework& framework, physx::PxPhysics& physics);

	/**
	Release this ExtPxAsset.
	*/
	virtual void					release() = 0;

	/**
	Write the asset's data to the user-defined PxFileBuf stream. Underlying TkAsset would be also serialized.

	\param[in]	stream			User-defined stream object.
	\param[in]	cooking			The PxCooking instance to be used to serialize PxConvexMesh(s).

	\return true if serialization was successful, false otherwise.
	*/
	virtual bool					serialize(physx::general_PxIOStream2::PxFileBuf& stream, physx::PxCooking& cooking) const = 0;
	
	/**
	Every ExtPxAsset has corresponding TkAsset.

	/return a pointer to TkAsset actor.
	*/
	virtual const TkAsset&			getTkAsset() const = 0;

	/**
	Get the number of chunks for this asset.  May be used in conjunction with getChunks().

	\return	the number of chunks for the asset.
	*/
	virtual uint32_t				getChunkCount() const = 0;

	/**
	Access asset's array of chunks. Use getChunkCount() to get the size of this array.

	\return	a pointer to an array of chunk of an asset.
	*/
	virtual const ExtPxChunk*		getChunks() const = 0;
	
	/**
	Get the number of subchunks for this asset.  May be used in conjunction with getSubchunks().
	Subchunk count is the maximum value of ExtPxChunk: (firstSubchunkIndex + subchunkCount).

	\return	the number of subchunks for the asset.
	*/
	virtual uint32_t				getSubchunkCount() const = 0;
	
	/**
	Access asset's array of subchunks. Use getSubchunkCount() to get the size of this array.

	\return	a pointer to an array of subchunks of an asset.
	*/
	virtual const ExtPxSubchunk*	getSubchunks() const = 0;

	/**
	Pointer field available to the user.
	*/
	void*	userData;
};



} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTPXASSET_H
