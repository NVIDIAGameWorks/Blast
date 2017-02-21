#include "fbxsdk.h"
#include "FbxUtils.h"
#include "PxVec3.h"
#include "PxVec2.h"
#include "NvBlastExtAuthoringTypes.h"

using physx::PxVec3;
using physx::PxVec2;


void FbxUtils::VertexToFbx(Nv::Blast::Vertex& vert, FbxVector4& outVertex, FbxVector4& outNormal, FbxVector2& outUV)
{
	PxVec3ToFbx(vert.p, outVertex);
	PxVec3ToFbx(vert.n, outNormal);
	PxVec2ToFbx(vert.uv[0], outUV);
}

void FbxUtils::PxVec3ToFbx(physx::PxVec3& inVector, FbxVector4& outVector)
{
	outVector[0] = inVector.x;
	outVector[1] = inVector.y;
	outVector[2] = inVector.z;
	outVector[3] = 0;
}

void FbxUtils::PxVec2ToFbx(physx::PxVec2& inVector, FbxVector2& outVector)
{
	outVector[0] = inVector.x;
	outVector[1] = inVector.y;
}
