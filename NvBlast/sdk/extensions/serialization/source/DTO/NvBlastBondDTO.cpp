/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastBondDTO.h"
#include "NvBlastAssert.h"

namespace Nv
{
	namespace Blast
	{
		
		bool NvBlastBondDTO::serialize(Nv::Blast::Serialization::NvBlastBond::Builder builder, const NvBlastBond * poco)
		{
			NVBLAST_ASSERT(poco != nullptr);
		
			kj::ArrayPtr<const float> normArray(poco->normal, 3);
		
			builder.setNormal(normArray);
		
			builder.setArea(poco->area);
		
			kj::ArrayPtr<const float> centArray(poco->centroid, 3);
		
			builder.setCentroid(centArray);
		
			builder.setUserData(poco->userData);
		
			return true;
		}
		
		NvBlastBond* NvBlastBondDTO::deserialize(Nv::Blast::Serialization::NvBlastBond::Reader reader)
		{
			//FIXME
			reader = reader;
			//TODO: Allocate with ExtContext and return
			return nullptr;
		}
		
		bool NvBlastBondDTO::deserializeInto(Nv::Blast::Serialization::NvBlastBond::Reader reader, NvBlastBond * poco)
		{
			poco->area = reader.getArea();
		
			poco->centroid[0] = reader.getCentroid()[0];
			poco->centroid[1] = reader.getCentroid()[1];
			poco->centroid[2] = reader.getCentroid()[2];
		
			poco->normal[0] = reader.getNormal()[0];
			poco->normal[1] = reader.getNormal()[1];
			poco->normal[2] = reader.getNormal()[2];
		
			poco->userData = reader.getUserData();
		
			return true;
		}
	}
}
