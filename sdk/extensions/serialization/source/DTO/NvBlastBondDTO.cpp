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
	NV_UNUSED(reader);
	//TODO: Allocate with ExtContext and return
	return nullptr;
}


bool NvBlastBondDTO::deserializeInto(Nv::Blast::Serialization::NvBlastBond::Reader reader, NvBlastBond * poco)
{
	poco->area = reader.getArea();

	auto readerCentroid = reader.getCentroid();
	poco->centroid[0] = readerCentroid[0];
	poco->centroid[1] = readerCentroid[1];
	poco->centroid[2] = readerCentroid[2];

	auto readerNormal = reader.getNormal();
	poco->normal[0] = readerNormal[0];
	poco->normal[1] = readerNormal[1];
	poco->normal[2] = readerNormal[2];

	poco->userData = reader.getUserData();

	return true;
}

}	// namespace Blast
}	// namespace Nv
