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

#define _CRT_SECURE_NO_WARNINGS

#include "NvBlastExtAuthoringMeshImpl.h"
#include "NvBlastExtAuthoringTypes.h"
#include "NvBlastExtAuthoringPerlinNoise.h"
#include <NvBlastAssert.h>
#include "PxMath.h"
#include <cmath>
#include <string.h>
#include <vector>
#include <algorithm>

using physx::PxVec2;
using physx::PxVec3;
using physx::PxBounds3;

#define UV_SCALE 1.f

#define CYLINDER_UV_SCALE (UV_SCALE * 1.732)

namespace Nv
{
namespace Blast
{

MeshImpl::MeshImpl(const PxVec3* position, const PxVec3* normals, const PxVec2* uv, uint32_t verticesCount, const uint32_t* indices, uint32_t indicesCount)
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
			mVertices[i].n = PxVec3(0, 0, 0);
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
			mVertices[i].uv[0] = PxVec2(0, 0);
		}
	}
	mEdges.resize(indicesCount);
	mFacets.resize(indicesCount / 3);
	mBounds.setEmpty();
	for (uint32_t i = 0; i < verticesCount; ++i)
	{
		mBounds.include(mVertices[i].p);
	}
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
}

MeshImpl::MeshImpl(const Vertex* vertices, const Edge* edges, const Facet* facets, uint32_t posCount, uint32_t edgesCount, uint32_t facetsCount)
{
	mVertices.resize(posCount);
	mEdges.resize(edgesCount);
	mFacets.resize(facetsCount);

	memcpy(mVertices.data(), vertices, sizeof(Vertex) * posCount);
	memcpy(mEdges.data(), edges, sizeof(Edge) * edgesCount);
	memcpy(mFacets.data(), facets, sizeof(Facet) * facetsCount);
	mBounds.setEmpty();
	for (uint32_t i = 0; i < posCount; ++i)
	{
		mBounds.include(mVertices[i].p);
	}
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
		PxVec3& a = mVertices[mEdges[offset].s].p;
		PxVec3& b = mVertices[mEdges[offset + 1].s].p;
		PxVec3& c = mVertices[mEdges[offset + 2].s].p;
		
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

const PxBounds3& MeshImpl::getBoundingBox() const
{
	return mBounds;
}

PxBounds3& MeshImpl::getBoundingBoxWritable() 
{
	return mBounds;
}

void MeshImpl::recalculateBoundingBox()
{
	mBounds.setEmpty();
	for (uint32_t i = 0; i < mVertices.size(); ++i)
	{
		mBounds.include(mVertices[i].p);
	}
}



void getTangents(const PxVec3& normal, PxVec3& t1, PxVec3& t2)
{

	if (std::abs(normal.z) < 0.9)
	{
		t1 = normal.cross(PxVec3(0, 0, 1));
	}
	else
	{
		t1 = normal.cross(PxVec3(1, 0, 0));
	}
	t2 = t1.cross(normal);
}

Mesh* getCuttingBox(const PxVec3& point, const PxVec3& normal, float size, int64_t id, int32_t interiorMaterialId)
{
	PxVec3 lNormal = normal.getNormalized();
	PxVec3 t1, t2;
	getTangents(lNormal, t1, t2);

	std::vector<Vertex> positions(8);
	positions[0].p = point + (t1 + t2) * size;
	positions[1].p = point + (t2 - t1) * size;

	positions[2].p = point + (-t1 - t2) * size;
	positions[3].p = point + (t1 - t2) * size;


	positions[4].p = point + (t1 + t2 + lNormal) * size;
	positions[5].p = point + (t2 - t1 + lNormal) * size;

	positions[6].p = point + (-t1 - t2 + lNormal) * size;
	positions[7].p = point + (t1 - t2 + lNormal) * size;

	positions[0].n = -lNormal;
	positions[1].n = -lNormal;

	positions[2].n = -lNormal;
	positions[3].n = -lNormal;


	positions[4].n = -lNormal;
	positions[5].n = -lNormal;

	positions[6].n = -lNormal;
	positions[7].n = -lNormal;

	positions[0].uv[0] = PxVec2(0, 0);
	positions[1].uv[0] = PxVec2(UV_SCALE, 0);

	positions[2].uv[0] = PxVec2(UV_SCALE, UV_SCALE);
	positions[3].uv[0] = PxVec2(0, UV_SCALE);


	positions[4].uv[0] = PxVec2(0, 0);
	positions[5].uv[0] = PxVec2(UV_SCALE, 0);

	positions[6].uv[0] = PxVec2(UV_SCALE, UV_SCALE);
	positions[7].uv[0] = PxVec2(0, UV_SCALE);


	std::vector<Edge> edges;
	std::vector<Facet> facets;

	edges.push_back(Edge(0, 1));
	edges.push_back(Edge(1, 2));
	edges.push_back(Edge(2, 3));
	edges.push_back(Edge(3, 0));
	facets.push_back(Facet(0, 4, interiorMaterialId, id, -1));


	edges.push_back(Edge(0, 3));
	edges.push_back(Edge(3, 7));
	edges.push_back(Edge(7, 4));
	edges.push_back(Edge(4, 0));
	facets.push_back(Facet(4, 4, interiorMaterialId, id, -1));

	edges.push_back(Edge(3, 2));
	edges.push_back(Edge(2, 6));
	edges.push_back(Edge(6, 7));
	edges.push_back(Edge(7, 3));
	facets.push_back(Facet(8, 4, interiorMaterialId, id, -1));

	edges.push_back(Edge(5, 6));
	edges.push_back(Edge(6, 2));
	edges.push_back(Edge(2, 1));
	edges.push_back(Edge(1, 5));
	facets.push_back(Facet(12, 4, interiorMaterialId, id, -1));

	edges.push_back(Edge(4, 5));
	edges.push_back(Edge(5, 1));
	edges.push_back(Edge(1, 0));
	edges.push_back(Edge(0, 4));
	facets.push_back(Facet(16, 4, interiorMaterialId, id, -1));

	edges.push_back(Edge(4, 7));
	edges.push_back(Edge(7, 6));
	edges.push_back(Edge(6, 5));
	edges.push_back(Edge(5, 4));
	facets.push_back(Facet(20, 4, interiorMaterialId, id, -1));
	return new MeshImpl(positions.data(), edges.data(), facets.data(), static_cast<uint32_t>(positions.size()), static_cast<uint32_t>(edges.size()), static_cast<uint32_t>(facets.size()));
}

void inverseNormalAndIndices(Mesh* mesh)
{
	for (uint32_t i = 0; i < mesh->getVerticesCount(); ++i)
	{
		mesh->getVerticesWritable()[i].n *= -1.0f;
	}
	for (uint32_t i = 0; i < mesh->getFacetCount(); ++i)
	{
		mesh->getFacetWritable(i)->userData = -mesh->getFacet(i)->userData;
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


void setCuttingBox(const PxVec3& point, const PxVec3& normal, Mesh* mesh, float size, int64_t id)
{
	PxVec3 t1, t2;
	PxVec3 lNormal = normal.getNormalized();
	getTangents(lNormal, t1, t2);

	Vertex* positions = mesh->getVerticesWritable();
	positions[0].p = point + (t1 + t2) * size;
	positions[1].p = point + (t2 - t1) * size;

	positions[2].p = point + (-t1 - t2) * size;
	positions[3].p = point + (t1 - t2) * size;


	positions[4].p = point + (t1 + t2 + lNormal) * size;
	positions[5].p = point + (t2 - t1 + lNormal) * size;

	positions[6].p = point + (-t1 - t2 + lNormal) * size;
	positions[7].p = point + (t1 - t2 + lNormal) * size;

	positions[0].n = -lNormal;
	positions[1].n = -lNormal;

	positions[2].n = -lNormal;
	positions[3].n = -lNormal;


	positions[4].n = -lNormal;
	positions[5].n = -lNormal;

	positions[6].n = -lNormal;
	positions[7].n = -lNormal;

	for (uint32_t i = 0; i < mesh->getFacetCount(); ++i)
	{
		mesh->getFacetWritable(i)->userData = id;
	}
	mesh->recalculateBoundingBox();
}

bool MeshImpl::isValid() const
{
	return mVertices.size() > 0 && mEdges.size() > 0 && mFacets.size() > 0;
}

struct Stepper
{
	virtual physx::PxVec3	getStep1(uint32_t w, uint32_t h) const = 0;
	virtual physx::PxVec3	getStep2(uint32_t w) const = 0;
	virtual physx::PxVec3	getStart() const = 0;
	virtual physx::PxVec3	getNormal(uint32_t w, uint32_t h) const = 0;
	virtual bool			isStep2ClosedLoop() const
	{
		return false;
	}
	virtual bool			isStep2FreeBoundary() const
	{
		return false;
	}
};

struct PlaneStepper : public Stepper
{
	PlaneStepper(const physx::PxVec3& normal, const physx::PxVec3& point, float sizeX, float sizeY, uint32_t resolutionX, uint32_t resolutionY, bool swapTangents = false)
	{
		PxVec3 t1, t2;
		lNormal = normal.getNormalized();
		getTangents(lNormal, t1, t2);
		if (swapTangents)
		{
			std::swap(t1, t2);
		}
		t11d = -t1 * 2.0f * sizeX / resolutionX;
		t12d = -t2 * 2.0f * sizeY / resolutionY;
		t21d = t11d;
		t22d = t12d;
		cPos = point + (t1 * sizeX + t2 * sizeY);
		resY = resolutionY;
	}
	//Define face by 4 corner points, points should lay in plane
	PlaneStepper(const physx::PxVec3& p11, const physx::PxVec3& p12, const physx::PxVec3& p21, const physx::PxVec3& p22, 
		uint32_t resolutionX, uint32_t resolutionY)
	{
		lNormal = -(p21 - p11).cross(p12 - p11).getNormalized();
		if (lNormal.magnitude() < 1e-5)
		{
			lNormal = (p21 - p22).cross(p12 - p22).getNormalized();
		}
		t11d = (p11 - p21) / resolutionX;
		t12d = (p12 - p11) / resolutionY;
		t21d = (p12 - p22) / resolutionX;
		t22d = (p22 - p21) / resolutionY;
		cPos = p21;
		resY = resolutionY;
	}
	physx::PxVec3 getStep1(uint32_t y, uint32_t) const
	{
		return (t11d * (resY - y) + t21d * y) / resY;
	}
	physx::PxVec3 getStep2(uint32_t) const
	{
		return t22d;
	}
	physx::PxVec3 getStart() const
	{
		return cPos;
	}
	physx::PxVec3 getNormal(uint32_t, uint32_t) const
	{
		return lNormal;
	}

	PxVec3 t11d, t12d, t21d, t22d, cPos, lNormal;
	uint32_t resY;
};

void fillEdgesAndFaces(std::vector<Edge>& edges, std::vector<Facet>& facets, 
	uint32_t h, uint32_t w, uint32_t firstVertex, uint32_t verticesCount, int64_t id, int32_t interiorMaterialId, int32_t smoothingGroup = -1, bool reflected = false)
{
	for (uint32_t i = 0; i < w; ++i)
	{
		for (uint32_t j = 0; j < h; ++j)
		{
			uint32_t start = edges.size();
			uint32_t idx00 = i * (h + 1) + j + firstVertex;
			uint32_t idx01 = idx00 + 1;
			uint32_t idx10 = (idx00 + h + 1) % verticesCount;
			uint32_t idx11 = (idx01 + h + 1) % verticesCount;
			if (reflected)
			{
				edges.push_back(Edge(idx01, idx11));
				edges.push_back(Edge(idx11, idx10));
				edges.push_back(Edge(idx10, idx01));
				facets.push_back(Facet(start, 3, interiorMaterialId, id, smoothingGroup));

				start = edges.size();
				edges.push_back(Edge(idx01, idx10));
				edges.push_back(Edge(idx10, idx00));
				edges.push_back(Edge(idx00, idx01));
				facets.push_back(Facet(start, 3, interiorMaterialId, id, smoothingGroup));
			}
			else
			{
				edges.push_back(Edge(idx00, idx01));
				edges.push_back(Edge(idx01, idx11));
				edges.push_back(Edge(idx11, idx00));
				facets.push_back(Facet(start, 3, interiorMaterialId, id, smoothingGroup));

				start = edges.size();
				edges.push_back(Edge(idx00, idx11));
				edges.push_back(Edge(idx11, idx10));
				edges.push_back(Edge(idx10, idx00));
				facets.push_back(Facet(start, 3, interiorMaterialId, id, smoothingGroup));
			}
		}
	}
}

void getNoisyFace(std::vector<Vertex>& vertices, std::vector<Edge>& edges, std::vector<Facet>& facets,
	uint32_t h, uint32_t w, const physx::PxVec2& uvOffset, const physx::PxVec2& uvScale,
	const Stepper& stepper, SimplexNoise& nEval, int64_t id, int32_t interiorMaterialId, bool randomizeLast = false)
{
	uint32_t randIdx = randomizeLast ? 1 : 0;
	PxVec3 cPosit = stepper.getStart();
	uint32_t firstVertex = vertices.size();
	for (uint32_t i = 0; i < w + 1; ++i)
	{
		PxVec3 lcPosit = cPosit;
		for (uint32_t j = 0; j < h + 1; ++j)
		{
			vertices.push_back(Vertex());
			vertices.back().p = lcPosit;
			vertices.back().uv[0] = uvOffset + uvScale.multiply(physx::PxVec2(j, i));
			lcPosit += stepper.getStep1(i, j);
		}
		cPosit += stepper.getStep2(i);
	}

	for (uint32_t i = 1 - randIdx; i < w + randIdx; ++i)
	{
		for (uint32_t j = 1; j < h; ++j)
		{
			//TODO limit max displacement for cylinder
			PxVec3& pnt = vertices[i * (h + 1) + j + firstVertex].p;
			pnt += stepper.getNormal(i, j) * nEval.sample(pnt);
		}
	}

	fillEdgesAndFaces(edges, facets, h, w, firstVertex, vertices.size(), id, interiorMaterialId);
}

PX_INLINE uint32_t unsignedMod(int32_t n, uint32_t modulus)
{
	const int32_t d = n / (int32_t)modulus;
	const int32_t m = n - d*(int32_t)modulus;
	return m >= 0 ? (uint32_t)m : (uint32_t)m + modulus;
}

void calculateNormals(std::vector<Vertex>& vertices, uint32_t h, uint32_t w, bool inverseNormals = false)
{
	for (uint32_t i = 1; i < w; ++i)
	{
		for (uint32_t j = 1; j < h; ++j)
		{
			int32_t idx = i * (h + 1) + j;
			PxVec3 v1 = vertices[idx + h + 1].p - vertices[idx].p;
			PxVec3 v2 = vertices[idx + 1].p - vertices[idx].p;
			PxVec3 v3 = vertices[idx - (h + 1)].p - vertices[idx].p;
			PxVec3 v4 = vertices[idx - 1].p - vertices[idx].p;

			vertices[idx].n = v1.cross(v2) + v2.cross(v3) + v3.cross(v4) + v4.cross(v1);
			if (inverseNormals)
			{
				vertices[idx].n = -vertices[idx].n;
			}
			vertices[idx].n.normalize();
		}
	}
}

Mesh* getNoisyCuttingBoxPair(const physx::PxVec3& point, const physx::PxVec3& normal, float size, float jaggedPlaneSize, physx::PxVec3 resolution, int64_t id, float amplitude, float frequency, int32_t octaves, int32_t seed, int32_t interiorMaterialId)
{
	PxVec3 t1, t2;
	PxVec3 lNormal = normal.getNormalized();
	getTangents(lNormal, t1, t2);
	float sz = 2.f * jaggedPlaneSize;
	uint32_t resolutionX = std::max(1u, (uint32_t)std::roundf(sz * std::abs(t1.x) * resolution.x + sz * std::abs(t1.y) * resolution.y + sz * std::abs(t1.z) * resolution.z));
	uint32_t resolutionY = std::max(1u, (uint32_t)std::roundf(sz * std::abs(t2.x) * resolution.x + sz * std::abs(t2.y) * resolution.y + sz * std::abs(t2.z) * resolution.z));

	PlaneStepper stepper(normal, point, jaggedPlaneSize, jaggedPlaneSize, resolutionX, resolutionY);
	SimplexNoise nEval(amplitude, frequency, octaves, seed);

	std::vector<Vertex> vertices; vertices.reserve((resolutionX + 1) * (resolutionY + 1) + 12);
	std::vector<Edge> edges;
	std::vector<Facet> facets;
	getNoisyFace(vertices, edges, facets, resolutionX, resolutionY, physx::PxVec2(0.f), physx::PxVec2(UV_SCALE / resolutionX, UV_SCALE / resolutionY),
		stepper, nEval, id, interiorMaterialId);
	calculateNormals(vertices, resolutionX, resolutionY);

	uint32_t offset = (resolutionX + 1) * (resolutionY + 1);
	vertices.resize(offset + 12);

	vertices[0 + offset].p = point + (t1 + t2) * size;
	vertices[1 + offset].p = point + (t2 - t1) * size;

	vertices[2 + offset].p = point + (-t1 - t2) * size;
	vertices[3 + offset].p = point + (t1 - t2) * size;

	vertices[8 + offset].p = point + (t1 + t2) * jaggedPlaneSize;
	vertices[9 + offset].p = point + (t2 - t1) * jaggedPlaneSize;

	vertices[10 + offset].p = point + (-t1 - t2) * jaggedPlaneSize;
	vertices[11 + offset].p = point + (t1 - t2) * jaggedPlaneSize;

	vertices[4 + offset].p = point + (t1 + t2 + lNormal) * size;
	vertices[5 + offset].p = point + (t2 - t1 + lNormal) * size;

	vertices[6 + offset].p = point + (-t1 - t2 + lNormal) * size;
	vertices[7 + offset].p = point + (t1 - t2 + lNormal) * size;

	int32_t edgeOffset = edges.size();
	edges.push_back(Edge(0 + offset, 1 + offset));
	edges.push_back(Edge(1 + offset, 2 + offset));
	edges.push_back(Edge(2 + offset, 3 + offset));
	edges.push_back(Edge(3 + offset, 0 + offset));

	edges.push_back(Edge(11 + offset, 10 + offset));
	edges.push_back(Edge(10 + offset, 9 + offset));
	edges.push_back(Edge(9 + offset, 8 + offset));
	edges.push_back(Edge(8 + offset, 11 + offset));

	facets.push_back(Facet(edgeOffset, 8, interiorMaterialId, id, -1));

	edges.push_back(Edge(0 + offset, 3 + offset));
	edges.push_back(Edge(3 + offset, 7 + offset));
	edges.push_back(Edge(7 + offset, 4 + offset));
	edges.push_back(Edge(4 + offset, 0 + offset));
	facets.push_back(Facet(8 + edgeOffset, 4, interiorMaterialId, id, -1));

	edges.push_back(Edge(3 + offset, 2 + offset));
	edges.push_back(Edge(2 + offset, 6 + offset));
	edges.push_back(Edge(6 + offset, 7 + offset));
	edges.push_back(Edge(7 + offset, 3 + offset));
	facets.push_back(Facet(12 + edgeOffset, 4, interiorMaterialId, id, -1));

	edges.push_back(Edge(5 + offset, 6 + offset));
	edges.push_back(Edge(6 + offset, 2 + offset));
	edges.push_back(Edge(2 + offset, 1 + offset));
	edges.push_back(Edge(1 + offset, 5 + offset));
	facets.push_back(Facet(16 + edgeOffset, 4, interiorMaterialId, id, -1));

	edges.push_back(Edge(4 + offset, 5 + offset));
	edges.push_back(Edge(5 + offset, 1 + offset));
	edges.push_back(Edge(1 + offset, 0 + offset));
	edges.push_back(Edge(0 + offset, 4 + offset));
	facets.push_back(Facet(20 + edgeOffset, 4, interiorMaterialId, id, -1));

	edges.push_back(Edge(4 + offset, 7 + offset));
	edges.push_back(Edge(7 + offset, 6 + offset));
	edges.push_back(Edge(6 + offset, 5 + offset));
	edges.push_back(Edge(5 + offset, 4 + offset));
	facets.push_back(Facet(24 + edgeOffset, 4, interiorMaterialId, id, -1));

	//
	return new MeshImpl(vertices.data(), edges.data(), facets.data(), vertices.size(), edges.size(), facets.size());
}

Mesh* getBigBox(const PxVec3& point, float size, int32_t interiorMaterialId)
{
	PxVec3 normal(0, 0, 1);
	normal.normalize();
	PxVec3 t1, t2;
	getTangents(normal, t1, t2);

	std::vector<Vertex> positions(8);
	positions[0].p = point + (t1 + t2 - normal) * size;
	positions[1].p = point + (t2 - t1 - normal) * size;

	positions[2].p = point + (-t1 - t2 - normal) * size;
	positions[3].p = point + (t1 - t2 - normal) * size;


	positions[4].p = point + (t1 + t2 + normal) * size;
	positions[5].p = point + (t2 - t1 + normal) * size;

	positions[6].p = point + (-t1 - t2 + normal) * size;
	positions[7].p = point + (t1 - t2 + normal) * size;

	positions[0].uv[0] = PxVec2(0, 0);
	positions[1].uv[0] = PxVec2(UV_SCALE, 0);

	positions[2].uv[0] = PxVec2(UV_SCALE, UV_SCALE);
	positions[3].uv[0] = PxVec2(0, UV_SCALE);


	positions[4].uv[0] = PxVec2(0, 0);
	positions[5].uv[0] = PxVec2(UV_SCALE, 0);

	positions[6].uv[0] = PxVec2(UV_SCALE, UV_SCALE);
	positions[7].uv[0] = PxVec2(0, UV_SCALE);


	std::vector<Edge> edges;
	std::vector<Facet> facets;

	edges.push_back(Edge(0, 1));
	edges.push_back(Edge(1, 2));
	edges.push_back(Edge(2, 3));
	edges.push_back(Edge(3, 0));
	facets.push_back(Facet(0, 4, interiorMaterialId, 0, -1));


	edges.push_back(Edge(0, 3));
	edges.push_back(Edge(3, 7));
	edges.push_back(Edge(7, 4));
	edges.push_back(Edge(4, 0));
	facets.push_back(Facet(4, 4, interiorMaterialId, 0, -1));

	edges.push_back(Edge(3, 2));
	edges.push_back(Edge(2, 6));
	edges.push_back(Edge(6, 7));
	edges.push_back(Edge(7, 3));
	facets.push_back(Facet(8, 4, interiorMaterialId, 0, -1));

	edges.push_back(Edge(5, 6));
	edges.push_back(Edge(6, 2));
	edges.push_back(Edge(2, 1));
	edges.push_back(Edge(1, 5));
	facets.push_back(Facet(12, 4, interiorMaterialId, 0, -1));

	edges.push_back(Edge(4, 5));
	edges.push_back(Edge(5, 1));
	edges.push_back(Edge(1, 0));
	edges.push_back(Edge(0, 4));
	facets.push_back(Facet(16, 4, interiorMaterialId, 0, -1));

	edges.push_back(Edge(4, 7));
	edges.push_back(Edge(7, 6));
	edges.push_back(Edge(6, 5));
	edges.push_back(Edge(5, 4));
	facets.push_back(Facet(20, 4, interiorMaterialId, 0, -1));
	for (int i = 0; i < 8; ++i)
		positions[i].n = PxVec3(0, 0, 0);
	return new MeshImpl(positions.data(), edges.data(), facets.data(), static_cast<uint32_t>(positions.size()), static_cast<uint32_t>(edges.size()), static_cast<uint32_t>(facets.size()));
}

bool CmpSharedFace::operator()(const std::pair<physx::PxVec3, physx::PxVec3>& pv1, const std::pair<physx::PxVec3, physx::PxVec3>& pv2) const
{
	CmpVec vc;
	if ((pv1.first -pv2.first).magnitude() < 1e-5)
	{
		return vc(pv1.second, pv2.second);
	}
	return vc(pv1.first, pv2.first);
}

#define INDEXER_OFFSET (1ll << 32)

void buildCuttingConeFaces(const CutoutConfiguration& conf, const std::vector<std::vector<physx::PxVec3>>& cutoutPoints,
	float heightBot, float heightTop, float conicityBot, float conicityTop,
	int64_t& id, int32_t seed, int32_t interiorMaterialId, SharedFacesMap& sharedFacesMap)
{
	std::map<physx::PxVec3, std::pair<uint32_t, std::vector<physx::PxVec3>>, CmpVec> newCutoutPoints;
	uint32_t resH = (conf.noise.amplitude <= FLT_EPSILON) ? 1 : std::max((uint32_t)std::roundf((heightBot + heightTop) / conf.noise.samplingInterval.z) , 1u);

	//generate noisy faces
	SimplexNoise nEval(conf.noise.amplitude, conf.noise.frequency, conf.noise.octaveNumber, seed);

	for (uint32_t i = 0; i < cutoutPoints.size(); i++)
	{
		auto& points = cutoutPoints[i];
		uint32_t pointCount = points.size();
		float finalP = 0, currentP = 0;
		for (uint32_t j = 0; j < pointCount; j++)
		{
			finalP += (points[(j + 1) % pointCount] - points[j]).magnitude();
		}

		for (uint32_t p = 0; p < pointCount; p++)
		{
			auto p0 = points[p];
			auto p1 = points[(p + 1) % pointCount];

			auto cp0 = newCutoutPoints.find(p0);
			if (cp0 == newCutoutPoints.end())
			{ 
				newCutoutPoints[p0] = std::make_pair(0u, std::vector<physx::PxVec3>(resH + 1, physx::PxVec3(0.f)));
				cp0 = newCutoutPoints.find(p0);
			}
			auto cp1 = newCutoutPoints.find(p1);
			if (cp1 == newCutoutPoints.end())
			{
				newCutoutPoints[p1] = std::make_pair(0u, std::vector<physx::PxVec3>(resH + 1, physx::PxVec3(0.f)));
				cp1 = newCutoutPoints.find(p1);
			}		


			auto vec = p1 - p0;
			auto cPos = (p0 + p1) * 0.5f;
			uint32_t numPts = (conf.noise.amplitude <= FLT_EPSILON) ? 1 : (uint32_t)(std::abs(vec.x) / conf.noise.samplingInterval.x + std::abs(vec.y) / conf.noise.samplingInterval.y) + 1;

			auto normal = vec.cross(physx::PxVec3(0, 0, 1));
			normal = normal;

			auto p00 = p0 * conicityBot; p00.z = -heightBot;
			auto p01 = p1 * conicityBot; p01.z = -heightBot;
			auto p10 = p0 * conicityTop; p10.z = heightTop;
			auto p11 = p1 * conicityTop; p11.z = heightTop;
			PlaneStepper stepper(p00, p01, p10, p11, resH, numPts);

			PlaneStepper stepper1(normal, cPos, heightTop, vec.magnitude() * 0.5f, resH, numPts, true);
			stepper1.getNormal(0, 0);

			auto t = std::make_pair(p0, p1);
			auto sfIt = sharedFacesMap.find(t);
			if (sfIt == sharedFacesMap.end() && sharedFacesMap.find(std::make_pair(p1, p0)) == sharedFacesMap.end())
			{
				sharedFacesMap[t] = SharedFace(numPts, resH, -(id + INDEXER_OFFSET), interiorMaterialId);
				sfIt = sharedFacesMap.find(t);
				auto& SF = sfIt->second;
				getNoisyFace(SF.vertices, SF.edges, SF.facets, resH, numPts,
					physx::PxVec2(0, CYLINDER_UV_SCALE * currentP / (heightBot + heightTop)),
					physx::PxVec2(CYLINDER_UV_SCALE / resH, CYLINDER_UV_SCALE * vec.magnitude() / (heightBot + heightTop) / numPts),
					stepper, nEval, id++ + INDEXER_OFFSET, interiorMaterialId, true);

				currentP += vec.magnitude();
				cp0->second.first++;
				cp1->second.first++;
				for (uint32_t k = 0; k <= resH; k++)
				{
					cp0->second.second[k] += SF.vertices[k].p;
					cp1->second.second[k] += SF.vertices[SF.vertices.size() - resH - 1 + k].p;
				}
			}
		}
	}

	//limit faces displacement iteratively
	for (uint32_t i = 0; i < cutoutPoints.size(); i++)
	{
		auto& points = cutoutPoints[i];
		uint32_t pointCount = points.size();
		for (uint32_t p = 0; p < pointCount; p++)
		{
			auto p0 = points[p];
			auto p1 = points[(p + 1) % pointCount];
			auto p2 = points[(p + 2) % pointCount];
			auto& cp1 = newCutoutPoints.find(p1)->second;
			float d = physx::PxClamp((p1 - p0).getNormalized().dot((p2 - p1).getNormalized()), 0.f, 1.f);

			for (uint32_t h = 0; h <= resH; h++)
			{
				float z = cp1.second[h].z;
				float conicity = (conicityBot * h + conicityTop * (resH - h)) / resH;
				cp1.second[h] = cp1.second[h] * d + p1 * cp1.first * conicity * (1.f - d);
				cp1.second[h].z = z;
			}		
		}
	}

	//relax nearby points for too big faces displacement limitations
	for (uint32_t i = 0; i < cutoutPoints.size(); i++)
	{
		auto& points = cutoutPoints[i];
		uint32_t pointCount = points.size();
		for (uint32_t p = 0; p < pointCount; p++)
		{
			auto p0 = points[p];
			auto p1 = points[(p + 1) % pointCount];
			auto& cp0 = newCutoutPoints.find(p0)->second;
			auto& cp1 = newCutoutPoints.find(p1)->second;
			
			auto SFIt = sharedFacesMap.find(std::make_pair(p0, p1));
				
			uint32_t idx0 = 0, idx1;
			if (SFIt == sharedFacesMap.end())
			{
				SFIt = sharedFacesMap.find(std::make_pair(p1, p0));
				idx1 = 0;
				idx0 = SFIt->second.w * (SFIt->second.h + 1);
			}
			else
			{
				idx1 = SFIt->second.w * (SFIt->second.h + 1);
			}

			for (uint32_t h = 0; h <= resH; h++)
			{
				float z = cp1.second[h].z;
				float R0 = (cp0.second[h] / cp0.first - SFIt->second.vertices[idx0 + h].p).magnitude();
				float R1 = (cp1.second[h] / cp1.first - SFIt->second.vertices[idx1 + h].p).magnitude();
				float R = R0 - R1;
				float r = 0.25f * (cp1.second[h] / cp1.first - cp0.second[h] / cp0.first).magnitude();
				float conicity = (conicityBot * h + conicityTop * (resH - h)) / resH;
				if (R > r)
				{
					float w = std::min(1.f, r / R);
					cp1.second[h] = cp1.second[h] * w + p1 * cp1.first * conicity * (1.f - w);
					cp1.second[h].z = z;
				}
			}
		}

		for (int32_t p = pointCount - 1; p >= 0; p--)
		{
			auto p0 = points[p];
			auto p1 = points[unsignedMod(p - 1, pointCount)];
			auto& cp0 = newCutoutPoints.find(p0)->second;
			auto& cp1 = newCutoutPoints.find(p1)->second;

			auto SFIt = sharedFacesMap.find(std::make_pair(p0, p1));
			uint32_t idx0 = 0, idx1;
			if (SFIt == sharedFacesMap.end())
			{
				SFIt = sharedFacesMap.find(std::make_pair(p1, p0));
				idx1 = 0;
				idx0 = SFIt->second.w * (SFIt->second.h + 1);
			}
			else
			{
				idx1 = SFIt->second.w * (SFIt->second.h + 1);
			}

			for (uint32_t h = 0; h <= resH; h++)
			{
				float z = cp1.second[h].z;
				float R0 = (cp0.second[h] / cp0.first - SFIt->second.vertices[idx0 + h].p).magnitude();
				float R1 = (cp1.second[h] / cp1.first - SFIt->second.vertices[idx1 + h].p).magnitude();
				float R = R0 - R1;
				float r = 0.25f * (cp1.second[h] / cp1.first - cp0.second[h] / cp0.first).magnitude();
				float conicity = (conicityBot * h + conicityTop * (resH - h)) / resH;
				if (R  > r)
				{
					float w = std::min(1.f, r / R);
					cp1.second[h] = cp1.second[h] * w + p1 * cp1.first * conicity * (1.f - w);
					cp1.second[h].z = z;
				}
			}
		}
	}

	//glue faces
	for (auto& SF : sharedFacesMap)
	{
		auto& cp0 = newCutoutPoints.find(SF.first.first)->second;
		auto& cp1 = newCutoutPoints.find(SF.first.second)->second;
		auto& v = SF.second.vertices;
		float invW = 1.f / SF.second.w;

		for (uint32_t w = 0; w <= SF.second.w; w++)
		{
			for (uint32_t h = 0; h <= SF.second.h; h++)
			{
				v[w * (SF.second.h + 1) + h].p += ((cp0.second[h] / cp0.first - v[h].p) * (SF.second.w - w)
					+ (cp1.second[h] / cp1.first - v[SF.second.w * (SF.second.h + 1) + h].p) * w) * invW;
			}
		}
	}
}

Mesh* getNoisyCuttingCone(const std::vector<physx::PxVec3>& points, const std::set<int32_t>& smoothingGroups,
	const physx::PxTransform& transform, bool useSmoothing, float heightBot, float heightTop, float conicityMultiplierBot, float conicityMultiplierTop, 
	const physx::PxVec3* samplingInterval, uint32_t interiorMaterialId, const SharedFacesMap& sharedFacesMap, bool inverseNormals)
{
	uint32_t pointCount = points.size();
	uint32_t resP = pointCount;
	uint32_t resH = 1;
	if (samplingInterval != nullptr)
	{
		for (uint32_t i = 0; i < pointCount; i++)
		{
			auto vec = (points[(i + 1) % pointCount] - points[i]);
			resP += (uint32_t)(std::abs(vec.x) / samplingInterval->x + std::abs(vec.y) / samplingInterval->y);
		}
		resH = std::max((uint32_t)std::roundf((heightBot + heightTop) / samplingInterval->z), 1u);
	}

	std::vector<Vertex> positions; positions.reserve((resH + 1) * (resP + 1));
	std::vector<Edge> edges; edges.reserve(resH * resP * 6 + (resP + 1) * 2);
	std::vector<Facet> facets; facets.reserve(resH * resP * 2 + 2);

	uint32_t pCount = 0;
	int sg = useSmoothing ? 1 : -1;
	for (uint32_t p = 0; p < pointCount; p++)
	{
		if (useSmoothing && smoothingGroups.find(p) != smoothingGroups.end())
		{
			sg = sg ^ 3;
		}
		auto p0 = points[p];
		auto p1 = points[(p + 1) % pointCount];

		uint32_t firstVertexIndex = positions.size();
		uint32_t firstEdgeIndex = edges.size();

		auto sfIt = sharedFacesMap.find(std::make_pair(p0, p1));
		int32_t vBegin = 0, vEnd = -1, vIncr = 1;
		if (sfIt == sharedFacesMap.end())
		{
			sfIt = sharedFacesMap.find(std::make_pair(p1, p0));;
			vBegin = sfIt->second.w;
			vIncr = -1;
		}
		else
		{
			vEnd = sfIt->second.w + 1;
		}

		auto& SF = sfIt->second;
		positions.resize(firstVertexIndex + (SF.w + 1) * (SF.h + 1));
		if (vBegin < vEnd)
		{
			for (auto& e : SF.edges)
			{
				edges.push_back(Edge(e.s + firstVertexIndex, e.e + firstVertexIndex));
			}
			for (auto& f : SF.facets)
			{
				facets.push_back(f);
				facets.back().firstEdgeNumber += firstEdgeIndex;
				facets.back().smoothingGroup = sg;
			}
		}
		else
		{
			fillEdgesAndFaces(edges, facets, SF.h, SF.w, firstVertexIndex, positions.size(), SF.f.userData, SF.f.materialId, sg, true);
		}
		for (int32_t v = vBegin; v != vEnd; v += vIncr)
		{
			std::copy(SF.vertices.begin() + v * (resH + 1), SF.vertices.begin() + (v + 1) * (SF.h + 1), positions.begin() + firstVertexIndex);
			firstVertexIndex += SF.h + 1;
		}
		pCount += SF.vertices.size() / (resH + 1) - 1;
	}
	
	if (inverseNormals)
	{
		for (uint32_t e = 0; e < edges.size(); e += 3)
		{
			std::swap(edges[e + 0].s, edges[e + 0].e);
			std::swap(edges[e + 1].s, edges[e + 1].e);
			std::swap(edges[e + 2].s, edges[e + 2].e);
			std::swap(edges[e + 0], edges[e + 2]);
		}
	}

	uint32_t totalCount = pCount + pointCount;
	calculateNormals(positions, resH, totalCount - 1, inverseNormals);

	std::vector<float> xPos, yPos;
	int32_t ii = 0;
	for (auto& p : positions)
	{
		if ((ii++) % (resH + 1) == 1)
		{
			xPos.push_back(p.p.x);
			yPos.push_back(p.p.y);
		}
		p.p = transform.transform(p.p);
		p.n = transform.rotate(p.n);
	}
	totalCount /= 2;

	for (uint32_t i = 0; i < totalCount; i++)
	{
		uint32_t idx = 2 * i  * (resH + 1);
		edges.push_back(Edge(idx, (idx + 2 * (resH + 1)) % positions.size()));
	}
	for (int32_t i = totalCount; i > 0; i--)
	{
		uint32_t idx = (2 * i + 1) * (resH + 1) - 1;
		edges.push_back(Edge(idx % positions.size(), idx - 2 * (resH + 1)));
	}

	if (smoothingGroups.find(0) != smoothingGroups.end() || smoothingGroups.find(pointCount - 1) != smoothingGroups.end())
	{
		if (facets[0].smoothingGroup == facets[facets.size() - 1].smoothingGroup)
		{
			for (uint32_t i = 0; i < resH; i++)
			{
				facets[i].smoothingGroup = 4;
			}
		}
	}

	facets.push_back(Facet(resH * pCount * 6, totalCount, interiorMaterialId, 0, -1));
	facets.push_back(Facet(resH * pCount * 6 + totalCount, totalCount, interiorMaterialId, 0, -1));
	return new MeshImpl(positions.data(), edges.data(), facets.data(), static_cast<uint32_t>(positions.size()), static_cast<uint32_t>(edges.size()), static_cast<uint32_t>(facets.size()));
}

Mesh* getCuttingCone(const CutoutConfiguration& conf, const std::vector<physx::PxVec3>& points, const std::set<int32_t>& smoothingGroups,
	float heightBot, float heightTop, float conicityBot, float conicityTop,
	int64_t& id, int32_t seed, int32_t interiorMaterialId, const SharedFacesMap& sharedFacesMap, bool inverseNormals)
{
	if (conf.noise.amplitude > FLT_EPSILON)
	{
		return getNoisyCuttingCone(points, smoothingGroups, conf.transform, conf.useSmoothing, heightBot, heightTop, conicityBot, conicityTop,
			&conf.noise.samplingInterval, interiorMaterialId, sharedFacesMap, inverseNormals);
	}
	else
	{
		return getNoisyCuttingCone(points, smoothingGroups, conf.transform, conf.useSmoothing, heightBot, heightTop, conicityBot, conicityTop,
			nullptr, interiorMaterialId, sharedFacesMap, inverseNormals);
	}
}

} // namespace Blast
} // namespace Nv
