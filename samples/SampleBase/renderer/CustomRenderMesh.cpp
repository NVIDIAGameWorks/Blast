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


#include "CustomRenderMesh.h"


CustomRenderMesh::CustomRenderMesh()
	: m_indexBuffer(nullptr)
{
}

CustomRenderMesh::CustomRenderMesh(const void* vertices, uint32_t numVertices, uint32_t vertexSize, std::vector<D3D11_INPUT_ELEMENT_DESC>& inputDesc, const uint16_t* faces, uint32_t numFaces)
	: m_indexBuffer(nullptr)
{
	initialize(vertices, numVertices, vertexSize, inputDesc, faces, numFaces);
}

void CustomRenderMesh::initialize(const void* vertices, uint32_t numVertices, uint32_t vertexSize, std::vector<D3D11_INPUT_ELEMENT_DESC>& inputDesc, const uint16_t* faces, uint32_t numFaces)
{
	ID3D11Device* device = GetDeviceManager()->GetDevice();

	m_inputDesc = inputDesc;
	m_numVertices = numVertices;
	m_vertexSize = vertexSize;
	m_numFaces = numFaces;

	// VB
	{
		D3D11_SUBRESOURCE_DATA vertexBufferData;

		ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
		vertexBufferData.pSysMem = vertices;

		D3D11_BUFFER_DESC bufferDesc;

		memset(&bufferDesc, 0, sizeof(D3D11_BUFFER_DESC));
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.ByteWidth = vertexSize * numVertices;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;
		bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;

		V(device->CreateBuffer(&bufferDesc, &vertexBufferData, &m_vertexBuffer));
	}

	// IB
	if (faces != nullptr)
	{
		D3D11_SUBRESOURCE_DATA indexBufferData;

		ZeroMemory(&indexBufferData, sizeof(indexBufferData));
		indexBufferData.pSysMem = faces;

		D3D11_BUFFER_DESC bufferDesc;

		memset(&bufferDesc, 0, sizeof(D3D11_BUFFER_DESC));
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufferDesc.ByteWidth = sizeof(uint16_t) * numFaces;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;
		bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;

		V(device->CreateBuffer(&bufferDesc, &indexBufferData, &m_indexBuffer));
	}
}

CustomRenderMesh::~CustomRenderMesh()
{
	SAFE_RELEASE(m_vertexBuffer);
	SAFE_RELEASE(m_indexBuffer);
}


void CustomRenderMesh::render(ID3D11DeviceContext& context) const
{
	context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT strides[1] = { m_vertexSize };
	UINT offsets[1] = { 0 };
	context.IASetVertexBuffers(0, 1, &m_vertexBuffer, strides, offsets);

	context.IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R16_UINT, 0);

	if (m_indexBuffer)
		context.DrawIndexed(m_numFaces, 0, 0);
	else
		context.Draw(m_numVertices, 0);
}

