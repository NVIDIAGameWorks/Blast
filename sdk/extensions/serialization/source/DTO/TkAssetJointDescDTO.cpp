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


#include "TkAssetJointDescDTO.h"
#include "PxVec3DTO.h"


namespace Nv
{
namespace Blast
{

bool TkAssetJointDescDTO::serialize(Nv::Blast::Serialization::TkAssetJointDesc::Builder builder, const Nv::Blast::TkAssetJointDesc * poco)
{
	kj::ArrayPtr<const uint32_t> nodeIndices(poco->nodeIndices, 2);
	builder.setNodeIndices(nodeIndices);

	for (int i = 0; i < 2; i++)
	{
		PxVec3DTO::serialize(builder.getAttachPositions()[i], &poco->attachPositions[i]);
	}

	return true;
}


Nv::Blast::TkAssetJointDesc* TkAssetJointDescDTO::deserialize(Nv::Blast::Serialization::TkAssetJointDesc::Reader reader)
{
	//TODO: Allocate with ExtContent and return

	NV_UNUSED(reader);

	return nullptr;
}


bool TkAssetJointDescDTO::deserializeInto(Nv::Blast::Serialization::TkAssetJointDesc::Reader reader, Nv::Blast::TkAssetJointDesc * poco)
{
	auto readerAttachPositions = reader.getAttachPositions();
	PxVec3DTO::deserializeInto(readerAttachPositions[0], &poco->attachPositions[0]);
	PxVec3DTO::deserializeInto(readerAttachPositions[1], &poco->attachPositions[1]);

	auto readerNodeIndices = reader.getNodeIndices();
	poco->nodeIndices[0] = readerNodeIndices[0];
	poco->nodeIndices[1] = readerNodeIndices[1];

	return true;
}

}	// namespace Blast
}	// namespace Nv
