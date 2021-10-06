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


#include "fbxsdk.h"
#include "NvBlastExtExporterFbxUtils.h"
#include "NvBlastExtAuthoringTypes.h"
#include <sstream>
#include <cctype>

void FbxUtils::VertexToFbx(const Nv::Blast::Vertex& vert, FbxVector4& outVertex, FbxVector4& outNormal, FbxVector2& outUV)
{
	NvcVec3ToFbx(vert.p, outVertex);
	NvcVec3ToFbx(vert.n, outNormal);
	NvcVec2ToFbx(vert.uv[0], outUV);
}

void FbxUtils::NvcVec3ToFbx(const NvcVec3& inVector, FbxVector4& outVector)
{
	outVector[0] = inVector.x;
	outVector[1] = inVector.y;
	outVector[2] = inVector.z;
	outVector[3] = 0;
}

void FbxUtils::NvcVec2ToFbx(const NvcVec2& inVector, FbxVector2& outVector)
{
	outVector[0] = inVector.x;
	outVector[1] = inVector.y;
}

FbxAxisSystem FbxUtils::getBlastFBXAxisSystem()
{
	const FbxAxisSystem::EUpVector upVector = FbxAxisSystem::eZAxis;
	//From the documentation: If the up axis is Z, the remain two axes will X And Y, so the ParityEven is X, and the ParityOdd is Y
	const FbxAxisSystem::EFrontVector frontVector = FbxAxisSystem::eParityOdd;
	const FbxAxisSystem::ECoordSystem rightVector = FbxAxisSystem::eRightHanded;
	return FbxAxisSystem(upVector, frontVector, rightVector);
}

FbxSystemUnit FbxUtils::getBlastFBXUnit()
{
	return FbxSystemUnit::cm;
}

std::string FbxUtils::FbxAxisSystemToString(const FbxAxisSystem& axisSystem)
{
	std::stringstream ss;
	int upSign, frontSign;
	FbxAxisSystem::EUpVector upVector = axisSystem.GetUpVector(upSign);
	FbxAxisSystem::EFrontVector frontVector = axisSystem.GetFrontVector(frontSign);
	FbxAxisSystem::ECoordSystem  coordSystem = axisSystem.GetCoorSystem();
	ss << "Predefined Type: ";
	if (axisSystem == FbxAxisSystem::MayaZUp)
	{
		ss << "MayaZUP";
	}
	else if (axisSystem == FbxAxisSystem::MayaYUp)
	{
		ss << "MayaYUp";
	}
	else if (axisSystem == FbxAxisSystem::Max)
	{
		ss << "Max";
	}
	else if (axisSystem == FbxAxisSystem::Motionbuilder)
	{
		ss << "Motionbuilder";
	}
	else if (axisSystem == FbxAxisSystem::OpenGL)
	{
		ss << "OpenGL";
	}
	else if (axisSystem == FbxAxisSystem::DirectX)
	{
		ss << "OpenGL";
	}
	else if (axisSystem == FbxAxisSystem::Lightwave)
	{
		ss << "OpenGL";
	}
	else
	{
		ss << "<Other>";
	}
	ss << " UpVector: " << (upSign > 0 ? "+" : "-");
	switch (upVector)
	{
		case FbxAxisSystem::eXAxis: ss << "eXAxis"; break;
		case FbxAxisSystem::eYAxis: ss << "eYAxis"; break;
		case FbxAxisSystem::eZAxis: ss << "eZAxis"; break;
		default: ss << "<unknown>"; break;
	}

	ss << " FrontVector: " << (frontSign > 0 ? "+" : "-");
	switch (frontVector)
	{
	case FbxAxisSystem::eParityEven: ss << "eParityEven"; break;
	case FbxAxisSystem::eParityOdd: ss << "eParityOdd"; break;
	default: ss << "<unknown>"; break;
	}

	ss << " CoordSystem: ";
	switch (coordSystem)
	{
	case FbxAxisSystem::eLeftHanded: ss << "eLeftHanded"; break;
	case FbxAxisSystem::eRightHanded: ss << "eRightHanded"; break;
	default: ss << "<unknown>"; break;
	}
	
	return ss.str();
}

std::string FbxUtils::FbxSystemUnitToString(const FbxSystemUnit& systemUnit)
{
	return std::string(systemUnit.GetScaleFactorAsString());
}

const static std::string currentChunkPrefix = "chunk_";
const static std::string oldChunkPrefix = "bone_";

static uint32_t getChunkIndexForNodeInternal(const std::string& chunkPrefix, FbxNode* node, uint32_t* outParentChunkIndex /*=nullptr*/)
{
	if (!node)
	{
		//Found nothing
		return UINT32_MAX;
	}

	std::string nodeName(node->GetNameOnly());
	for (char& c : nodeName)
		c = (char)std::tolower(c);

	if (nodeName.substr(0, chunkPrefix.size()) == chunkPrefix)
	{
		std::istringstream iss(nodeName.substr(chunkPrefix.size()));
		uint32_t ret = UINT32_MAX;
		iss >> ret;
		if (!iss.fail())
		{
			if (outParentChunkIndex)
			{
				*outParentChunkIndex = getChunkIndexForNodeInternal(chunkPrefix, node->GetParent(), nullptr);
			}
			return ret;
		}
	}

	return getChunkIndexForNodeInternal(chunkPrefix, node->GetParent(), outParentChunkIndex);
}

uint32_t FbxUtils::getChunkIndexForNode(FbxNode* node, uint32_t* outParentChunkIndex /*=nullptr*/)
{
	return getChunkIndexForNodeInternal(currentChunkPrefix, node, outParentChunkIndex);
}

uint32_t FbxUtils::getChunkIndexForNodeBackwardsCompatible(FbxNode* node, uint32_t* outParentChunkIndex /*= nullptr*/)
{
	return getChunkIndexForNodeInternal(oldChunkPrefix, node, outParentChunkIndex);
}

std::string FbxUtils::getChunkNodeName(uint32_t chunkIndex)
{
	//This naming is required for the UE4 plugin to find them
	std::ostringstream namestream;
	namestream << currentChunkPrefix << chunkIndex;
	return namestream.str();
}

std::string FbxUtils::getCollisionGeometryLayerName()
{
	return "Collision";
}

std::string FbxUtils::getRenderGeometryLayerName()
{
	return "Render";
}
