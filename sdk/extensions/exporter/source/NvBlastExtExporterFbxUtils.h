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
// Copyright (c) 2017 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTEXTEXPORTERFBXUTILS_H
#define NVBLASTEXTEXPORTERFBXUTILS_H

#include "fbxsdk.h"
#include "PxVec3.h"
#include "PxVec2.h"
#include <string>

namespace Nv
{
	namespace Blast
	{
		struct Vertex;
	}
}

class FbxUtils
{
public:
	static void VertexToFbx(const Nv::Blast::Vertex& vert, FbxVector4& outVertex, FbxVector4& outNormal, FbxVector2& outUV);

	static void PxVec3ToFbx(const physx::PxVec3& inVector, FbxVector4& outVector);
	static void PxVec2ToFbx(const physx::PxVec2& inVector, FbxVector2& outVector);

	static FbxAxisSystem getBlastFBXAxisSystem();
	static FbxSystemUnit getBlastFBXUnit();

	static std::string FbxAxisSystemToString(const FbxAxisSystem& axisSystem);
	static std::string FbxSystemUnitToString(const FbxSystemUnit& systemUnit);

	//returns UINT32_MAX if not a chunk
	static uint32_t getChunkIndexForNode(FbxNode* node, uint32_t* outParentChunkIndex = nullptr);
	//Search using the old naming 
	static uint32_t getChunkIndexForNodeBackwardsCompatible(FbxNode* node, uint32_t* outParentChunkIndex = nullptr);
	static std::string getChunkNodeName(uint32_t chunkIndex);

	static std::string getCollisionGeometryLayerName();
	static std::string getRenderGeometryLayerName();
};

#endif //NVBLASTEXTEXPORTERFBXUTILS_H