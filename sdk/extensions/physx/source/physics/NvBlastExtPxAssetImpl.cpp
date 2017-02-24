/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastExtPxAssetImpl.h"
#include "NvBlastExtHashMap.h"

#include "NvBlastAssert.h"
#include "NvBlastIndexFns.h"

#include "NvBlastTkAsset.h"

#include "PxIO.h"
#include "PxPhysics.h"
#include "PxFileBuf.h"
#include "cooking/PxCooking.h"

#include <algorithm>

namespace Nv
{
namespace Blast
{


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


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//										ExtPxAssetImpl Implementation
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ExtPxAssetImpl::ExtPxAssetImpl(const ExtPxAssetDesc& desc, TkFramework& framework)
{
	m_tkAsset = framework.createAsset(desc);

	// count subchunks and reserve memory
	uint32_t subchunkCount = 0;
	for (uint32_t i = 0; i < desc.chunkCount; ++i)
	{
		const auto& chunk = desc.pxChunks[i];
		subchunkCount += static_cast<uint32_t>(chunk.subchunkCount);
	}
	m_subchunks.reserve(subchunkCount);

	// fill chunks and subchunks
	m_chunks.resize(desc.chunkCount);
	for (uint32_t i = 0; i < desc.chunkCount; ++i)
	{
		const auto& chunk = desc.pxChunks[i];
		m_chunks[i].isStatic = chunk.isStatic;
		m_chunks[i].firstSubchunkIndex = m_subchunks.size();
		m_chunks[i].subchunkCount = chunk.subchunkCount;
		for (uint32_t k = 0; k < chunk.subchunkCount; ++k)
		{
			ExtPxSubchunk subchunk =
			{
				chunk.subchunks[k].transform,
				chunk.subchunks[k].geometry
			};
			m_subchunks.pushBack(subchunk);
		}
	}
}

ExtPxAssetImpl::ExtPxAssetImpl(TkAsset* tkAsset):
	m_tkAsset(tkAsset)
{

}

ExtPxAssetImpl::~ExtPxAssetImpl()
{
	if (m_tkAsset)
	{
		m_tkAsset->release();
	}
}

void ExtPxAssetImpl::release()
{
	NVBLASTEXT_DELETE(this, ExtPxAssetImpl);
}

NV_INLINE bool serializeConvexMesh(const PxConvexMesh& convexMesh, PxCooking& cooking, ExtArray<uint32_t>::type& indicesScratch, 
	ExtArray<PxHullPolygon>::type hullPolygonsScratch, PxOutputStream& stream)
{
	PxConvexMeshDesc desc;
	desc.points.data = convexMesh.getVertices();
	desc.points.count = convexMesh.getNbVertices();
	desc.points.stride = sizeof(PxVec3);

	hullPolygonsScratch.resize(convexMesh.getNbPolygons());

	uint32_t indexCount = 0;
	for (uint32_t i = 0; i < convexMesh.getNbPolygons(); i++)
	{
		PxHullPolygon polygon;
		convexMesh.getPolygonData(i, polygon);
		if (polygon.mNbVerts)
		{
			indexCount = std::max<uint32_t>(indexCount, polygon.mIndexBase + polygon.mNbVerts);
		}
	}
	indicesScratch.resize(indexCount);

	for (uint32_t i = 0; i < convexMesh.getNbPolygons(); i++)
	{
		PxHullPolygon polygon;
		convexMesh.getPolygonData(i, polygon);
		for (uint32_t j = 0; j < polygon.mNbVerts; j++)
		{
			indicesScratch[polygon.mIndexBase + j] = convexMesh.getIndexBuffer()[polygon.mIndexBase + j];
		}

		hullPolygonsScratch[i] = polygon;
	}

	desc.indices.count = indexCount;
	desc.indices.data = indicesScratch.begin();
	desc.indices.stride = sizeof(uint32_t);

	desc.polygons.count = convexMesh.getNbPolygons();
	desc.polygons.data = hullPolygonsScratch.begin();
	desc.polygons.stride = sizeof(PxHullPolygon);

	return cooking.cookConvexMesh(desc, stream);
}

bool ExtPxAssetImpl::serialize(PxFileBuf& stream, PxCooking& cooking) const
{
	// Header data
	stream.storeDword(ClassID);
	stream.storeDword(Version::Current);

	m_tkAsset->serialize(stream);

	// Chunks
	const uint32_t chunkCount = m_tkAsset->getChunkCount();
	for (uint32_t i = 0; i < chunkCount; ++i)
	{
		const ExtPxChunk& chunk = m_chunks[i];
		stream.storeDword(chunk.firstSubchunkIndex);
		stream.storeDword(chunk.subchunkCount);
		stream.storeDword(chunk.isStatic ? 1 : 0);
	}

	stream.storeDword(m_subchunks.size());

	ExtArray<uint32_t>::type indicesScratch(512);
	ExtArray<PxHullPolygon>::type hullPolygonsScratch(512);
	ExtHashMap<PxConvexMesh*, uint32_t>::type convexReuseMap;

	FileBufToPxOutputStream outputStream(stream);
	for (uint32_t i = 0; i < m_subchunks.size(); ++i)
	{
		auto& subchunk = m_subchunks[i];

		// Subchunk transform
		stream.storeFloat(subchunk.transform.q.x);	stream.storeFloat(subchunk.transform.q.y);	stream.storeFloat(subchunk.transform.q.z);	stream.storeFloat(subchunk.transform.q.w);
		stream.storeFloat(subchunk.transform.p.x);	stream.storeFloat(subchunk.transform.p.y);	stream.storeFloat(subchunk.transform.p.z);

		// Subchunk scale
		stream.storeFloat(subchunk.geometry.scale.scale.x);	stream.storeFloat(subchunk.geometry.scale.scale.y);	stream.storeFloat(subchunk.geometry.scale.scale.z);
		stream.storeFloat(subchunk.geometry.scale.rotation.x);	stream.storeFloat(subchunk.geometry.scale.rotation.y);	stream.storeFloat(subchunk.geometry.scale.rotation.z);	stream.storeFloat(subchunk.geometry.scale.rotation.w);

		auto convexMesh = subchunk.geometry.convexMesh;
		NVBLASTEXT_CHECK_ERROR(convexMesh != nullptr, "ExtPxAssetImpl::serialize: subchunk convexMesh is nullptr.", return false);

		auto entry = convexReuseMap.find(convexMesh);
		if (entry)
		{
			stream.storeDword(entry->second);
		}
		else
		{
			stream.storeDword(invalidIndex<uint32_t>());
			if (!serializeConvexMesh(*convexMesh, cooking, indicesScratch, hullPolygonsScratch, outputStream))
			{
				NVBLASTEXT_LOG_ERROR("ExtPxAssetImpl::serialize: subchunk convexMesh cooking/serialization failed.");
				return false;
			}
			convexReuseMap[convexMesh] = i;
		}
	}

	return true;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//										ExtPxAsset Static
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ExtPxAsset*	ExtPxAsset::create(const ExtPxAssetDesc& desc, TkFramework& framework)
{
	ExtPxAssetImpl* asset = NVBLASTEXT_NEW(ExtPxAssetImpl)(desc, framework);
	return asset;
}


Nv::Blast::ExtPxAsset* ExtPxAsset::create(TkAsset* tkAsset)
{
	ExtPxAssetImpl* asset = NVBLASTEXT_NEW(ExtPxAssetImpl)(tkAsset);

	// Don't populate the chunks or subchunks!

	return asset;
}

ExtPxAsset* ExtPxAsset::deserialize(PxFileBuf& stream, TkFramework& framework, PxPhysics& physics)
{
	ExtPxAssetImpl::DataHeader header;
	header.dataType = stream.readDword();
	header.version = stream.readDword();
	NVBLASTEXT_CHECK_ERROR(header.dataType == ExtPxAssetImpl::ClassID,	"ExtPxAsset::deserialize: wrong data type in filebuf stream.",	return nullptr);
	NVBLASTEXT_CHECK_ERROR(header.version == ExtPxAssetImpl::Version::Current, "ExtPxAsset::deserialize: wrong data version in filebuf stream.", return nullptr);

	TkAsset* tkAsset = static_cast<TkAsset*>(framework.deserialize(stream));
	NVBLASTEXT_CHECK_ERROR(tkAsset != nullptr, "ExtPxAsset::deserialize: failed to deserialize TkAsset.", return nullptr);

	ExtPxAssetImpl* asset = NVBLASTEXT_NEW(ExtPxAssetImpl)(tkAsset);

	asset->m_chunks.resize(asset->m_tkAsset->getChunkCount());

	const uint32_t chunkCount = asset->m_chunks.size();
	for (uint32_t i = 0; i < chunkCount; ++i)
	{
		ExtPxChunk& chunk = asset->m_chunks[i];
		chunk.firstSubchunkIndex = stream.readDword();
		chunk.subchunkCount = stream.readDword();
		chunk.isStatic = 0 != stream.readDword();
	}

	const uint32_t subchunkCount = stream.readDword();
	asset->m_subchunks.resize(subchunkCount);

	FileBufToPxInputStream inputStream(stream);
	for (uint32_t i = 0; i < asset->m_subchunks.size(); ++i)
	{
		ExtPxSubchunk& subChunk = asset->m_subchunks[i];

		// Subchunk transform
		subChunk.transform.q.x = stream.readFloat();	subChunk.transform.q.y = stream.readFloat();	subChunk.transform.q.z = stream.readFloat();	subChunk.transform.q.w = stream.readFloat();
		subChunk.transform.p.x = stream.readFloat();	subChunk.transform.p.y = stream.readFloat();	subChunk.transform.p.z = stream.readFloat();

		// Subchunk scale
		subChunk.geometry.scale.scale.x = stream.readFloat();	subChunk.geometry.scale.scale.y = stream.readFloat();	subChunk.geometry.scale.scale.z = stream.readFloat();
		subChunk.geometry.scale.rotation.x = stream.readFloat();	subChunk.geometry.scale.rotation.y = stream.readFloat();	subChunk.geometry.scale.rotation.z = stream.readFloat();	subChunk.geometry.scale.rotation.w = stream.readFloat();

		const uint32_t convexReuseIndex = stream.readDword();
		if (isInvalidIndex(convexReuseIndex))
		{
			subChunk.geometry.convexMesh = physics.createConvexMesh(inputStream);
		}
		else
		{
			NVBLAST_ASSERT_WITH_MESSAGE(convexReuseIndex < i, "ExtPxAsset::deserialize: wrong convexReuseIndex.");
			subChunk.geometry.convexMesh = asset->m_subchunks[convexReuseIndex].geometry.convexMesh;
		}
		if (!subChunk.geometry.convexMesh)
		{
			NVBLASTEXT_LOG_ERROR("ExtPxAsset::deserialize: failed to deserialize convex mesh.");
			asset->release();
			return nullptr;
		}
	}

	return asset;
}


} // namespace Blast
} // namespace Nv
