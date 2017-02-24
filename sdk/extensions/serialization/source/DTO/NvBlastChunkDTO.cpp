/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

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
			reader = reader;
		
			return nullptr;
		}
		
		bool NvBlastChunkDTO::deserializeInto(Nv::Blast::Serialization::NvBlastChunk::Reader reader, NvBlastChunk* target)
		{
			NVBLAST_ASSERT(target != nullptr);
		
			target->centroid[0] = reader.getCentroid()[0];
			target->centroid[1] = reader.getCentroid()[1];
			target->centroid[2] = reader.getCentroid()[2];
		
			target->childIndexStop = reader.getChildIndexStop();
			target->firstChildIndex = reader.getFirstChildIndex();
			target->parentChunkIndex = reader.getParentChunkIndex();
			target->userData = reader.getUserData();
			target->volume = reader.getVolume();
		
			return true;
		}
	}
}
