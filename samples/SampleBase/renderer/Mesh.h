/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef MESH_H
#define MESH_H

#include <vector>
#include "PxVec2.h"
#include "PxVec3.h"


class Mesh
{
	virtual uint32_t getVertexStride() = 0;
	// ... TBD
};

/**
SimpleMesh: position + normal + uv
We use only this type everywhere, once other versions will be required we should generalize Mesh and refactor code.
*/
class SimpleMesh : public Mesh
{
public:

	class Vertex
	{
	public:
		physx::PxVec3 position;
		physx::PxVec3 normal;
		physx::PxVec2 uv;
	};

	virtual uint32_t getVertexStride() { return sizeof(Vertex); }

	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;

	physx::PxVec3 extents;
	physx::PxVec3 center;
};


#endif //MESH_H