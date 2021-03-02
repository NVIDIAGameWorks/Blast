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


#include "FamilyGraphDTO.h"
#include "NvBlastGlobals.h"

namespace Nv
{
namespace Blast
{

bool FamilyGraphDTO::serialize(Nv::Blast::Serialization::FamilyGraph::Builder builder, const Nv::Blast::FamilyGraph * poco)
{
    // this needs to be set externally so we have access to it here
    const uint32_t nodeCount = builder.getNodeCount();

    kj::ArrayPtr<IslandId> islandIdsArray(poco->getIslandIds(), nodeCount);
    builder.setIslandIds(islandIdsArray);

    kj::ArrayPtr<NodeIndex> dirtyNodeLinksArray(poco->getDirtyNodeLinks(), nodeCount);
    builder.setDirtyNodeLinks(dirtyNodeLinksArray);

    kj::ArrayPtr<uint32_t> firstDirtyNodeIndicesArray(poco->getFirstDirtyNodeIndices(), nodeCount);
    builder.setFirstDirtyNodeIndices(firstDirtyNodeIndicesArray);

    kj::ArrayPtr<NodeIndex> fastRouteArray(poco->getFastRoute(), nodeCount);
    builder.setFastRoute(fastRouteArray);

    kj::ArrayPtr<uint32_t> hopCountsArray(poco->getHopCounts(), nodeCount);
    builder.setHopCounts(hopCountsArray);

    auto isEdgeRemoved = poco->getIsEdgeRemoved();
    uint8_t* isEdgeRemovedData = reinterpret_cast<uint8_t*>(const_cast<char*>(isEdgeRemoved->getData()));
    capnp::Data::Reader isEdgeRemovedReader(isEdgeRemovedData, isEdgeRemoved->getSize());
    builder.setIsEdgeRemoved(isEdgeRemovedReader);


    auto isNodeInDirtyList = poco->getIsNodeInDirtyList();
    uint8_t* isNodeInDirtyListData = reinterpret_cast<uint8_t*>(const_cast<char*>(isNodeInDirtyList->getData()));
    capnp::Data::Reader isNodeInDirtyListReader(isNodeInDirtyListData, isNodeInDirtyList->getSize());
    builder.setIsNodeInDirtyList(isNodeInDirtyListReader);

    return true;
}


Nv::Blast::FamilyGraph* FamilyGraphDTO::deserialize(Nv::Blast::Serialization::FamilyGraph::Reader reader)
{
    NV_UNUSED(reader);
    return nullptr;
}


bool FamilyGraphDTO::deserializeInto(Nv::Blast::Serialization::FamilyGraph::Reader reader, Nv::Blast::FamilyGraph * poco)
{
    auto readerIslandIds = reader.getIslandIds();
    const uint32_t numIslandIds = readerIslandIds.size();
    for (uint32_t i = 0; i < numIslandIds; i++)
    {
        poco->getIslandIds()[i] = readerIslandIds[i];
    }

    auto readerDirtyNodeLinks = reader.getDirtyNodeLinks();
    const uint32_t numDirtyNodeLinks = readerDirtyNodeLinks.size();
    for (uint32_t i = 0; i < numDirtyNodeLinks; i++)
    {
        poco->getDirtyNodeLinks()[i] = readerDirtyNodeLinks[i];
    }

    auto readerFirstDirtyNodeIndices = reader.getFirstDirtyNodeIndices();
    const uint32_t numFirstDirtyNodeIndices = readerFirstDirtyNodeIndices.size();
    for (uint32_t i = 0; i < numFirstDirtyNodeIndices; i++)
    {
        poco->getFirstDirtyNodeIndices()[i] = readerFirstDirtyNodeIndices[i];
    }

    auto readerFastRoute = reader.getFastRoute();
    const uint32_t numFastRoute = readerFastRoute.size();
    for (uint32_t i = 0; i < numFastRoute; i++)
    {
        poco->getFastRoute()[i] = readerFastRoute[i];
    }

    auto readerHopCounts = reader.getHopCounts();
    const uint32_t numHopCounts = readerHopCounts.size();
    for (uint32_t i = 0; i < numHopCounts; i++)
    {
        poco->getHopCounts()[i] = readerHopCounts[i];
    }

    auto readerIsEdgeRemoved = reader.getIsEdgeRemoved();
    const uint32_t numIsEdgeRemoved = readerIsEdgeRemoved.size();
    const char* isEdgeRemovedData = reinterpret_cast<const char*>(readerIsEdgeRemoved.begin());
    auto isEdgeRemoved = poco->getIsEdgeRemoved();
    isEdgeRemoved->setData(isEdgeRemovedData, numIsEdgeRemoved);

    auto readerIsNodeInDirtyList = reader.getIsNodeInDirtyList();
    const uint32_t numIsNodeInDirtyList = readerIsNodeInDirtyList.size();
    const char* readerIsNodeInDirtyListData = reinterpret_cast<const char*>(readerIsNodeInDirtyList.begin());
    auto isNodeInDirtyList = poco->getIsNodeInDirtyList();
    isNodeInDirtyList->setData(readerIsNodeInDirtyListData, numIsNodeInDirtyList);

    return true;
}

}   // namespace Blast
}   // namespace Nv
