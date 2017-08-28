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
// Copyright (c) 2008-2017 NVIDIA Corporation. All rights reserved.


#include "BlastFamilyModelSimple.h"
#include "RenderUtils.h"
#include "DeviceManager.h"
#include "Renderer.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlastExtPxActor.h"
#include "NvBlastTkActor.h"
#include "NvBlastTkAsset.h"
#include "PxRigidDynamic.h"

using namespace physx;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												SimpleRenderMesh
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class SimpleRenderMesh : public IRenderMesh
{
public:
	SimpleRenderMesh(const SimpleMesh* mesh) : m_mesh(mesh)
	{
		m_device = GetDeviceManager()->GetDevice();

		m_inputDesc.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 });
		m_inputDesc.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 });
		m_inputDesc.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 });
		m_inputDesc.push_back({ "TEXCOORD", 1, DXGI_FORMAT_R32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 });

		m_numVertices = static_cast<uint32_t>(mesh->vertices.size());
		m_numFaces = static_cast<uint32_t>(mesh->indices.size());

		// VB
		{
			D3D11_SUBRESOURCE_DATA vertexBufferData;
			ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
			vertexBufferData.pSysMem = mesh->vertices.data();

			D3D11_BUFFER_DESC bufferDesc;
			memset(&bufferDesc, 0, sizeof(D3D11_BUFFER_DESC));
			bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bufferDesc.ByteWidth = sizeof(SimpleMesh::Vertex) * m_numVertices;
			bufferDesc.CPUAccessFlags = 0;
			bufferDesc.MiscFlags = 0;
			bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;

			V(m_device->CreateBuffer(&bufferDesc, &vertexBufferData, &m_vertexBuffer));
		}

		// Health Buffer
		{
			// fill with 1.0f initially
			std::vector<float> healths(mesh->vertices.size());
			std::fill(healths.begin(), healths.end(), 1.0f);

			D3D11_SUBRESOURCE_DATA vertexBufferData;
			ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
			vertexBufferData.pSysMem = healths.data();

			D3D11_BUFFER_DESC bufferDesc;
			memset(&bufferDesc, 0, sizeof(D3D11_BUFFER_DESC));
			bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bufferDesc.ByteWidth = (uint32_t)(sizeof(float) * m_numVertices);
			bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bufferDesc.MiscFlags = 0;
			bufferDesc.Usage = D3D11_USAGE_DYNAMIC;

			V(m_device->CreateBuffer(&bufferDesc, &vertexBufferData, &m_healthBuffer));
		}

		// IB
		if (m_numFaces)
		{
			D3D11_SUBRESOURCE_DATA indexBufferData;

			ZeroMemory(&indexBufferData, sizeof(indexBufferData));
			indexBufferData.pSysMem = mesh->indices.data();

			D3D11_BUFFER_DESC bufferDesc;

			memset(&bufferDesc, 0, sizeof(D3D11_BUFFER_DESC));
			bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			bufferDesc.ByteWidth = sizeof(uint32_t) * m_numFaces;
			bufferDesc.CPUAccessFlags = 0;
			bufferDesc.MiscFlags = 0;
			bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;

			V(m_device->CreateBuffer(&bufferDesc, &indexBufferData, &m_indexBuffer));
		}
	}

	~SimpleRenderMesh()
	{
		SAFE_RELEASE(m_healthBuffer);
		SAFE_RELEASE(m_vertexBuffer);
		SAFE_RELEASE(m_indexBuffer);
	}


	void render(ID3D11DeviceContext& context) const
	{
		context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		UINT strides[2] = { sizeof(SimpleMesh::Vertex), sizeof(uint32_t) };
		UINT offsets[2] = { 0 };
		ID3D11Buffer* buffers[2] = { m_vertexBuffer, m_healthBuffer };
		context.IASetVertexBuffers(0, 2, buffers, strides, offsets);


		context.IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		if (m_indexBuffer)
			context.DrawIndexed(m_numFaces, 0, 0);
		else
			context.Draw(m_numVertices, 0);
	}

	const std::vector<D3D11_INPUT_ELEMENT_DESC>& getInputElementDesc() const { return m_inputDesc; }
	
	const SimpleMesh* getMesh() { return m_mesh; }

	void updateHealths(const std::vector<float>& healths)
	{
		ID3D11DeviceContext* context;
		m_device->GetImmediateContext(&context);

		// update buffer
		{
			D3D11_MAPPED_SUBRESOURCE mappedRead;
			V(context->Map(m_healthBuffer, 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedRead));
			memcpy(mappedRead.pData, healths.data(), sizeof(float) * healths.size());
			context->Unmap(m_healthBuffer, 0);
		}

	}


private:

	ID3D11Device* m_device;

	ID3D11Buffer* m_vertexBuffer;
	ID3D11Buffer* m_healthBuffer;
	ID3D11Buffer* m_indexBuffer;
	uint32_t      m_numFaces;
	uint32_t      m_numVertices;

	std::vector<D3D11_INPUT_ELEMENT_DESC> m_inputDesc;

	const SimpleMesh* m_mesh;
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//											BlastFamilyModelSimple
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BlastFamilyModelSimple::BlastFamilyModelSimple(PhysXController& physXController, ExtPxManager& pxManager, Renderer& renderer, const BlastAssetModelSimple& blastAsset, const BlastAsset::ActorDesc& desc)
	: BlastFamily(physXController, pxManager, blastAsset), m_renderer(renderer)
{
	// materials
	auto materials = blastAsset.getRenderMaterials();

	// model
	const BlastModel& model = blastAsset.getModel();

	// create render mesh for every BlastModel::Chunk::Mesh and renderable with it
	const std::vector<BlastModel::Chunk>& modelChunks = model.chunks;
	m_chunks.resize(modelChunks.size());
	for (uint32_t chunkIndex = 0; chunkIndex < modelChunks.size(); chunkIndex++)
	{
		const std::vector<BlastModel::Chunk::Mesh>& meshes = modelChunks[chunkIndex].meshes;
		std::vector<SimpleRenderMesh*>& renderMeshes = m_chunks[chunkIndex].renderMeshes;
		std::vector<Renderable*>& renderables = m_chunks[chunkIndex].renderables;
		renderMeshes.resize(meshes.size());
		renderables.resize(meshes.size());
		for (uint32_t i = 0; i < meshes.size(); i++)
		{
			renderMeshes[i] = new SimpleRenderMesh(&meshes[i].mesh);

			uint32_t materialIndex = model.chunks[chunkIndex].meshes[i].materialIndex;
			Renderable* renderable = renderer.createRenderable(*renderMeshes[i], *materials[materialIndex]);
			renderable->setHidden(true);
			renderables[i] = renderable;

		}
	}

	// initialize in position
	initialize(desc);
}

BlastFamilyModelSimple::~BlastFamilyModelSimple()
{
	// release all chunks
	for (uint32_t chunkIndex = 0; chunkIndex < m_chunks.size(); chunkIndex++)
	{
		std::vector<Renderable*>& renderables = m_chunks[chunkIndex].renderables;
		for (uint32_t i = 0; i < m_chunks[chunkIndex].renderables.size(); i++)
		{
			m_renderer.removeRenderable(m_chunks[chunkIndex].renderables[i]);
			SAFE_DELETE(m_chunks[chunkIndex].renderMeshes[i]);
		}
	}
}

void BlastFamilyModelSimple::onActorCreated(const ExtPxActor& actor)
{
	// separate color for every material
	std::vector<DirectX::XMFLOAT4> colors;

	const uint32_t* chunkIndices = actor.getChunkIndices();
	uint32_t chunkCount = actor.getChunkCount();
	for (uint32_t i = 0; i < chunkCount; i++)
	{
		uint32_t chunkIndex = chunkIndices[i];
		std::vector<Renderable*>& renderables = m_chunks[chunkIndex].renderables;
		for (uint32_t r = 0; r < renderables.size(); r++)
		{
			if (colors.size() <= r)
				colors.push_back(getRandomPastelColor());

			renderables[r]->setHidden(false);
			renderables[r]->setColor(colors[r]);
		}
	}
}

void BlastFamilyModelSimple::onActorUpdate(const ExtPxActor& actor)
{
	const ExtPxChunk* chunks = m_blastAsset.getPxAsset()->getChunks();
	const ExtPxSubchunk* subChunks = m_blastAsset.getPxAsset()->getSubchunks();
	const uint32_t* chunkIndices = actor.getChunkIndices();
	uint32_t chunkCount = actor.getChunkCount();
	for (uint32_t i = 0; i < chunkCount; i++)
	{
		uint32_t chunkIndex = chunkIndices[i];
		std::vector<Renderable*>& renderables = m_chunks[chunkIndex].renderables;
		for (Renderable* r : renderables)
		{
			r->setTransform(actor.getPhysXActor().getGlobalPose() * subChunks[chunks[chunkIndex].firstSubchunkIndex].transform);
		}
	}
}

void BlastFamilyModelSimple::onActorDestroyed(const ExtPxActor& actor)
{
	const uint32_t* chunkIndices = actor.getChunkIndices();
	uint32_t chunkCount = actor.getChunkCount();
	for (uint32_t i = 0; i < chunkCount; i++)
	{
		uint32_t chunkIndex = chunkIndices[i];
		std::vector<Renderable*>& renderables = m_chunks[chunkIndex].renderables;
		for (Renderable* r : renderables)
		{
			r->setHidden(true);
		}
	}
}

void BlastFamilyModelSimple::onActorHealthUpdate(const ExtPxActor& actor)
{
	TkActor& tkActor = actor.getTkActor();
	const TkAsset* tkAsset = tkActor.getAsset();

	const float* bondHealths = tkActor.getBondHealths();
	uint32_t nodeCount = tkActor.getGraphNodeCount();
	if (nodeCount == 0) // subsupport chunks don't have graph nodes
		return;

	std::vector<uint32_t> nodes(tkActor.getGraphNodeCount());
	tkActor.getGraphNodeIndices(nodes.data(), static_cast<uint32_t>(nodes.size()));

	const NvBlastChunk* chunks = tkAsset->getChunks();
	const NvBlastBond* bonds = tkAsset->getBonds();

	const NvBlastSupportGraph graph = tkAsset->getGraph();
	const float bondHealthMax = m_blastAsset.getBondHealthMax();

	std::vector<float> healthBuffer;

	for (uint32_t node0 : nodes)
	{
		uint32_t chunkIndex = graph.chunkIndices[node0];

		if (chunkIndex >= m_chunks.size())
			continue;

		std::vector<SimpleRenderMesh*>& meshes = m_chunks[chunkIndex].renderMeshes;
		const auto& renderables = m_chunks[chunkIndex].renderables;
		for (uint32_t i = 0; i < meshes.size(); ++i)
		{
			if(renderables[i]->isHidden())
				continue;

			SimpleRenderMesh* renderMesh = meshes[i];

			const SimpleMesh* mesh = renderMesh->getMesh();
			healthBuffer.resize(mesh->vertices.size());
			
			for (uint32_t vertexIndex = 0; vertexIndex < mesh->vertices.size(); vertexIndex++)
			{
				PxVec3 position = mesh->vertices[vertexIndex].position;
				float health = 0.0f;
				float healthDenom = 0.0f;

				for (uint32_t adjacencyIndex = graph.adjacencyPartition[node0]; adjacencyIndex < graph.adjacencyPartition[node0 + 1]; adjacencyIndex++)
				{
					uint32_t node1 = graph.adjacentNodeIndices[adjacencyIndex];
					uint32_t bondIndex = graph.adjacentBondIndices[adjacencyIndex];
					float bondHealth = PxClamp(bondHealths[bondIndex] / bondHealthMax, 0.0f, 1.0f);
					const NvBlastBond& solverBond = bonds[bondIndex];
					const PxVec3& centroid = reinterpret_cast<const PxVec3&>(solverBond.centroid);
					
					float factor = 1.0f / (centroid - position).magnitudeSquared();

					health += bondHealth * factor;
					healthDenom += factor;
				}

				healthBuffer[vertexIndex] = healthDenom > 0.0f ? health / healthDenom : 1.0f;
			}

			renderMesh->updateHealths(healthBuffer);
		}
	}
}
