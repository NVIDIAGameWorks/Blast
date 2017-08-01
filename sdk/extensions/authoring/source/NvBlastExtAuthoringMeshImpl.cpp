/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#define _CRT_SECURE_NO_WARNINGS

#include "NvBlastExtAuthoringMeshImpl.h"
#include "NvBlastExtAuthoringTypes.h"
#include <string.h>
#include "NvBlastExtAuthoringPerlinNoise.h"
#include <cmath>

using physx::PxVec2;
using physx::PxVec3;
using physx::PxBounds3;

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



void getTangents(PxVec3& normal, PxVec3& t1, PxVec3& t2)
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

Mesh* getCuttingBox(const PxVec3& point, const PxVec3& normal, float size, int32_t id)
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
	positions[1].uv[0] = PxVec2(10, 0);

	positions[2].uv[0] = PxVec2(10, 10);
	positions[3].uv[0] = PxVec2(0, 10);


	positions[4].uv[0] = PxVec2(0, 0);
	positions[5].uv[0] = PxVec2(10, 0);

	positions[6].uv[0] = PxVec2(10, 10);
	positions[7].uv[0] = PxVec2(0, 10);


	std::vector<Edge> edges;
	std::vector<Facet> facets;

	edges.push_back(Edge(0, 1));
	edges.push_back(Edge(1, 2));
	edges.push_back(Edge(2, 3));
	edges.push_back(Edge(3, 0));
	facets.push_back(Facet(0, 4, MATERIAL_INTERIOR, id));


	edges.push_back(Edge(0, 3));
	edges.push_back(Edge(3, 7));
	edges.push_back(Edge(7, 4));
	edges.push_back(Edge(4, 0));
	facets.push_back(Facet(4, 4, MATERIAL_INTERIOR, id));

	edges.push_back(Edge(3, 2));
	edges.push_back(Edge(2, 6));
	edges.push_back(Edge(6, 7));
	edges.push_back(Edge(7, 3));
	facets.push_back(Facet(8, 4, MATERIAL_INTERIOR, id));

	edges.push_back(Edge(5, 6));
	edges.push_back(Edge(6, 2));
	edges.push_back(Edge(2, 1));
	edges.push_back(Edge(1, 5));
	facets.push_back(Facet(12, 4, MATERIAL_INTERIOR, id));

	edges.push_back(Edge(4, 5));
	edges.push_back(Edge(5, 1));
	edges.push_back(Edge(1, 0));
	edges.push_back(Edge(0, 4));
	facets.push_back(Facet(16, 4, MATERIAL_INTERIOR, id));

	edges.push_back(Edge(4, 7));
	edges.push_back(Edge(7, 6));
	edges.push_back(Edge(6, 5));
	edges.push_back(Edge(5, 4));
	facets.push_back(Facet(20, 4, MATERIAL_INTERIOR, id));
	return new MeshImpl(positions.data(), edges.data(), facets.data(), static_cast<uint32_t>(positions.size()), static_cast<uint32_t>(edges.size()), static_cast<uint32_t>(facets.size()));
}

void inverseNormalAndSetIndices(Mesh* mesh, int32_t id)
{
	for (uint32_t i = 0; i < mesh->getVerticesCount(); ++i)
	{
		mesh->getVerticesWritable()[i].n *= -1.0f;
	}
	for (uint32_t i = 0; i < mesh->getFacetCount(); ++i)
	{
		mesh->getFacetWritable(i)->userData = id;
	}
}

void MeshImpl::setMaterialId(int32_t* materialId)
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

void MeshImpl::setSmoothingGroup(int32_t* smoothingGroup)
{
	if (smoothingGroup != nullptr)
	{
		for (uint32_t i = 0; i < mFacets.size(); ++i)
		{
			mFacets[i].smoothingGroup = *smoothingGroup;
			++smoothingGroup;
		}
	}
}


void setCuttingBox(const PxVec3& point, const PxVec3& normal, Mesh* mesh, float size, int32_t id)
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

Mesh* getNoisyCuttingBoxPair(const physx::PxVec3& point, const physx::PxVec3& normal, float size, float jaggedPlaneSize, uint32_t resolution,  int32_t id, float amplitude, float frequency, int32_t octaves, int32_t seed)
{
	SimplexNoise nEval(amplitude, frequency, octaves, seed);
	PxVec3 t1, t2;
	PxVec3 lNormal = normal.getNormalized();
	getTangents(lNormal, t1, t2);

	std::vector<Vertex> vertices ((resolution + 1) * (resolution + 1) + 12);
	PxVec3 cPosit = point + (t1 + t2) * jaggedPlaneSize;
	PxVec3 t1d = -t1 * 2.0f * jaggedPlaneSize / resolution;
	PxVec3 t2d = -t2 * 2.0f * jaggedPlaneSize / resolution;

	int32_t vrtId = 0;
	for (uint32_t i = 0; i < resolution + 1; ++i)
	{
		PxVec3 lcPosit = cPosit;
		for (uint32_t j = 0; j < resolution + 1; ++j)
		{
			vertices[vrtId].p = lcPosit;
			lcPosit += t1d;
			vrtId++;
		}
		cPosit += t2d;
	}

	
	for (uint32_t i = 1; i < resolution; ++i)
	{
		for (uint32_t j = 1; j < resolution; ++j)
		{
			PxVec3& pnt = vertices[i * (resolution + 1) + j].p;
			pnt += lNormal * nEval.sample(pnt);
		}
	}

	std::vector<Edge> edges;
	std::vector<Facet> facets;
	for (uint32_t i = 0; i < resolution; ++i)
	{
		for (uint32_t j = 0; j < resolution; ++j)
		{
			uint32_t start = edges.size();
			edges.push_back(Edge(i * (resolution + 1) + j, i * (resolution + 1) + j + 1));
			edges.push_back(Edge(i * (resolution + 1) + j + 1, (i + 1) * (resolution + 1) + j + 1));
			edges.push_back(Edge((i + 1) * (resolution + 1) + j + 1, (i + 1) * (resolution + 1) + j));
			edges.push_back(Edge((i + 1) * (resolution + 1) + j, i * (resolution + 1) + j));
			facets.push_back(Facet(start, 4, MATERIAL_INTERIOR, id));
		}
	}
	uint32_t offset = (resolution + 1) * (resolution + 1);

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

	for (uint32_t i = 1; i < resolution; ++i)
	{
		for (uint32_t j = 1; j < resolution; ++j)
		{
			PxVec3 v1 = vertices[(resolution + 1) * (i + 1) + j].p - vertices[(resolution + 1) * i + j].p;
			PxVec3 v2 = vertices[(resolution + 1) * (i) + j + 1].p - vertices[(resolution + 1) * i + j].p;
			PxVec3 v3 = vertices[(resolution + 1) * (i - 1) + j].p - vertices[(resolution + 1) * i + j].p;
			PxVec3 v4 = vertices[(resolution + 1) * (i) + j - 1].p - vertices[(resolution + 1) * i + j].p;

			vertices[(resolution + 1) * i + j].n = v1.cross(v2) + v2.cross(v3) + v3.cross(v4) + v4.cross(v1);
			vertices[(resolution + 1) * i + j].n.normalize();
		}
	}
	
	int32_t edgeOffset = edges.size();
	edges.push_back(Edge(0 + offset, 1 + offset));
	edges.push_back(Edge(1 + offset, 2 + offset));
	edges.push_back(Edge(2 + offset, 3 + offset));
	edges.push_back(Edge(3 + offset, 0 + offset));

	edges.push_back(Edge(11 + offset, 10 + offset));
	edges.push_back(Edge(10 + offset, 9 + offset));
	edges.push_back(Edge(9 + offset, 8 + offset));
	edges.push_back(Edge(8 + offset, 11 + offset));

	facets.push_back(Facet(edgeOffset, 8, MATERIAL_INTERIOR, id));



	edges.push_back(Edge(0 + offset, 3 + offset));
	edges.push_back(Edge(3 + offset, 7 + offset));
	edges.push_back(Edge(7 + offset, 4 + offset));
	edges.push_back(Edge(4 + offset, 0 + offset));
	facets.push_back(Facet(8 + edgeOffset, 4, MATERIAL_INTERIOR, id));

	edges.push_back(Edge(3 + offset, 2 + offset));
	edges.push_back(Edge(2 + offset, 6 + offset));
	edges.push_back(Edge(6 + offset, 7 + offset));
	edges.push_back(Edge(7 + offset, 3 + offset));
	facets.push_back(Facet(12 + edgeOffset, 4, MATERIAL_INTERIOR, id));

	edges.push_back(Edge(5 + offset, 6 + offset));
	edges.push_back(Edge(6 + offset, 2 + offset));
	edges.push_back(Edge(2 + offset, 1 + offset));
	edges.push_back(Edge(1 + offset, 5 + offset));
	facets.push_back(Facet(16 + edgeOffset, 4, MATERIAL_INTERIOR, id));

	edges.push_back(Edge(4 + offset, 5 + offset));
	edges.push_back(Edge(5 + offset, 1 + offset));
	edges.push_back(Edge(1 + offset, 0 + offset));
	edges.push_back(Edge(0 + offset, 4 + offset));
	facets.push_back(Facet(20 + edgeOffset, 4, MATERIAL_INTERIOR, id));

	edges.push_back(Edge(4 + offset, 7 + offset));
	edges.push_back(Edge(7 + offset, 6 + offset));
	edges.push_back(Edge(6 + offset, 5 + offset));
	edges.push_back(Edge(5 + offset, 4 + offset));
	facets.push_back(Facet(24 + edgeOffset, 4, MATERIAL_INTERIOR, id));

	//
	return new MeshImpl(vertices.data(), edges.data(), facets.data(), vertices.size(), edges.size(), facets.size());
}

Mesh* getBigBox(const PxVec3& point, float size)
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
	positions[1].uv[0] = PxVec2(10, 0);

	positions[2].uv[0] = PxVec2(10, 10);
	positions[3].uv[0] = PxVec2(0, 10);


	positions[4].uv[0] = PxVec2(0, 0);
	positions[5].uv[0] = PxVec2(10, 0);

	positions[6].uv[0] = PxVec2(10, 10);
	positions[7].uv[0] = PxVec2(0, 10);


	std::vector<Edge> edges;
	std::vector<Facet> facets;

	edges.push_back(Edge(0, 1));
	edges.push_back(Edge(1, 2));
	edges.push_back(Edge(2, 3));
	edges.push_back(Edge(3, 0));
	facets.push_back(Facet(0, 4, MATERIAL_INTERIOR, 0));


	edges.push_back(Edge(0, 3));
	edges.push_back(Edge(3, 7));
	edges.push_back(Edge(7, 4));
	edges.push_back(Edge(4, 0));
	facets.push_back(Facet(4, 4, MATERIAL_INTERIOR, 0));

	edges.push_back(Edge(3, 2));
	edges.push_back(Edge(2, 6));
	edges.push_back(Edge(6, 7));
	edges.push_back(Edge(7, 3));
	facets.push_back(Facet(8, 4, MATERIAL_INTERIOR, 0));

	edges.push_back(Edge(5, 6));
	edges.push_back(Edge(6, 2));
	edges.push_back(Edge(2, 1));
	edges.push_back(Edge(1, 5));
	facets.push_back(Facet(12, 4, MATERIAL_INTERIOR, 0));

	edges.push_back(Edge(4, 5));
	edges.push_back(Edge(5, 1));
	edges.push_back(Edge(1, 0));
	edges.push_back(Edge(0, 4));
	facets.push_back(Facet(16, 4, MATERIAL_INTERIOR, 0));

	edges.push_back(Edge(4, 7));
	edges.push_back(Edge(7, 6));
	edges.push_back(Edge(6, 5));
	edges.push_back(Edge(5, 4));
	facets.push_back(Facet(20, 4, MATERIAL_INTERIOR, 0));
	for (int i = 0; i < 8; ++i)
		positions[i].n = PxVec3(0, 0, 0);
	return new MeshImpl(positions.data(), edges.data(), facets.data(), static_cast<uint32_t>(positions.size()), static_cast<uint32_t>(edges.size()), static_cast<uint32_t>(facets.size()));
}

} // namespace Blast
} // namespace Nv
