/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef CONVEX_RENDER_MESH_H
#define CONVEX_RENDER_MESH_H

#include "CustomRenderMesh.h"

namespace physx
{
class PxConvexMesh;
}


/**
PxConvexMesh render mesh 
(this class relates to PhysX more then to Renderer)
*/
class ConvexRenderMesh : public CustomRenderMesh
{
public:
	ConvexRenderMesh(const PxConvexMesh* mesh);
	virtual ~ConvexRenderMesh();
};


#endif //CONVEX_RENDER_MESH_H