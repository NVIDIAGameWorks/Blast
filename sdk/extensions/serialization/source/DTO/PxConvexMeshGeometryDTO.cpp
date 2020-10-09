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


#include "PxConvexMeshGeometryDTO.h"
#include "PxMeshScaleDTO.h"
#include "NvBlastAssert.h"
#include "NvBlastExtKJPxInputStream.h"
#include "NvBlastExtKJPxOutputStream.h"
#include "PxConvexMeshDesc.h"
#include "NvBlastExtSerialization.h"
#include "PxVec3.h"
#include <algorithm>
#include <vector>
#include "PxPhysics.h"
#include "NvBlastPxCallbacks.h"
#include "PxDefaultStreams.h"


namespace Nv
{
namespace Blast
{

extern physx::PxPhysics* sExtPxSerializerPhysics;
extern physx::PxCooking* sExtPxSerializerCooking;


bool PxConvexMeshGeometryDTO::serialize(Nv::Blast::Serialization::PxConvexMeshGeometry::Builder builder, const physx::PxConvexMeshGeometry * poco)
{
	NVBLAST_ASSERT(sExtPxSerializerCooking != nullptr);

	PxMeshScaleDTO::serialize(builder.getScale(), &poco->scale);

	//TODO: Use cooking.cookConvexMesh to cook the mesh to a stream - then get that backing buffer and put it into the Data field

	physx::PxConvexMeshDesc desc;
	desc.points.data = poco->convexMesh->getVertices();
	desc.points.count = poco->convexMesh->getNbVertices();
	desc.points.stride = sizeof(physx::PxVec3);

	std::vector<uint32_t> indicesScratch;
	std::vector<physx::PxHullPolygon> hullPolygonsScratch;

	hullPolygonsScratch.resize(poco->convexMesh->getNbPolygons());

	uint32_t indexCount = 0;
	for (uint32_t i = 0; i < hullPolygonsScratch.size(); i++)
	{
		physx::PxHullPolygon polygon;
		poco->convexMesh->getPolygonData(i, polygon);
		if (polygon.mNbVerts)
		{
			indexCount = std::max<uint32_t>(indexCount, polygon.mIndexBase + polygon.mNbVerts);
		}
	}
	indicesScratch.resize(indexCount);

	for (uint32_t i = 0; i < hullPolygonsScratch.size(); i++)
	{
		physx::PxHullPolygon polygon;
		poco->convexMesh->getPolygonData(i, polygon);
		for (uint32_t j = 0; j < polygon.mNbVerts; j++)
		{
			indicesScratch[polygon.mIndexBase + j] = poco->convexMesh->getIndexBuffer()[polygon.mIndexBase + j];
		}

		hullPolygonsScratch[i] = polygon;
	}

	desc.indices.count = indexCount;
	desc.indices.data = indicesScratch.data();
	desc.indices.stride = sizeof(uint32_t);

	desc.polygons.count = poco->convexMesh->getNbPolygons();
	desc.polygons.data = hullPolygonsScratch.data();
	desc.polygons.stride = sizeof(physx::PxHullPolygon);
		
	physx::PxDefaultMemoryOutputStream outStream(NvBlastGetPxAllocatorCallback());
	if (!sExtPxSerializerCooking->cookConvexMesh(desc, outStream))
	{
		return false;
	}

	kj::ArrayPtr<unsigned char> cookedBuffer(outStream.getData(), outStream.getSize());

	builder.setConvexMesh(cookedBuffer);

	return true;
}


physx::PxConvexMeshGeometry* PxConvexMeshGeometryDTO::deserialize(Nv::Blast::Serialization::PxConvexMeshGeometry::Reader reader)
{
	NVBLAST_ASSERT(sExtPxSerializerCooking != nullptr);

	NV_UNUSED(reader);

	return nullptr;
}


bool PxConvexMeshGeometryDTO::deserializeInto(Nv::Blast::Serialization::PxConvexMeshGeometry::Reader reader, physx::PxConvexMeshGeometry * poco)
{
	NVBLAST_ASSERT(sExtPxSerializerPhysics != nullptr);

	PxMeshScaleDTO::deserializeInto(reader.getScale(), &poco->scale);

	Nv::Blast::ExtKJPxInputStream inputStream(reader.getConvexMesh());

	//NOTE: Naive approach, no shared convex hulls
	poco->convexMesh = sExtPxSerializerPhysics->createConvexMesh(inputStream);

	return poco->convexMesh != nullptr;
}

}	// namespace Blast
}	// namespace Nv
