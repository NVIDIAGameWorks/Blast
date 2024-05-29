// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2022-2024 NVIDIA Corporation. All rights reserved.


#include "BlastDataExporter.h"
#include "NvBlastExtPxManager.h"
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
    ExtPxAssetDesc  descriptor;
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
