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
// Copyright (c) 2016-2017 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTAUTHORING_H
#define NVBLASTAUTHORING_H

#include "NvBlastExtAuthoringTypes.h"

namespace physx
{
	class PxCooking;
	class PxPhysicsInsertionCallback;
}

namespace Nv
{
	namespace Blast
	{
		class Mesh;
		class VoronoiSitesGenerator;
		class FractureTool;
		class ConvexMeshBuilder;
		class BlastBondGenerator;
		class MeshCleaner;
	}
}

/**
Constructs mesh object from array of triangles.
User should call release() after usage.

\param[in] positions		Array for vertex positions, 3 * verticesCount floats will be read
\param[in] normals			Array for vertex normals, 3 * verticesCount floats will be read
\param[in] uv				Array for vertex uv coordinates, 2 * verticesCount floats will be read
\param[in] verticesCount	Number of vertices in mesh
\param[in] indices			Array of vertex indices. Indices contain vertex index triplets which form a mesh triangle.
\param[in] indicesCount		Indices count (should be equal to numberOfTriangles * 3)

\return pointer to Nv::Blast::Mesh if it was created succefully otherwise return nullptr
*/
NVBLAST_API Nv::Blast::Mesh* NvBlastExtAuthoringCreateMesh(const physx::PxVec3* positions, const physx::PxVec3* normals,
	const physx::PxVec2* uv, uint32_t verticesCount, const uint32_t* indices, uint32_t indicesCount);

/**
Voronoi sites should not be generated outside of the fractured mesh, so VoronoiSitesGenerator
should be supplied with fracture mesh.
\param[in] mesh			Fracture mesh
\param[in] rnd			User supplied random value generator.
\return					Pointer to VoronoiSitesGenerator. User's code should release it after usage.
*/
NVBLAST_API Nv::Blast::VoronoiSitesGenerator* NvBlastExtAuthoringCreateVoronoiSitesGenerator(Nv::Blast::Mesh* mesh,
	Nv::Blast::RandomGeneratorBase* rng);

/**
Create FractureTool object.
\return Pointer to create FractureTool. User's code should release it after usage.
*/
NVBLAST_API Nv::Blast::FractureTool* NvBlastExtAuthoringCreateFractureTool();

/**
Create BlastBondGenerator
\return Pointer to created BlastBondGenerator. User's code should release it after usage.
*/
NVBLAST_API Nv::Blast::BlastBondGenerator* NvBlastExtAuthoringCreateBondGenerator(physx::PxCooking* cooking, 
	physx::PxPhysicsInsertionCallback* insertionCallback);

/**
Create ConvexMeshBuilder
\return Pointer to created ConvexMeshBuilder. User's code should release it after usage.
*/
NVBLAST_API Nv::Blast::ConvexMeshBuilder* NvBlastExtAuthoringCreateConvexMeshBuilder(physx::PxCooking* cooking,
	physx::PxPhysicsInsertionCallback* insertionCallback);

/**
Performs pending fractures and generates fractured asset, render and collision geometry

\param[in]  fTool				Fracture tool created by NvBlastExtAuthoringCreateFractureTool
\param[in]  bondGenerator		Bond generator created by NvBlastExtAuthoringCreateBondGenerator
\param[in]  collisionBuilder	Collision builder created by NvBlastExtAuthoringCreateConvexMeshBuilder
\param[in]  defaultSupportDepth All new chunks will be marked as support if its depth equal to defaultSupportDepth. 
								By default leaves (chunks without children) marked as support.
\return		Authoring result
*/
NVBLAST_API Nv::Blast::AuthoringResult* NvBlastExtAuthoringProcessFracture(Nv::Blast::FractureTool& fTool,
	Nv::Blast::BlastBondGenerator& bondGenerator, Nv::Blast::ConvexMeshBuilder& collisionBuilder, int32_t defaultSupportDepth = -1);


/**
	Creates MeshCleaner object
	\return pointer to Nv::Blast::Mesh if it was created succefully otherwise return nullptr
*/
NVBLAST_API Nv::Blast::MeshCleaner* NvBlastExtAuthoringCreateMeshCleaner();

#endif // ifndef NVBLASTAUTHORING_H
