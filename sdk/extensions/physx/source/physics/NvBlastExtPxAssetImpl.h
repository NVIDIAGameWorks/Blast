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
// Copyright (c) 2016-2020 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTEXTPXASSETIMPL_H
#define NVBLASTEXTPXASSETIMPL_H

#include "NvBlastExtPxAsset.h"
#include "NvBlastArray.h"


namespace Nv
{
namespace Blast
{


using namespace physx;
using namespace general_PxIOStream2;


// Macro to load a uint32_t (or larger) with four characters (move it in some shared header if it's used anywhere else in Ext)
#define NVBLASTEXT_FOURCC(_a, _b, _c, _d)	( (uint32_t)(_a) | (uint32_t)(_b)<<8 | (uint32_t)(_c)<<16 | (uint32_t)(_d)<<24 )


class ExtPxAssetImpl final : public ExtPxAsset
{
	NV_NOCOPY(ExtPxAssetImpl)

public:
	friend class ExtPxAsset;

	//////// ctor ////////

	ExtPxAssetImpl(const ExtPxAssetDesc& desc, TkFramework& framework);
	ExtPxAssetImpl(const TkAssetDesc& desc, ExtPxChunk* pxChunks, ExtPxSubchunk* pxSubchunks, TkFramework& framework);
	ExtPxAssetImpl(TkAsset* asset, ExtPxAssetDesc::ChunkDesc* chunks, uint32_t chunkCount);
	ExtPxAssetImpl(TkAsset* asset);

	~ExtPxAssetImpl();


	//////// interface ////////

	virtual void					release() override;

	virtual const TkAsset&			getTkAsset() const override
	{
		return *m_tkAsset;
	}

	virtual uint32_t				getChunkCount() const override
	{
		return m_chunks.size();
	}

	virtual const ExtPxChunk*		getChunks() const override
	{
		return m_chunks.begin();
	}

	virtual uint32_t				getSubchunkCount() const override
	{
		return m_subchunks.size();
	}

	virtual const ExtPxSubchunk*	getSubchunks() const override
	{
		return m_subchunks.begin();
	}

	virtual NvBlastActorDesc&		getDefaultActorDesc() override
	{
		return m_defaultActorDesc;
	}

	virtual const NvBlastActorDesc&	getDefaultActorDesc() const override
	{
		return m_defaultActorDesc;
	}

	virtual void					setUniformHealth(bool enabled) override;

	virtual void					setAccelerator(NvBlastExtDamageAccelerator* accelerator) override
	{
		m_accelerator = accelerator;
	}

	virtual NvBlastExtDamageAccelerator* getAccelerator() const override
	{
		return m_accelerator;
	}


	//////// internal public methods ////////

	/*
		Get the underlying array for the chunks. Used for serialization.
	*/
	Array<ExtPxChunk>::type&		getChunksArray() { return m_chunks; }

	/*
	Get the underlying array for the subchunks. Used for serialization.
	*/
	Array<ExtPxSubchunk>::type&		getSubchunksArray() { return m_subchunks; }

	/*
	Get the underlying array for the bond healths. Used for serialization.
	*/
	Array<float>::type&				getBondHealthsArray() { return m_bondHealths; }

	/*
	Get the underlying array for the support chunk healths. Used for serialization.
	*/
	Array<float>::type&				getSupportChunkHealthsArray() { return m_supportChunkHealths; }

private:

	////////   initialization   /////////
	void fillPhysicsChunks(ExtPxChunk* pxChunks, ExtPxSubchunk* pxSuchunk, uint32_t chunkCount);
	void fillPhysicsChunks(ExtPxAssetDesc::ChunkDesc* desc, uint32_t count);


	 //////// data ////////

	TkAsset*					 m_tkAsset;
	Array<ExtPxChunk>::type		 m_chunks;
	Array<ExtPxSubchunk>::type	 m_subchunks;
	Array<float>::type			 m_bondHealths;
	Array<float>::type			 m_supportChunkHealths;
	NvBlastExtDamageAccelerator* m_accelerator;
	NvBlastActorDesc			 m_defaultActorDesc;
};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTPXASSETIMPL_H
