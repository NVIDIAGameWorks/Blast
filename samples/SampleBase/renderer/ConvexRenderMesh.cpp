/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "ConvexRenderMesh.h"
#include "Renderer.h"
#include "PxConvexMesh.h"


struct Vertex
{
	PxVec3 position;
	PxVec3 normal;
};

ConvexRenderMesh::ConvexRenderMesh(const PxConvexMesh* mesh)
{
	const uint32_t nbPolygons = mesh->getNbPolygons();
	const uint8_t* indexBuffer = mesh->getIndexBuffer();
	const PxVec3* meshVertices = mesh->getVertices();

	uint32_t nbVerts = 0;
	uint32_t nbFaces = 0;

	for (uint32_t i = 0; i < nbPolygons; i++)
	{
		PxHullPolygon data;
		mesh->getPolygonData(i, data);
		uint32_t nbPolyVerts = data.mNbVerts;
		nbVerts += nbPolyVerts;
		nbFaces += (nbPolyVerts - 2) * 3;
	}

	std::vector<Vertex> vertices;
	std::vector<uint16_t> faces;

	vertices.resize(nbVerts);
	faces.resize(nbFaces);

	uint32_t vertCounter = 0;
	uint32_t facesCounter = 0;
	for (uint32_t i = 0; i < nbPolygons; i++)
	{
		PxHullPolygon data;
		mesh->getPolygonData(i, data);

		PxVec3 normal(data.mPlane[0], data.mPlane[1], data.mPlane[2]);

		uint32_t vI0 = vertCounter;
		for (uint32_t vI = 0; vI < data.mNbVerts; vI++)
		{
			vertices[vertCounter].position = meshVertices[indexBuffer[data.mIndexBase + vI]];
			vertices[vertCounter].normal = normal;
			vertCounter++;
		}

		for (uint32_t vI = 1; vI < uint32_t(data.mNbVerts) - 1; vI++)
		{
			faces[facesCounter++] = uint16_t(vI0);
			faces[facesCounter++] = uint16_t(vI0 + vI + 1);
			faces[facesCounter++] = uint16_t(vI0 + vI);
		}
	}
	
	std::vector<D3D11_INPUT_ELEMENT_DESC> layout;
	layout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 });
	layout.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 });

	initialize(vertices.data(), (uint32_t)vertices.size(), sizeof(Vertex), layout, faces.data(), nbFaces);
}


ConvexRenderMesh::~ConvexRenderMesh()
{
}

