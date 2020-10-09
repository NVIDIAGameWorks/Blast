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
// Copyright (c) 2016-2020 NVIDIA Corporation. All rights reserved.

#define _CRT_SECURE_NO_WARNINGS

#include "NvBlastExtAuthoringMeshImpl.h"
#include "NvBlastExtAuthoringTypes.h"
#include <NvBlastAssert.h>
#include "PxMath.h"
#include <NvBlastPxSharedHelpers.h>
#include <cmath>
#include <string.h>
#include <vector>
#include <algorithm>

namespace Nv
{
namespace Blast
{

MeshImpl::MeshImpl(const NvcVec3* position, const NvcVec3* normals, const NvcVec2* uv, uint32_t verticesCount,
                   const uint32_t* indices, uint32_t indicesCount)
{

	mVertices.resize(verticesCount);
	for (uint32_t i = 0; i < mVertices.size(); ++i)
	{
		mVertices[i].p = position[i];
	}
	if (normals != 0)
	{
		for (uint32_t i = 0; i < mVertices.size(); ++i)
		{
			mVertices[i].n = normals[i];
		}

	}
	else
	{
		for (uint32_t i = 0; i < mVertices.size(); ++i)
		{
			mVertices[i].n = {0, 0, 0};
		}
	}
	if (uv != 0)
	{
		for (uint32_t i = 0; i < mVertices.size(); ++i)
		{
			mVertices[i].uv[0] = uv[i];
		}
	}
	else
	{
		for (uint32_t i = 0; i < mVertices.size(); ++i)
		{
			mVertices[i].uv[0] = {0, 0};
		}
	}
	mEdges.resize(indicesCount);
	mFacets.resize(indicesCount / 3);


	int32_t facetId = 0;
	for (uint32_t i = 0; i < indicesCount; i += 3)
	{
		mEdges[i].s = indices[i];
		mEdges[i].e = indices[i + 1];

		mEdges[i + 1].s = indices[i + 1];
		mEdges[i + 1].e = indices[i + 2];

		mEdges[i + 2].s = indices[i + 2];
		mEdges[i + 2].e = indices[i];
		mFacets[facetId].firstEdgeNumber = i;
		mFacets[facetId].edgesCount = 3;
		mFacets[facetId].materialId = 0;
		//Unassigned for now
		mFacets[facetId].smoothingGroup = -1;
		facetId++;
	}
	recalculateBoundingBox();
}

MeshImpl::MeshImpl(const Vertex* vertices, const Edge* edges, const Facet* facets, uint32_t posCount, uint32_t edgesCount, uint32_t facetsCount)
{
	mVertices.resize(posCount);
	mEdges.resize(edgesCount);
	mFacets.resize(facetsCount);

	memcpy(mVertices.data(), vertices, sizeof(Vertex) * posCount);
	memcpy(mEdges.data(), edges, sizeof(Edge) * edgesCount);
	memcpy(mFacets.data(), facets, sizeof(Facet) * facetsCount);
	recalculateBoundingBox();	
}

MeshImpl::MeshImpl(const Vertex* vertices, uint32_t count)
{
	mVertices = std::vector<Vertex>(vertices, vertices + count);
	mEdges.resize(count);
	mFacets.resize(count / 3);
	uint32_t vp = 0;
	for (uint32_t i = 0; i < count; i += 3)
	{
		mEdges[i].s = vp;
		mEdges[i].e = vp + 1;

		mEdges[i + 1].s = vp + 1;
		mEdges[i + 1].e = vp + 2;

		mEdges[i + 2].s = vp + 2;
		mEdges[i + 2].e = vp;
		vp += 3;
	}
	for (uint32_t i = 0; i < count / 3; ++i)
	{
		mFacets[i].edgesCount = 3;
		mFacets[i].firstEdgeNumber = i * 3;
	}
	recalculateBoundingBox();
}

MeshImpl::MeshImpl(const Vertex* vertices, uint32_t count, uint32_t* indices, uint32_t indexCount, void* materials, uint32_t materialStride)
{
	mVertices = std::vector<Vertex>(vertices, vertices + count);
	mEdges.resize(indexCount);
	mFacets.resize(indexCount / 3);

	for (uint32_t i = 0; i < indexCount; i += 3)
	{
		mEdges[i].s = indices[i];
		mEdges[i].e = indices[i + 1];

		mEdges[i + 1].s = indices[i + 1];
		mEdges[i + 1].e = indices[i + 2];

		mEdges[i + 2].s = indices[i + 2];
		mEdges[i + 2].e = indices[i];
	}
	for (uint32_t i = 0; i < indexCount / 3; ++i)
	{
		mFacets[i].edgesCount = 3;
		mFacets[i].firstEdgeNumber = i * 3;
		mFacets[i].userData = 0;
		if (materials != nullptr)
		{
			mFacets[i].materialId = *(uint32_t*)((uint8_t*)materials + i * materialStride);
		}
	}
	recalculateBoundingBox();
}



float MeshImpl::getMeshVolume()
{
	/**
		Check if mesh boundary consist only of triangles
	*/
	for (uint32_t i = 0; i < mFacets.size(); ++i)
	{
		if (mFacets[i].edgesCount != 3)
		{
			return 0.0f;
		}
	}	

	float volume = 0;
	for (uint32_t i = 0; i < mFacets.size(); ++i)
	{
		int32_t offset = mFacets[i].firstEdgeNumber;
		NvcVec3& a     = mVertices[mEdges[offset].s].p;
		NvcVec3& b     = mVertices[mEdges[offset + 1].s].p;
		NvcVec3& c     = mVertices[mEdges[offset + 2].s].p;
		
		volume += (a.x * b.y * c.z - a.x * b.z * c.y - a.y * b.x * c.z + a.y * b.z * c.x + a.z * b.x * c.y - a.z * b.y * c.x);
	}
	return (1.0f / 6.0f) * std::abs(volume);
}


uint32_t MeshImpl::getFacetCount() const
{
	return static_cast<uint32_t>(mFacets.size());
}

Vertex* MeshImpl::getVerticesWritable()
{
	return mVertices.data();
}

Edge* MeshImpl::getEdgesWritable()
{
	return mEdges.data();
}

const Vertex* MeshImpl::getVertices() const
{
	return mVertices.data();
}

const Edge* MeshImpl::getEdges() const
{
	return mEdges.data();
}

uint32_t MeshImpl::getEdgesCount() const
{
	return static_cast<uint32_t>(mEdges.size());
}
uint32_t MeshImpl::getVerticesCount() const
{
	return static_cast<uint32_t>(mVertices.size());
}
Facet* MeshImpl::getFacetsBufferWritable()
{
	return mFacets.data();
}
const Facet* MeshImpl::getFacetsBuffer() const
{
	return mFacets.data();
}
Facet* MeshImpl::getFacetWritable(int32_t facet)
{
	return &mFacets[facet];
}
const Facet* MeshImpl::getFacet(int32_t facet) const
{
	return &mFacets[facet];
}

MeshImpl::~MeshImpl()
{
}

void MeshImpl::release()
{
	delete this;
}

const NvcBounds3& MeshImpl::getBoundingBox() const
{
	return fromPxShared(mBounds);
}

NvcBounds3& MeshImpl::getBoundingBoxWritable()
{
	return fromPxShared(mBounds);
}


void MeshImpl::recalculateBoundingBox()
{
	mBounds.setEmpty();
	for (uint32_t i = 0; i < mVertices.size(); ++i)
	{
		mBounds.include(toPxShared(mVertices[i].p));
	}
	calcPerFacetBounds();
}

const NvcBounds3* MeshImpl::getFacetBound(uint32_t index) const 
{
	if (mPerFacetBounds.empty())
	{
		return nullptr;
	}
	return &fromPxShared(mPerFacetBounds[index]);
}

void MeshImpl::calcPerFacetBounds()
{
	mPerFacetBounds.resize(mFacets.size());

	for (uint32_t i = 0; i < mFacets.size(); ++i)
	{
		auto& fb = mPerFacetBounds[i];
		fb.setEmpty();

		for (uint32_t v = 0; v < mFacets[i].edgesCount; ++v)
		{
			fb.include(toPxShared(mVertices[mEdges[mFacets[i].firstEdgeNumber + v].s].p));
			fb.include(toPxShared(mVertices[mEdges[mFacets[i].firstEdgeNumber + v].e].p));
		}
	}
}

void MeshImpl::setMaterialId(const int32_t* materialId)
{
	if (materialId != nullptr)
	{
		for (uint32_t i = 0; i < mFacets.size(); ++i)
		{
			mFacets[i].materialId = *materialId;
			++materialId;
		}
	}
}

bool MeshImpl::isValid() const
{
	return mVertices.size() > 0 && mEdges.size() > 0 && mFacets.size() > 0;
}


void MeshImpl::replaceMaterialId(int32_t oldMaterialId, int32_t newMaterialId)
{
	for (uint32_t i = 0; i < mFacets.size(); ++i)
	{
		if (mFacets[i].materialId == oldMaterialId)
		{
			mFacets[i].materialId = newMaterialId;
		}
	}
}

void MeshImpl::setSmoothingGroup(const int32_t* smoothingGroups)
{
	if (smoothingGroups != nullptr)
	{
		for (uint32_t i = 0; i < mFacets.size(); ++i)
		{
			mFacets[i].smoothingGroup = *smoothingGroups;
			++smoothingGroups;
		}
	}
}



} // namespace Blast
} // namespace Nv
