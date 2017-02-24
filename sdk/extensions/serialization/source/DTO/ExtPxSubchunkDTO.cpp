/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "ExtPxSubchunkDTO.h"
#include "PxTransformDTO.h"
#include "PxConvexMeshGeometryDTO.h"

namespace Nv
{
	namespace Blast
	{
		bool ExtPxSubchunkDTO::serialize(Nv::Blast::Serialization::ExtPxSubchunk::Builder builder, const Nv::Blast::ExtPxSubchunk * poco)
		{
			PxTransformDTO::serialize(builder.getTransform(), &poco->transform);
			PxConvexMeshGeometryDTO::serialize(builder.getGeometry(), &poco->geometry);
		
			return true;
		}
		
		Nv::Blast::ExtPxSubchunk* ExtPxSubchunkDTO::deserialize(Nv::Blast::Serialization::ExtPxSubchunk::Reader reader)
		{
			reader = reader;
			//TODO: Allocate with ExtContext and return
		
			return nullptr;
		}
		
		bool ExtPxSubchunkDTO::deserializeInto(Nv::Blast::Serialization::ExtPxSubchunk::Reader reader, Nv::Blast::ExtPxSubchunk * poco)
		{
			PxTransformDTO::deserializeInto(reader.getTransform(), &poco->transform);
		
			return true;
		}
		
	}
}
