/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "PxVec3DTO.h"
#include "NvBlastAssert.h"

namespace Nv
{
	namespace Blast
	{
		bool PxVec3DTO::serialize(Nv::Blast::Serialization::PxVec3::Builder builder, const physx::PxVec3 * poco)
		{
			NVBLAST_ASSERT(poco != nullptr);
		
			builder.setX(poco->x);
			builder.setY(poco->y);
			builder.setZ(poco->z);
		
			return true;
		}
		
		physx::PxVec3* PxVec3DTO::deserialize(Nv::Blast::Serialization::PxVec3::Reader reader)
		{
			//TODO: Allocate using ExtContext and return
			reader = reader;
			return nullptr;
		}
		
		bool PxVec3DTO::deserializeInto(Nv::Blast::Serialization::PxVec3::Reader reader, physx::PxVec3* target)
		{
			target->x = reader.getX();
			target->y = reader.getY();
			target->z = reader.getZ();
		
			return true;
		}
	}
}
