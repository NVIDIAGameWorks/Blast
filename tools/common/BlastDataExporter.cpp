#include <BlastDataExporter.h>
#include "NvBlastExtPxManager.h"
#include <NvBlastExtAuthoringCollisionBuilder.h>
#include <Log.h>
#include "PsFileBuffer.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlast.h"
#include <NvBlastTkAsset.h>

using namespace Nv::Blast;


ExtPxAsset* BlastDataExporter::createExtBlastAsset(std::vector<NvBlastBondDesc>& bondDescs, const std::vector<NvBlastChunkDesc>& chunkDescs,
	std::vector<ExtPxAssetDesc::ChunkDesc>& physicsChunks)
{
	ExtPxAssetDesc	descriptor;
	descriptor.bondCount = static_cast<uint32_t>(bondDescs.size());
	descriptor.bondDescs = bondDescs.data();
	descriptor.chunkCount = static_cast<uint32_t>(chunkDescs.size());
	descriptor.chunkDescs = chunkDescs.data();
	descriptor.bondFlags = nullptr;
	descriptor.pxChunks = physicsChunks.data();
	ExtPxAsset* asset = ExtPxAsset::create(descriptor, *mFramework);
	return asset;
}



NvBlastAsset* BlastDataExporter::createLlBlastAsset(std::vector<NvBlastBondDesc>& bondDescs, const std::vector<NvBlastChunkDesc>& chunkDescs)
{

	NvBlastAssetDesc assetDesc;
	assetDesc.bondCount = static_cast<uint32_t>(bondDescs.size());
	assetDesc.bondDescs = bondDescs.data();

	assetDesc.chunkCount = static_cast<uint32_t>(chunkDescs.size());
	assetDesc.chunkDescs = chunkDescs.data();


	std::vector<uint8_t> scratch(static_cast<unsigned int>(NvBlastGetRequiredScratchForCreateAsset(&assetDesc, m_log)));
	void* mem = _aligned_malloc(NvBlastGetAssetMemorySize(&assetDesc, m_log), 16);
	NvBlastAsset* asset = NvBlastCreateAsset(mem, &assetDesc, &scratch[0], m_log);
	return asset;
}

TkAsset* BlastDataExporter::createTkBlastAsset(const std::vector<NvBlastBondDesc>& bondDescs, const std::vector<NvBlastChunkDesc>& chunkDescs)
{
	TkAssetDesc desc;
	desc.bondCount = static_cast<uint32_t>(bondDescs.size());
	desc.bondDescs = bondDescs.data();
	desc.chunkCount = static_cast<uint32_t>(chunkDescs.size());
	desc.chunkDescs = chunkDescs.data();
	desc.bondFlags = nullptr;
	TkAsset* asset = mFramework->createAsset(desc);
	return asset;
};


bool BlastDataExporter::saveBlastLLAsset(const std::string& outputFilePath, const NvBlastAsset* asset)
{
	uint32_t assetSize = NvBlastAssetGetSize(asset, m_log);

	physx::PsFileBuffer fileBuf(outputFilePath.c_str(), physx::PxFileBuf::OPEN_WRITE_ONLY);
	if (!fileBuf.isOpen())
	{
		NVBLAST_LOG_ERROR(m_log, "Can't open output buffer. \n");
		return false;
	}
	fileBuf.write(asset, sizeof(char) * assetSize);
	fileBuf.close();
	return true;
}

bool BlastDataExporter::saveBlastTkAsset(const std::string& outputFilePath, const TkAsset* asset)
{
	physx::PsFileBuffer fileBuf(outputFilePath.c_str(), physx::PxFileBuf::OPEN_WRITE_ONLY);
	if (!fileBuf.isOpen())
	{
		NVBLAST_LOG_ERROR(m_log, "Can't open output buffer. \n");
		return false;
	}
	if (!asset->serialize(fileBuf))
	{
		NVBLAST_LOG_ERROR(m_log, "Serialization failed. \n");
		return false;
	}
	fileBuf.close();
	return true;
}

bool BlastDataExporter::saveBlastExtAsset(const std::string& outputFilePath, const ExtPxAsset* asset)
{
	physx::PsFileBuffer fileBuf(outputFilePath.c_str(), physx::PxFileBuf::OPEN_WRITE_ONLY);
	if (!fileBuf.isOpen())
	{
		NVBLAST_LOG_ERROR(m_log, "Can't open output buffer. \n");
		return false;
	}
	if (!asset->serialize(fileBuf, *mCooking))
	{
		NVBLAST_LOG_ERROR(m_log, "ExtPhysicsAsset serialization failed.\n");
		return false;
	}
	fileBuf.close();
	return true;
}