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


#include "ExtPxChunkDTO.h"

namespace Nv
{
namespace Blast
{

bool ExtPxChunkDTO::serialize(Nv::Blast::Serialization::ExtPxChunk::Builder builder, const Nv::Blast::ExtPxChunk * poco)
{
	builder.setFirstSubchunkIndex(poco->firstSubchunkIndex);
	builder.setSubchunkCount(poco->subchunkCount);
	builder.setIsStatic(poco->isStatic);

	return true;
}


Nv::Blast::ExtPxChunk* ExtPxChunkDTO::deserialize(Nv::Blast::Serialization::ExtPxChunk::Reader reader)
{
	NV_UNUSED(reader);
	// TODO: Allocate with ExtContext and return

	return nullptr;
}


bool ExtPxChunkDTO::deserializeInto(Nv::Blast::Serialization::ExtPxChunk::Reader reader, Nv::Blast::ExtPxChunk * poco)
{
	poco->firstSubchunkIndex = reader.getFirstSubchunkIndex();
	poco->subchunkCount = reader.getSubchunkCount();
	poco->isStatic = reader.getIsStatic();

	return true;
}

}	// namespace Blast
}	// namespace Nv
