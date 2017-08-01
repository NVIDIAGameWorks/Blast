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
// Copyright (c) 2017 NVIDIA Corporation. All rights reserved.


#include "NvBlastExtSerialization.h"
#include "NvBlastExtTkSerializerRAW.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlastTkAsset.h"
#include "physics/NvBlastExtPxAssetImpl.h"
#include "NvBlastIndexFns.h"
#include "NvBlastAssert.h"
#include "NvBlastExtSerializationInternal.h"

#include "PxPhysics.h"
#include "PsMemoryBuffer.h"
#include "PxIO.h"


namespace Nv
{
namespace Blast
{

// Legacy IDs
struct ExtPxSerializationLegacyID
{
	enum Enum
	{
		Asset = NVBLAST_FOURCC('B', 'P', 'X', 'A'),	//!< ExtPxAsset identifier token, used in serialization
	};
};


// Legacy object format versions
struct ExtPxSerializationLegacyAssetVersion
{
	enum Enum
	{
		/** Initial version */
		Initial,

		//	New formats must come before Count.  They should be given descriptive names with more information in comments.

		/** The number of serialized formats. */
		Count,

		/** The current version.  This should always be Count-1 */
		Current = Count - 1
	};
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//											Helpers/Wrappers
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class FileBufToPxInputStream final : public PxInputStream
{
public:
	FileBufToPxInputStream(PxFileBuf& filebuf) : m_filebuf(filebuf) {}

	virtual uint32_t read(void* dest, uint32_t count)
	{
		return m_filebuf.read(dest, count);
	}

private:
	FileBufToPxInputStream& operator=(const FileBufToPxInputStream&);

	PxFileBuf& m_filebuf;
};


class FileBufToPxOutputStream final : public PxOutputStream
{
public:
	FileBufToPxOutputStream(PxFileBuf& filebuf) : m_filebuf(filebuf) {}

	virtual uint32_t write(const void* src, uint32_t count) override
	{
		return m_filebuf.write(src, count);
	}

private:
	FileBufToPxOutputStream& operator=(const FileBufToPxOutputStream&);

	PxFileBuf& m_filebuf;
};


ExtPxAsset* deserializeExtPxAsset(ExtIStream& stream, TkFramework& framework, physx::PxPhysics& physics)
{
	// Read header
	struct LegacyAssetDataHeader
	{
		uint32_t dataType;
		uint32_t version;
	};
	LegacyAssetDataHeader header;
	stream >> header.dataType;
	stream >> header.version;
	NVBLAST_CHECK_ERROR(header.dataType == ExtPxSerializationLegacyID::Asset, "deserializeExtPxAsset: wrong data type in filebuf stream.", return nullptr);
	NVBLAST_CHECK_ERROR(header.version == ExtPxSerializationLegacyAssetVersion::Current, "deserializeExtPxAsset: wrong data version in filebuf stream.", return nullptr);

	// Read initial TkAsset
	TkAsset* tkAsset = deserializeTkAsset(stream, framework);
	NVBLAST_CHECK_ERROR(tkAsset != nullptr, "ExtPxAsset::deserialize: failed to deserialize TkAsset.", return nullptr);

	// Create ExtPxAsset
	ExtPxAssetImpl* asset = reinterpret_cast<ExtPxAssetImpl*>(ExtPxAsset::create(tkAsset));

	// Fill arrays
	auto& chunks = asset->getChunksArray();
	chunks.resize(tkAsset->getChunkCount());
	const uint32_t chunkCount = chunks.size();
	for (uint32_t i = 0; i < chunkCount; ++i)
	{
		ExtPxChunk& chunk = chunks[i];
		stream >> chunk.firstSubchunkIndex;
		stream >> chunk.subchunkCount;
		uint32_t val;
		stream >> val;
		chunk.isStatic = 0 != val;
	}

	auto& subchunks = asset->getSubchunksArray();
	uint32_t subchunkCount;
	stream >> subchunkCount;
	subchunks.resize(subchunkCount);
	for (uint32_t i = 0; i < subchunkCount; ++i)
	{
		ExtPxSubchunk& subchunk = subchunks[i];

		// Subchunk transform
		stream >> subchunk.transform.q.x >> subchunk.transform.q.y >> subchunk.transform.q.z >> subchunk.transform.q.w;
		stream >> subchunk.transform.p.x >> subchunk.transform.p.y >> subchunk.transform.p.z;

		// Subchunk scale
		stream >> subchunk.geometry.scale.scale.x >> subchunk.geometry.scale.scale.y >> subchunk.geometry.scale.scale.z;
		stream >> subchunk.geometry.scale.rotation.x >> subchunk.geometry.scale.rotation.y >> subchunk.geometry.scale.rotation.z >> subchunk.geometry.scale.rotation.w;

		uint32_t convexReuseIndex;
		stream >> convexReuseIndex;
		if (isInvalidIndex(convexReuseIndex))
		{
			physx::PsMemoryBuffer memBuf(stream.view(), stream.left());
			FileBufToPxInputStream inputStream(memBuf);
			subchunk.geometry.convexMesh = physics.createConvexMesh(inputStream);
			stream.advance(memBuf.tellRead());
		}
		else
		{
			NVBLAST_ASSERT_WITH_MESSAGE(convexReuseIndex < i, "ExtPxAsset::deserialize: wrong convexReuseIndex.");
			subchunk.geometry.convexMesh = subchunks[convexReuseIndex].geometry.convexMesh;
		}
		if (!subchunk.geometry.convexMesh)
		{
			NVBLAST_LOG_ERROR("ExtPxAsset::deserialize: failed to deserialize convex mesh.");
			return nullptr;
		}
	}

	// checking if it's the end, so it will be binary compatible with asset before m_defaultActorDesc was added
	if (!stream.eof())
	{
		auto& defaultActorDesc = asset->getDefaultActorDesc();

		stream >> defaultActorDesc.uniformInitialBondHealth;
		stream >> defaultActorDesc.uniformInitialLowerSupportChunkHealth;

		auto& bondHealths = asset->getBondHealthsArray();
		uint32_t bondHealthCount;
		stream >> bondHealthCount;
		bondHealths.resize(bondHealthCount);
		for (uint32_t i = 0; i < bondHealths.size(); ++i)
		{
			stream >> bondHealths[i];
		}
		defaultActorDesc.initialBondHealths = bondHealthCount ? bondHealths.begin() : nullptr;

		auto& supportChunkHealths = asset->getBondHealthsArray();
		uint32_t supportChunkHealthCount;
		stream >> supportChunkHealthCount;
		supportChunkHealths.resize(supportChunkHealthCount);
		for (uint32_t i = 0; i < supportChunkHealths.size(); ++i)
		{
			stream >> supportChunkHealths[i];
		}
		defaultActorDesc.initialSupportChunkHealths = supportChunkHealthCount ? supportChunkHealths.begin() : nullptr;
	}

	return asset;
}

}	// namespace Blast
}	// namespace Nv
