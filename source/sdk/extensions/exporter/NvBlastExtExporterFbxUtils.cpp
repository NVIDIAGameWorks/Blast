// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2022 NVIDIA Corporation. All rights reserved.


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
