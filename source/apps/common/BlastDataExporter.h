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
// Copyright (c) 2022-2023 NVIDIA Corporation. All rights reserved.


#ifndef BLAST_DATA_EXPORTER
#define BLAST_DATA_EXPORTER


#include <NvBlastIndexFns.h>
#include <NvBlastExtAuthoringTypes.h>
#include <NvBlastExtPxAsset.h>
#include <vector>
#include <string>

using namespace Nv::Blast;

namespace physx
{
class PxPhysics;
class PxCooking;
}


struct NvBlastBondDesc;
struct NvBlastChunkDesc;

struct NvBlastAsset;
namespace Nv
{
namespace Blast
{
class TkAsset;
class ExtPxAsset;
class ExtSerialization;
}
}


/**
    Tool for Blast asset creation and exporting
*/
class BlastDataExporter
{
public:
    BlastDataExporter(TkFramework* framework, physx::PxPhysics* physics, physx::PxCooking* cooking);
    ~BlastDataExporter();

    /**
        Creates ExtPxAsset
    */
    ExtPxAsset*     createExtBlastAsset(std::vector<NvBlastBondDesc>& bondDescs, const std::vector<NvBlastChunkDesc>& chunkDescs,
        std::vector<ExtPxAssetDesc::ChunkDesc>& physicsChunks);
    /**
        Creates Low Level Blast asset 
    */
    NvBlastAsset*   createLlBlastAsset(std::vector<NvBlastBondDesc>& bondDescs, const std::vector<NvBlastChunkDesc>& chunkDescs);

    /**
        Creates Blast Toolkit Asset asset 
    */
    TkAsset*        createTkBlastAsset(const std::vector<NvBlastBondDesc>& bondDescs, const std::vector<NvBlastChunkDesc>& chunkDescs);

    /*
    Saves a Blast object to given path
    */
    bool            saveBlastObject(const std::string& outputDir, const std::string& objectName, const void* object, uint32_t objectTypeID);

private:
    TkFramework*        mFramework;
    ExtSerialization*   mSerialization;
};




#endif