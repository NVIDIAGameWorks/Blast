/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

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
		
			reader = reader;
		
			return nullptr;
		}
		
		bool TkAssetJointDescDTO::deserializeInto(Nv::Blast::Serialization::TkAssetJointDesc::Reader reader, Nv::Blast::TkAssetJointDesc * poco)
		{
			PxVec3DTO::deserializeInto(reader.getAttachPositions()[0], &poco->attachPositions[0]);
			PxVec3DTO::deserializeInto(reader.getAttachPositions()[1], &poco->attachPositions[1]);
		
			poco->nodeIndices[0] = reader.getNodeIndices()[0];
			poco->nodeIndices[1] = reader.getNodeIndices()[1];
		
			return true;
		}
	}
}