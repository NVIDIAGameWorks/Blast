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
// Copyright (c) 2020 NVIDIA Corporation. All rights reserved.


#include "FamilyDTO.h"

#include "ActorDTO.h"
#include "AssetDTO.h"
#include "FamilyGraphDTO.h"
#include "NvBlastFamilyGraph.h"
#include "NvBlastGlobals.h"
#include "NvBlastIDDTO.h"
#include "NvBlastChunkDTO.h"
#include "NvBlastBondDTO.h"

#include <vector>

namespace Nv
{
namespace Blast
{

bool FamilyDTO::serialize(Nv::Blast::Serialization::Family::Builder builder, const Nv::Blast::FamilyHeader* poco)
{
    NvBlastIDDTO::serialize(builder.initAssetID(), &poco->m_assetID);

    // cache off the count data from the asset needed to re-create the family post serialization
    const NvBlastAssetMemSizeData sizeData = NvBlastAssetMemSizeDataFromAsset(poco->m_asset);
    builder.setBondCount(sizeData.bondCount);
    builder.setChunkCount(sizeData.chunkCount);
    builder.setNodeCount(sizeData.nodeCount);
    builder.setLowerSupportChunkCount(sizeData.lowerSupportChunkCount);
    builder.setUpperSupportChunkCount(sizeData.upperSupportChunkCount);

    // actorCount - these are active
    builder.setActorCount(poco->m_actorCount);

    // all possible actors
    const uint32_t actorCount = poco->getActorsArraySize();
    capnp::List<Nv::Blast::Serialization::Actor>::Builder actors = builder.initActors(actorCount);
    for (uint32_t i = 0; i < actorCount; i++)
    {
        Actor& actor = poco->getActors()[i];
        ActorDTO::serialize(actors[i], &actor);
    }

    // visibleChunkIndexLinks
    uint32_t* visibleChunkIndexLinks = reinterpret_cast<uint32_t *>(poco->getVisibleChunkIndexLinks());
    kj::ArrayPtr<uint32_t> visibleChunkIndexLinksArray(visibleChunkIndexLinks, sizeData.chunkCount * 2);
    builder.setVisibleChunkIndexLinks(visibleChunkIndexLinksArray);

    // chunkActorIndices
    kj::ArrayPtr<uint32_t> chunkActorIndicesArray(poco->getChunkActorIndices(), sizeData.chunkCount);
    builder.setChunkActorIndices(chunkActorIndicesArray);

    // graphNodeIndexLinks
    kj::ArrayPtr<uint32_t> graphNodeIndexLinksArray(poco->getGraphNodeIndexLinks(), sizeData.chunkCount);
    builder.setGraphNodeIndexLinks(graphNodeIndexLinksArray);

    // lowerSupportChunkHealths
    kj::ArrayPtr<float> lowerSupportChunkHealthsArray(poco->getLowerSupportChunkHealths(), sizeData.chunkCount);
    builder.setLowerSupportChunkHealths(lowerSupportChunkHealthsArray);

    // graphBondHealths
    kj::ArrayPtr<float> graphBondHealthsArray(poco->getBondHealths(), sizeData.bondCount);
    builder.setGraphBondHealths(graphBondHealthsArray);

    // familyGraph
    FamilyGraph *graph = poco->getFamilyGraph();
    auto builderGraph = builder.initFamilyGraph();
    builderGraph.setNodeCount(sizeData.nodeCount);
    FamilyGraphDTO::serialize(builderGraph, graph);

    return true;
}


Nv::Blast::FamilyHeader* FamilyDTO::deserialize(Nv::Blast::Serialization::Family::Reader reader)
{
    // fill in the count info from the reader
    NvBlastAssetMemSizeData sizeData;
    sizeData.bondCount = reader.getBondCount();
    sizeData.chunkCount = reader.getChunkCount();
    sizeData.nodeCount = reader.getNodeCount();
    sizeData.lowerSupportChunkCount = reader.getLowerSupportChunkCount();
    sizeData.upperSupportChunkCount = reader.getUpperSupportChunkCount();

    // allocate enough space to hold the family
    const size_t familySize = NvBlastAssetGetFamilyMemorySizeFromSizeData(sizeData, nullptr);
    void* mem = NVBLAST_ALLOC(familySize);

    // use the count info to initialize the family
    auto family = reinterpret_cast<Nv::Blast::FamilyHeader *>(NvBlastAssetCreateFamilyFromSizeData(mem, sizeData, Nv::Blast::logLL));

    // then fill in the data from the reader
    if (deserializeInto(reader, family))
        return family;

    // failed to deserialize, free the allocated memory so it doesn't leak
    NVBLAST_FREE(mem);
    return nullptr;
}


bool FamilyDTO::deserializeInto(Nv::Blast::Serialization::Family::Reader reader, Nv::Blast::FamilyHeader* poco)
{
    NvBlastIDDTO::deserializeInto(reader.getAssetID(), &poco->m_assetID);

    // active actor count
    poco->m_actorCount = reader.getActorCount();

    // all possible actors
    Actor* actors = poco->getActors();
    auto readerActors = reader.getActors();
    NVBLAST_ASSERT(poco->m_actorCount <= readerActors.size());
    for (uint32_t i = 0; i < readerActors.size(); i++)
    {
        auto actorReader = readerActors[i];
        ActorDTO::deserializeInto(actorReader, &actors[i]);
    }

    // visibleChunkIndexLinks
    // they are stored in the buffer as a flat list of uint32_t values,
    // but stored as pairs in the Family
    auto readerVisibleChunkIndexLinks = reader.getVisibleChunkIndexLinks();
    const uint32_t numVisibleChunkIndexLinks = readerVisibleChunkIndexLinks.size();
    for (uint32_t i = 0; i < numVisibleChunkIndexLinks; i += 2)
    {
        const uint32_t vcil = i / 2;
        poco->getVisibleChunkIndexLinks()[vcil].m_adj[0] = readerVisibleChunkIndexLinks[i];
        poco->getVisibleChunkIndexLinks()[vcil].m_adj[1] = readerVisibleChunkIndexLinks[i+1];
    }

    // chunkActorIndices
    auto readerChunkActorIndices = reader.getChunkActorIndices();
    const uint32_t numChunkActorIndices = readerChunkActorIndices.size();
    for (uint32_t i = 0; i < numChunkActorIndices; i++)
    {
        poco->getChunkActorIndices()[i] = readerChunkActorIndices[i];
    }

    // graphNodeIndexLinks
    auto readerGraphNodeIndexLinks = reader.getGraphNodeIndexLinks();
    const uint32_t numGraphNodeIndexLinks = readerGraphNodeIndexLinks.size();
    for (uint32_t i = 0; i < numGraphNodeIndexLinks; i++)
    {
        poco->getGraphNodeIndexLinks()[i] = readerGraphNodeIndexLinks[i];
    }

    // lowerSupportChunkHealths
    auto readerLowerSupportChunkHealths = reader.getLowerSupportChunkHealths();
    const uint32_t numLowerSupportChunkHealths = readerLowerSupportChunkHealths.size();
    for (uint32_t i = 0; i < numLowerSupportChunkHealths; i++)
    {
        poco->getLowerSupportChunkHealths()[i] = readerLowerSupportChunkHealths[i];
    }

    // graphBondHealths
    auto readerGraphBondHealths = reader.getGraphBondHealths();
    const uint32_t numGraphBondHealths = readerGraphBondHealths.size();
    for (uint32_t i = 0; i < numGraphBondHealths; i++)
    {
        poco->getBondHealths()[i] = readerGraphBondHealths[i];
    }

    // familyGraph
    FamilyGraphDTO::deserializeInto(reader.getFamilyGraph(), poco->getFamilyGraph());

    return true;
}

}   // namespace Blast
}   // namespace Nv
