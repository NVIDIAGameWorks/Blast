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


#include "NvPreprocessor.h"
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
	NV_UNUSED(reader);
	return nullptr;
}


bool PxMeshScaleDTO::deserializeInto(Nv::Blast::Serialization::PxMeshScale::Reader reader, physx::PxMeshScale * poco)
{
	PxVec3DTO::deserializeInto(reader.getScale(), &poco->scale);
	PxQuatDTO::deserializeInto(reader.getRotation(), &poco->rotation);

	return true;
}

}	// namespace Blast
}	// namespace Nv
