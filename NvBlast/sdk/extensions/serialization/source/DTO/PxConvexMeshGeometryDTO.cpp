/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "PxConvexMeshGeometryDTO.h"
#include "PxMeshScaleDTO.h"
#include "NvBlastAssert.h"
#include "NvBlastExtKJPxInputStream.h"
#include "NvBlastExtKJPxOutputStream.h"
#include "PxConvexMeshDesc.h"
#include "NvBlastExtSerialization.h"
#include "PxVec3.h"
#include <algorithm>
#include "PxPhysics.h"


namespace Nv
{
	namespace Blast
	{
		physx::PxCooking* PxConvexMeshGeometryDTO::Cooking = nullptr;
		physx::PxPhysics* PxConvexMeshGeometryDTO::Physics = nullptr;

		bool PxConvexMeshGeometryDTO::serialize(Nv::Blast::Serialization::PxConvexMeshGeometry::Builder builder, const physx::PxConvexMeshGeometry * poco)
		{
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
		
		
			std::vector<unsigned char> buffer;
			buffer.resize(16 * 1024 * 1024); // No idea how much memory is needed! Allocate 16MB
			kj::ArrayPtr<unsigned char> bufferArray(buffer.data(), buffer.size());
		
			Nv::Blast::ExtKJPxOutputStream outputStream(bufferArray);
		
			bool cookResult = Cooking->cookConvexMesh(desc, outputStream);
		
			if (!cookResult)
			{
				return false;
			}
		
			kj::ArrayPtr<unsigned char> cookedBuffer(outputStream.getBuffer().begin(), outputStream.getWrittenBytes());
		
			builder.setConvexMesh(cookedBuffer);
		
		//	builder.getConvexMesh().
		
			return true;
		}
		
		physx::PxConvexMeshGeometry* PxConvexMeshGeometryDTO::deserialize(Nv::Blast::Serialization::PxConvexMeshGeometry::Reader reader)
		{
			NVBLAST_ASSERT(PxConvexMeshGeometryDTO::Cooking != nullptr);
		
			reader = reader;
		
			return nullptr;
		}
		
		bool PxConvexMeshGeometryDTO::deserializeInto(Nv::Blast::Serialization::PxConvexMeshGeometry::Reader reader, physx::PxConvexMeshGeometry * poco)
		{
			NVBLAST_ASSERT(PxConvexMeshGeometryDTO::Cooking != nullptr);
		
			PxMeshScaleDTO::deserializeInto(reader.getScale(), &poco->scale);
		
			Nv::Blast::ExtKJPxInputStream inputStream(reader.getConvexMesh());
		
			//NOTE: Naive approach, no shared convex hulls
			poco->convexMesh = Physics->createConvexMesh(inputStream);
		
			return false;
		}
		
		
		
	}
}
