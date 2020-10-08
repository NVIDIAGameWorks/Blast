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


#ifndef NVBLASTEXTEXPORTERJSONCOLLISION_H
#define NVBLASTEXTEXPORTERJSONCOLLISION_H

#include "NvBlastTypes.h"

namespace Nv
{
namespace Blast
{

struct CollisionHull;

/**
	Interface to object which serializes collision geometry to JSON format. 
*/
class IJsonCollisionExporter
{
public: 
	/**
		Delete this object
	*/
	virtual void	release() = 0;

	/**
		Method creates file with given path and serializes given array of arrays of convex hulls to it in JSON format.
		\param[in] path			Output file path.
		\param[in] chunkCount	The number of chunks, may be less than the number of collision hulls.
		\param[in] hullOffsets	Collision hull offsets. Contains chunkCount + 1 element. First collision hull for i-th chunk: hull[hullOffsets[i]]. hullOffsets[chunkCount+1] is total number of hulls.
		\param[in] hulls		Array of pointers to convex hull descriptors, contiguously grouped for chunk[0], chunk[1], etc.
	*/
	virtual bool	writeCollision(const char* path, uint32_t chunkCount, const uint32_t* hullOffsets, const CollisionHull* const * hulls) = 0;
};

} // namespace Blast
} // namespace Nv


/**
Creates an instance of IMeshFileWriter for writing obj file.
*/
NVBLAST_API Nv::Blast::IJsonCollisionExporter* NvBlastExtExporterCreateJsonCollisionExporter();


#endif //NVBLASTEXTEXPORTERJSONCOLLISION_H
