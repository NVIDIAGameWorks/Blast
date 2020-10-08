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


#include "NvBlastChunkDTO.h"
#include "NvBlastAssert.h"

namespace Nv
{
namespace Blast
{

bool NvBlastChunkDTO::serialize(Nv::Blast::Serialization::NvBlastChunk::Builder builder, const NvBlastChunk* poco)
{
	NVBLAST_ASSERT(poco != nullptr);

	kj::ArrayPtr<const float> centArray(poco->centroid, 3);
	builder.setCentroid(centArray);

	builder.setVolume(poco->volume);

	builder.setParentChunkIndex(poco->parentChunkIndex);
	builder.setFirstChildIndex(poco->firstChildIndex);
	builder.setChildIndexStop(poco->childIndexStop);
	builder.setUserData(poco->userData);

	return true;
}


NvBlastChunk* NvBlastChunkDTO::deserialize(Nv::Blast::Serialization::NvBlastChunk::Reader reader)
{
	//FIXME
	NV_UNUSED(reader);

	return nullptr;
}


bool NvBlastChunkDTO::deserializeInto(Nv::Blast::Serialization::NvBlastChunk::Reader reader, NvBlastChunk* target)
{
	NVBLAST_ASSERT(target != nullptr);

	auto readerCentroid = reader.getCentroid();
	target->centroid[0] = readerCentroid[0];
	target->centroid[1] = readerCentroid[1];
	target->centroid[2] = readerCentroid[2];

	target->childIndexStop = reader.getChildIndexStop();
	target->firstChildIndex = reader.getFirstChildIndex();
	target->parentChunkIndex = reader.getParentChunkIndex();
	target->userData = reader.getUserData();
	target->volume = reader.getVolume();

	return true;
}

}	// namespace Blast
}	// namespace Nv
