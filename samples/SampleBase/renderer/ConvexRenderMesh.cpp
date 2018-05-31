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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.


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

