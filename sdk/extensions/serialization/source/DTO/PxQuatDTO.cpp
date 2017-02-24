/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "PxQuatDTO.h"

namespace Nv
{
	namespace Blast
	{
		
		bool PxQuatDTO::serialize(Nv::Blast::Serialization::PxQuat::Builder builder, const physx::PxQuat * poco)
		{
			builder.setX(poco->x);
			builder.setY(poco->y);
			builder.setZ(poco->z);
			builder.setW(poco->w);
		
			return true;
		}
		
		physx::PxQuat* PxQuatDTO::deserialize(Nv::Blast::Serialization::PxQuat::Reader reader)
		{
			reader = reader;
			return nullptr;
		}
		
		bool PxQuatDTO::deserializeInto(Nv::Blast::Serialization::PxQuat::Reader reader, physx::PxQuat * poco)
		{
			poco->x = reader.getX();
			poco->y = reader.getY();
			poco->z = reader.getZ();
			poco->w = reader.getW();
		
			return true;
		}
		
	}
}
