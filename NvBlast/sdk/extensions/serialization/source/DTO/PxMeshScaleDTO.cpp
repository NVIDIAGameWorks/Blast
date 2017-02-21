/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "PxMeshScaleDTO.h"
#include "PxVec3DTO.h"
#include "PxQuatDTO.h"

namespace Nv
{
	namespace Blast
	{
		bool PxMeshScaleDTO::serialize(Nv::Blast::Serialization::PxMeshScale::Builder builder, const physx::PxMeshScale * poco)
		{
			PxVec3DTO::serialize(builder.getScale(), &poco->scale);
			PxQuatDTO::serialize(builder.getRotation(), &poco->rotation);
		
			return true;
		}
		
		physx::PxMeshScale* PxMeshScaleDTO::deserialize(Nv::Blast::Serialization::PxMeshScale::Reader reader)
		{
			reader = reader;
			return nullptr;
		}
		
		bool PxMeshScaleDTO::deserializeInto(Nv::Blast::Serialization::PxMeshScale::Reader reader, physx::PxMeshScale * poco)
		{
			PxVec3DTO::deserializeInto(reader.getScale(), &poco->scale);
			PxQuatDTO::deserializeInto(reader.getRotation(), &poco->rotation);
		
			return true;
		}
	}
}

