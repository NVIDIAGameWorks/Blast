/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "PxTransformDTO.h"
#include "PxQuatDTO.h"
#include "PxVec3DTO.h"

namespace Nv
{
	namespace Blast
	{
		
		bool PxTransformDTO::serialize(Nv::Blast::Serialization::PxTransform::Builder builder, const physx::PxTransform * poco)
		{
			PxQuatDTO::serialize(builder.getQ(), &poco->q);
			PxVec3DTO::serialize(builder.getP(), &poco->p);
		
			return true;
		}
		
		physx::PxTransform* PxTransformDTO::deserialize(Nv::Blast::Serialization::PxTransform::Reader reader)
		{
			reader = reader;
			return nullptr;
		}
		
		bool PxTransformDTO::deserializeInto(Nv::Blast::Serialization::PxTransform::Reader reader, physx::PxTransform * poco)
		{
			PxQuatDTO::deserializeInto(reader.getQ(), &poco->q);
			PxVec3DTO::deserializeInto(reader.getP(), &poco->p);
		
			return true;
		}
	}
}
