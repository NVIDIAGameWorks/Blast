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


#ifndef NVBLASTAUTHORINGFCUTOUTIMPL_H
#define NVBLASTAUTHORINGFCUTOUTIMPL_H

#include "NvBlastExtAuthoringCutout.h"
#include <vector>
#include <PxVec2.h>
#include <PxVec3.h>
#include <PxMat44.h>

namespace Nv
{
namespace Blast
{

struct PolyVert
{
	uint16_t index;
	uint16_t flags;
};

struct ConvexLoop
{
	std::vector<PolyVert> polyVerts;
};

struct Cutout
{
	std::vector<physx::PxVec3> vertices;
	//std::vector<ConvexLoop> convexLoops;
	std::vector<physx::PxVec3> smoothingGroups;
};

struct POINT2D
{
	POINT2D() {}
	POINT2D(int32_t _x, int32_t _y) : x(_x), y(_y) {}

	int32_t x;
	int32_t y;

	bool operator==(const POINT2D& other) const
	{
		return x == other.x && y == other.y;
	}
	bool operator<(const POINT2D& other) const
	{
		if (x == other.x) return y < other.y;
		return x < other.x;
	}
};

struct CutoutSetImpl : public CutoutSet
{
	CutoutSetImpl() : periodic(false), dimensions(0.0f)
	{
	}

	uint32_t			getCutoutCount() const
	{
		return (uint32_t)cutouts.size() - 1;
	}

	uint32_t			getCutoutVertexCount(uint32_t cutoutIndex, uint32_t loopIndex) const
	{
		return (uint32_t)cutoutLoops[cutouts[cutoutIndex] + loopIndex].vertices.size();
	}
	uint32_t			getCutoutLoopCount(uint32_t cutoutIndex) const
	{
		return (uint32_t)cutouts[cutoutIndex + 1] - cutouts[cutoutIndex];
	}

	const NvcVec3& getCutoutVertex(uint32_t cutoutIndex, uint32_t loopIndex, uint32_t vertexIndex) const;

	bool				isCutoutVertexToggleSmoothingGroup(uint32_t cutoutIndex, uint32_t loopIndex, uint32_t vertexIndex) const
	{
		auto& vRef = cutoutLoops[cutouts[cutoutIndex] + loopIndex].vertices[vertexIndex];
		for (auto& v : cutoutLoops[cutouts[cutoutIndex] + loopIndex].smoothingGroups)
		{
			if ((vRef - v).magnitudeSquared() < 1e-5)
			{
				return true;
			}
		}
		return false;
	}

	bool					isPeriodic() const
	{
		return periodic;
	}
	const NvcVec2& getDimensions() const;

	//void					serialize(physx::PxFileBuf& stream) const;
	//void					deserialize(physx::PxFileBuf& stream);

	void					release()
	{
		delete this;
	}

	std::vector<Cutout>		cutoutLoops;
	std::vector<uint32_t>	cutouts;
	bool					periodic;
	physx::PxVec2			dimensions;
};

void createCutoutSet(Nv::Blast::CutoutSetImpl& cutoutSet, const uint8_t* pixelBuffer, uint32_t bufferWidth, uint32_t bufferHeight,
	float segmentationErrorThreshold, float snapThreshold, bool periodic, bool expandGaps);


} // namespace Blast
} // namespace Nv

#endif // ifndef NVBLASTAUTHORINGFCUTOUTIMPL_H
