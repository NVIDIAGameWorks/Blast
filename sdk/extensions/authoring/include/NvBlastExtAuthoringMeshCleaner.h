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
// Copyright (c) 2016-2018 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTEXTAUTHORINGMESHCLEANER_H
#define NVBLASTEXTAUTHORINGMESHCLEANER_H

#include "NvBlastExtAuthoringTypes.h"

/**
	FractureTool has requirements to input meshes to fracture them successfully:
		1) Mesh should be closed (watertight)
		2) There should not be self-intersections and open-edges.
*/

/**
	Mesh cleaner input is closed mesh with self-intersections and open-edges (only in the interior). 
	It tries to track outer hull to make input mesh solid and meet requierements of FractureTool. If mesh contained some internal cavities they will be removed.
*/

namespace Nv
{
namespace Blast
{

class Mesh;

class MeshCleaner
{
public:
	virtual ~MeshCleaner() {}

	/**
		Tries to remove self intersections and open edges in interior of mesh.
		\param[in] mesh Mesh to be cleaned.
		\return Cleaned mesh or nullptr if failed.
	*/
	virtual Mesh* cleanMesh(const Mesh* mesh) = 0;

	virtual void release() = 0;
};


} // namespace Blast
} // namespace Nv

#endif // ifndef NVBLASTEXTAUTHORINGMESHCLEANER_H