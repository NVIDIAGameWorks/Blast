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
	NV_UNUSED(reader);
	return nullptr;
}

bool PxVec3DTO::deserializeInto(Nv::Blast::Serialization::PxVec3::Reader reader, physx::PxVec3* target)
{
	target->x = reader.getX();
	target->y = reader.getY();
	target->z = reader.getZ();

	return true;
}

}	// namespace Blast
}	// namespace Nv
