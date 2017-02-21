/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTEXTPXASSETIMPL_H
#define NVBLASTEXTPXASSETIMPL_H

#include "NvBlastExtPxAsset.h"
#include "NvBlastExtArray.h"
#include "NvBlastExtDefs.h"


namespace Nv
{
namespace Blast
{


using namespace physx;
using namespace general_PxIOStream2;


class ExtPxAssetImpl final : public ExtPxAsset
{
	NV_NOCOPY(ExtPxAssetImpl)

public:
	friend class ExtPxAsset;

	/**
	Enum which keeps track of the serialized data format.
	*/
	enum Version
	{
		/** Initial version */
		Initial,

		//	New formats must come before Count.  They should be given descriptive names with more information in comments.

		/** The number of serialized formats. */
		Count,

		/** The current version.  This should always be Count-1 */
		Current = Count - 1
	};

	//////// ctor ////////

	ExtPxAssetImpl(const ExtPxAssetDesc& desc, TkFramework& framework);
	ExtPxAssetImpl(TkAsset* tkAsset);

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

	virtual bool					serialize(PxFileBuf& stream, PxCooking& cooking) const override;


	/*
		Get the underlying array for the chunks. Used for serialization.
	*/
	ExtArray<ExtPxChunk>::type&		getChunksArray() { return m_chunks; }

	/*
	Get the underlying array for the subchunks. Used for serialization.
	*/
	ExtArray<ExtPxSubchunk>::type&	getSubchunksArray() { return m_subchunks; }

private:
	//////// serialization data ////////

	struct DataHeader
	{
		uint32_t dataType;
		uint32_t version;
	};

	enum { ClassID = NVBLASTEXT_FOURCC('B', 'P', 'X', 'A') }; // Blast PhysX Asset


	 //////// data ////////

	TkAsset*						m_tkAsset;
	ExtArray<ExtPxChunk>::type		m_chunks;
	ExtArray<ExtPxSubchunk>::type	m_subchunks;
};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTPXASSETIMPL_H
