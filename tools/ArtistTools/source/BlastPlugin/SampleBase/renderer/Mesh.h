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
// Copyright (c) 2008-2017 NVIDIA Corporation. All rights reserved.


#ifndef MESH_H
#define MESH_H

#include <vector>
#include "PxVec2.h"
#include "PxVec3.h"

// Add By Lixu Begin
class MeshBase
// Add By Lixu End
{
	virtual uint32_t getVertexStride() = 0;
	// ... TBD
};

/**
SimpleMesh: position + normal + uv
We use only this type everywhere, once other versions will be required we should generalize Mesh and refactor code.
*/
// Add By Lixu Begin
class SimpleMesh : public MeshBase
// Add By Lixu End
{
public:

	class Vertex
	{
	public:
		physx::PxVec3 position;
		physx::PxVec3 normal;
		physx::PxVec3 facenormal;
		physx::PxVec3 tangent;
		physx::PxVec2 uv;
	};

	virtual uint32_t getVertexStride() { return sizeof(Vertex); }

	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;

	physx::PxVec3 extents;
	physx::PxVec3 center;
};


#endif //MESH_H