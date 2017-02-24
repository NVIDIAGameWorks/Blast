/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

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
			reader = reader;
			//TODO: Allocate with ExtContext and return
		
			return nullptr;
		}
		
		bool ExtPxChunkDTO::deserializeInto(Nv::Blast::Serialization::ExtPxChunk::Reader reader, Nv::Blast::ExtPxChunk * poco)
		{
			poco->firstSubchunkIndex = reader.getFirstSubchunkIndex();
			poco->subchunkCount = reader.getSubchunkCount();
			poco->isStatic = reader.getIsStatic();
		
			return true;
		}
	}
}