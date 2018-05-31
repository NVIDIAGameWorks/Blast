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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.


#include "BlastDataExporter.h"
#include "NvBlastExtPxManager.h"
#include <NvBlastExtAuthoringCollisionBuilder.h>
#include "NvBlastExtSerialization.h"
#include "NvBlastExtLlSerialization.h"
#include "NvBlastExtTkSerialization.h"
#include "NvBlastExtPxSerialization.h"
#include <Log.h>
#include "PsFileBuffer.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlast.h"
#include "NvBlastGlobals.h"
#include <NvBlastTkAsset.h>
#include <fstream>
using namespace Nv::Blast;


BlastDataExporter::BlastDataExporter(TkFramework* framework, physx::PxPhysics* physics, physx::PxCooking* cooking) : mFramework(framework)
{
	mSerialization = NvBlastExtSerializationCreate();
	if (mSerialization != nullptr && physics != nullptr && cooking != nullptr && framework != nullptr)
	{
		NvBlastExtTkSerializerLoadSet(*framework, *mSerialization);
		NvBlastExtPxSerializerLoadSet(*framework, *physics, *cooking, *mSerialization);
		mSerialization->setSerializationEncoding(NVBLAST_FOURCC('C', 'P', 'N', 'B'));
	}
}


BlastDataExporter::~BlastDataExporter()
{
	if (mSerialization != nullptr)
	{
		mSerialization->release();
	}
}


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

	std::vector<uint8_t> scratch(static_cast<unsigned int>(NvBlastGetRequiredScratchForCreateAsset(&assetDesc, logLL)));
	void* mem = NVBLAST_ALLOC(NvBlastGetAssetMemorySize(&assetDesc, logLL));
	NvBlastAsset* asset = NvBlastCreateAsset(mem, &assetDesc, scratch.data(), logLL);
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


bool BlastDataExporter::saveBlastObject(const std::string& outputDir, const std::string& objectName, const void* object, uint32_t objectTypeID)
{
	void* buffer;
	const uint64_t bufferSize = mSerialization->serializeIntoBuffer(buffer, object, objectTypeID);
	if (bufferSize == 0)
	{
		std::cerr << "saveBlastObject: Serialization failed.\n";
		return false;
	}

	physx::PsFileBuffer fileBuf((outputDir + "/" + objectName + ".blast").c_str(), physx::PxFileBuf::OPEN_WRITE_ONLY);
	bool result = fileBuf.isOpen();

	if (!result)
	{
		std::cerr << "Can't open output buffer.\n";
	}
	else
	{
		result = (bufferSize == (size_t)fileBuf.write(buffer, (uint32_t)bufferSize));
		if (!result)
		{
			std::cerr << "Buffer write failed.\n";
		}
		fileBuf.close();
	}

	NVBLAST_FREE(buffer);

	return result;
};
