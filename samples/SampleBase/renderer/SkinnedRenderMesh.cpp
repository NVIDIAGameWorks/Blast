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


#include "SkinnedRenderMesh.h"
#include "Renderer.h"

SkinnedRenderMesh::SkinnedRenderMesh(const std::vector<const SimpleMesh*>& meshes)
{
	PX_ASSERT_WITH_MESSAGE(meshes.size() <= MeshesCountMax, "meshes.size() have to be <= SkinnedRenderMesh::MeshesCountMax");

	m_device = GetDeviceManager()->GetDevice();

	// input element desc setup
	m_inputDesc.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 });
	m_inputDesc.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 });
	m_inputDesc.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 });
	m_inputDesc.push_back({ "TEXCOORD", 1, DXGI_FORMAT_R32_UINT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 });

	// reserve VB
	uint32_t verticesTotal = 0;
	std::for_each(meshes.begin(), meshes.end(), [&](const SimpleMesh* c) { verticesTotal += (uint32_t)c->vertices.size(); });
	std::vector<SimpleMesh::Vertex> vertexBuffer;
	vertexBuffer.reserve(verticesTotal);

	// reserve IB 
	uint32_t indicesTotal = 0;
	std::for_each(meshes.begin(), meshes.end(), [&](const SimpleMesh* c) { indicesTotal += (uint32_t)c->indices.size(); });
	m_indices.reserve(indicesTotal);

	// fill VB, IB, MeshInfo
	m_meshesInfo.resize(meshes.size());
	for (uint32_t meshIndex = 0; meshIndex < meshes.size(); ++meshIndex)
	{
		const SimpleMesh* mesh = meshes[meshIndex];
		MeshInfo& meshInfo = m_meshesInfo[meshIndex];

		meshInfo.firstVertex = (uint32_t)vertexBuffer.size();
		vertexBuffer.insert(vertexBuffer.end(), mesh->vertices.begin(), mesh->vertices.end());
		meshInfo.verticesCount = (uint32_t)mesh->vertices.size();

		meshInfo.firstIndex = (uint32_t)m_indices.size();
		uint32_t indexOffset = meshInfo.firstVertex;
		for (uint32_t index : mesh->indices)
		{
			m_indices.push_back((uint32_t)index + indexOffset);
		}
		meshInfo.indicesCount = (uint32_t)mesh->indices.size();
	}

	// vertex buffer
	{
		D3D11_SUBRESOURCE_DATA vertexBufferData;

		ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
		vertexBufferData.pSysMem = vertexBuffer.data();

		D3D11_BUFFER_DESC bufferDesc;

		memset(&bufferDesc, 0, sizeof(D3D11_BUFFER_DESC));
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.ByteWidth = (uint32_t)(sizeof(SimpleMesh::Vertex) * vertexBuffer.size());
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;
		bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;

		V(m_device->CreateBuffer(&bufferDesc, &vertexBufferData, &m_vertexBuffer));
	}

	// bone index buffer
	{
		D3D11_BUFFER_DESC bufferDesc;

		memset(&bufferDesc, 0, sizeof(D3D11_BUFFER_DESC));
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.ByteWidth = (uint32_t)(sizeof(uint32_t) * vertexBuffer.size());
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.MiscFlags = 0;
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;

		V(m_device->CreateBuffer(&bufferDesc, nullptr, &m_boneIndexBuffer));
	}

	// index buffer
	{
		D3D11_BUFFER_DESC bufferDesc;

		memset(&bufferDesc, 0, sizeof(D3D11_BUFFER_DESC));
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufferDesc.ByteWidth = (uint32_t)(sizeof(uint32_t) * m_indices.size());
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.MiscFlags = 0;
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;

		V(m_device->CreateBuffer(&bufferDesc, nullptr, &m_indexBuffer));
	}

	// bone texture
	{
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = 4;
		desc.Height = (uint32_t)meshes.size();
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		V(m_device->CreateTexture2D(&desc, nullptr, &m_boneTexture));
	}

	// bone texture SRV
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipLevels = 1;
		desc.Texture2D.MostDetailedMip = 0;
		V(m_device->CreateShaderResourceView(m_boneTexture, &desc, &m_boneTextureSRV));
	}
}

SkinnedRenderMesh::~SkinnedRenderMesh()
{
	SAFE_RELEASE(m_vertexBuffer);
	SAFE_RELEASE(m_boneIndexBuffer);
	SAFE_RELEASE(m_indexBuffer);
	SAFE_RELEASE(m_boneTexture);
	SAFE_RELEASE(m_boneTextureSRV);
}

void SkinnedRenderMesh::updateVisibleMeshes(const std::vector<uint32_t>& visibleMeshes)
{
	ID3D11DeviceContext* context;
	m_device->GetImmediateContext(&context);

	// update bone index buffer
	{
		D3D11_MAPPED_SUBRESOURCE mappedRead;
		V(context->Map(m_boneIndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedRead));

		uint32_t* boneIndexBuffer = (uint32_t*)mappedRead.pData;
		for (uint32_t i = 0; i < visibleMeshes.size(); ++i)
		{
			const MeshInfo& info = m_meshesInfo[visibleMeshes[i]];
			for (uint32_t v = info.firstVertex; v < info.firstVertex + info.verticesCount; ++v)
			{
				boneIndexBuffer[v] = i;
			}
		}

		context->Unmap(m_boneIndexBuffer, 0);
	}

	// update index buffer
	{
		D3D11_MAPPED_SUBRESOURCE mappedRead;
		V(context->Map(m_indexBuffer, 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedRead));

		uint32_t* indexBuffer = (uint32_t*)mappedRead.pData;
		uint32_t indexCount = 0;
		for (uint32_t meshIndex : visibleMeshes)
		{
			const MeshInfo& info = m_meshesInfo[meshIndex];
			memcpy(indexBuffer + indexCount, &m_indices[info.firstIndex], info.indicesCount * sizeof(uint32_t));
			indexCount += info.indicesCount;
		}
		context->Unmap(m_indexBuffer, 0);
		m_indexCount = indexCount;
		PX_ASSERT(m_indexCount % 3 == 0);
	}
}

void SkinnedRenderMesh::updateVisibleMeshTransforms(std::vector<PxMat44>& transforms)
{
	ID3D11DeviceContext* context;
	m_device->GetImmediateContext(&context);

	// update bone transform texture
	{
		D3D11_MAPPED_SUBRESOURCE mappedRead;
		V(context->Map(m_boneTexture, 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedRead));
		for (uint32_t i = 0; i < transforms.size(); ++i)
		{
			std::memcpy((uint8_t*)mappedRead.pData + i * mappedRead.RowPitch, &transforms[i], sizeof(PxMat44));
		}
		context->Unmap(m_boneTexture, 0);
	}
}

void SkinnedRenderMesh::render(ID3D11DeviceContext& context) const
{
	context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT strides[2] = { sizeof(SimpleMesh::Vertex), sizeof(uint32_t) };
	UINT offsets[2] = { 0 };
	ID3D11Buffer* buffers[2] = { m_vertexBuffer, m_boneIndexBuffer };
	context.IASetVertexBuffers(0, 2, buffers, strides, offsets);

	context.IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	context.VSSetShaderResources(1, 1, &m_boneTextureSRV);

	context.DrawIndexed(m_indexCount, 0, 0);
}