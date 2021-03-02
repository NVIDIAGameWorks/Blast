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


#include "ActorDTO.h"
#include "NvBlastGlobals.h"
#include "NvBlastIDDTO.h"
#include "NvBlastChunkDTO.h"
#include "NvBlastBondDTO.h"


namespace Nv
{
namespace Blast
{

bool ActorDTO::serialize(Nv::Blast::Serialization::Actor::Builder builder, const Nv::Blast::Actor* poco)
{
    builder.setFamilyOffset(poco->getFamilyOffset());
    builder.setFirstVisibleChunkIndex(poco->getFirstVisibleChunkIndex());
    builder.setVisibleChunkCount(poco->getVisibleChunkCount());
    builder.setFirstGraphNodeIndex(poco->getFirstGraphNodeIndex());
    builder.setGraphNodeCount(poco->getGraphNodeCount());
    builder.setLeafChunkCount(poco->getLeafChunkCount());

    return true;
}


Nv::Blast::Actor* ActorDTO::deserialize(Nv::Blast::Serialization::Actor::Reader reader)
{
    NV_UNUSED(reader);
    return nullptr;
}


bool ActorDTO::deserializeInto(Nv::Blast::Serialization::Actor::Reader reader, Nv::Blast::Actor* poco)
{
    poco->setFamilyOffset(reader.getFamilyOffset());
    poco->setFirstVisibleChunkIndex(reader.getFirstVisibleChunkIndex());
    poco->setVisibleChunkCount(reader.getVisibleChunkCount());
    poco->setFirstGraphNodeIndex(reader.getFirstGraphNodeIndex());
    poco->setGraphNodeCount(reader.getGraphNodeCount());
    poco->setLeafChunkCount(reader.getLeafChunkCount());

    return true;
}

}   // namespace Blast
}   // namespace Nv
