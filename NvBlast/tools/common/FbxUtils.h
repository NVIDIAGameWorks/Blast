#pragma once
#include "PxVec3.h"
#include "PxVec2.h"

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
	static void VertexToFbx(Nv::Blast::Vertex& vert, FbxVector4& outVertex, FbxVector4& outNormal, FbxVector2& outUV);

	static void PxVec3ToFbx(physx::PxVec3& inVector, FbxVector4& outVector);
	static void PxVec2ToFbx(physx::PxVec2& inVector, FbxVector2& outVector);
};
