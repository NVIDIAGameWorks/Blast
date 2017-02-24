/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "BlastFamilyModelSimple.h"
#include "RenderUtils.h"
#include "DeviceManager.h"
#include "Renderer.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlastExtPxActor.h"
#include "NvBlastTkActor.h"
#include "NvBlastTkAsset.h"
#include "PxRigidDynamic.h"
#include "MaterialLibraryPanel.h"

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
			bufferDesc.ByteWidth = sizeof(uint16_t) * m_numFaces;
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


		context.IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R16_UINT, 0);

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

// Add By Lixu Begin
	void setScale(PxMat44 scale, bool replace)
	{
		std::vector<SimpleMesh::Vertex> newVertex(m_numVertices);
		for (int v = 0; v < m_numVertices; v++)
		{
			newVertex[v] = m_mesh->vertices[v];
			newVertex[v].position = scale.transform(newVertex[v].position);
		}

		D3D11_SUBRESOURCE_DATA vertexBufferData;
		ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
		vertexBufferData.pSysMem = newVertex.data();

		D3D11_BUFFER_DESC bufferDesc;
		memset(&bufferDesc, 0, sizeof(D3D11_BUFFER_DESC));
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.ByteWidth = sizeof(SimpleMesh::Vertex) * m_numVertices;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;
		bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;

		ID3D11Buffer* pBuffer = m_vertexBuffer;
		V(m_device->CreateBuffer(&bufferDesc, &vertexBufferData, &m_vertexBuffer));
		if (NULL != pBuffer)
		{
			pBuffer->Release();
			pBuffer = NULL;
		}

		if (replace)
		{
			memcpy((void*)m_mesh->vertices.data(), newVertex.data(), bufferDesc.ByteWidth);
		}
	}
// Add By Lixu End

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
// Add By Lixu Begin
		std::vector<Renderable*>& ex_Renderables = m_chunks[chunkIndex].ex_Renderables;
		ex_Renderables.clear();
		std::vector<Renderable*>& in_Renderables = m_chunks[chunkIndex].in_Renderables;
		in_Renderables.clear();
// Add By Lixu End
		renderMeshes.resize(meshes.size());
		renderables.resize(meshes.size());
		for (uint32_t i = 0; i < meshes.size(); i++)
		{
			renderMeshes[i] = new SimpleRenderMesh(&meshes[i].mesh);

			uint32_t materialIndex = model.chunks[chunkIndex].meshes[i].materialIndex;
			Renderable* renderable = renderer.createRenderable(*renderMeshes[i], *materials[materialIndex]);
			renderable->setHidden(true);
			renderables[i] = renderable;
// Add By Lixu Begin
			if (materialIndex == 0)
			{
				ex_Renderables.push_back(renderable);
			}
			else if (materialIndex == 1)
			{
				in_Renderables.push_back(renderable);
			}
// Add By Lixu End
		}
	}

	// initialize in position
	initialize(desc);
}

BlastFamilyModelSimple::~BlastFamilyModelSimple()
{
// Add By Lixu Begin
	// disconnect the material and relative renderable
	const BlastAssetModelSimple& blastAsset = *(BlastAssetModelSimple*)&m_blastAsset;
	const std::vector<RenderMaterial*>& materials = blastAsset.getRenderMaterials();
	int materialSize = materials.size();
	for (int ms = 0; ms < materialSize; ms++)
	{
		RenderMaterial* pRenderMaterial = materials[ms];
		pRenderMaterial->clearRelatedRenderables();
	}
	std::map<std::string, RenderMaterial*>& RenderMaterialMap = MaterialLibraryPanel::ins()->getRenderMaterials();
	std::map<std::string, RenderMaterial*>::iterator it;
	for (it = RenderMaterialMap.begin(); it != RenderMaterialMap.end(); it++)
	{
		RenderMaterial* pRenderMaterial = it->second;
		pRenderMaterial->clearRelatedRenderables();
	}
// Add By Lixu End

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

			m_VisibleChangedChunks[chunkIndex] = true;
		}
	}
}

void BlastFamilyModelSimple::onActorUpdate(const ExtPxActor& actor)
{
// Add By Lixu Begin
	uint32_t shapesCount = actor.getPhysXActor().getNbShapes();
	PxTransform lp;
	if (shapesCount > 0)
	{
		std::vector<PxShape*> shapes(shapesCount);
		actor.getPhysXActor().getShapes(&shapes[0], shapesCount);
		PxShape* shape = shapes[0];
		lp = shape->getLocalPose();
	}
// Add By Lixu End

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
// Add By Lixu Begin
			r->setTransform(actor.getPhysXActor().getGlobalPose() * lp * subChunks[chunks[chunkIndex].firstSubchunkIndex].transform);
// Add By Lixu End
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

			m_VisibleChangedChunks[chunkIndex] = false;
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
					float bondHealth = PxClamp(bondHealths[bondIndex] / BOND_HEALTH_MAX, 0.0f, 1.0f);
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

// Add By Lixu Begin
void BlastFamilyModelSimple::setActorSelected(const ExtPxActor& actor, bool selected)
{
	const uint32_t* chunkIndices = actor.getChunkIndices();
	uint32_t chunkCount = actor.getChunkCount();
	for (uint32_t i = 0; i < chunkCount; i++)
	{
		uint32_t chunkIndex = chunkIndices[i];
		std::vector<Renderable*>& renderables = m_chunks[chunkIndex].renderables;
		for (Renderable* r : renderables)
		{
			r->setSelected(selected);
		}
	}
}

void BlastFamilyModelSimple::setChunkSelected(uint32_t chunk, bool selected)
{
	if (chunk > m_chunks.size())
		return;

	std::vector<Renderable*>& renderables = m_chunks[chunk].renderables;
	for (Renderable* r : renderables)
	{
		r->setSelected(select);
	}
}

void BlastFamilyModelSimple::setChunkSelected(std::vector<uint32_t> depths, bool selected)
{
	for (size_t i = 0; i < depths.size(); ++i)
	{
		const std::vector<uint32_t> indexes = m_blastAsset.getChunkIndexesByDepth(depths[i]);
		for (size_t j = 0; j < indexes.size(); ++j)
		{
			setChunkSelected(indexes[j], selected);
		}
	}
}

void BlastFamilyModelSimple::clearChunksSelected()
{
	size_t count = m_chunks.size();
	for (size_t chunkIndex = 0; chunkIndex < count; ++chunkIndex)
	{
		std::vector<Renderable*>& renderables = m_chunks[chunkIndex].renderables;
		for (Renderable* r : renderables)
		{
			r->setSelected(false);
		}
	}
}

bool BlastFamilyModelSimple::getChunkSelected(uint32_t chunk)
{
	if (chunk > m_chunks.size())
		return false;

	std::vector<Renderable*>& renderables = m_chunks[chunk].renderables;
	for (Renderable* r : renderables)
	{
		for (Renderable* r : renderables)
		{
			if (r->isSelected())
			{
				return true;
			}
		}
	}

	return false;
}

std::vector<uint32_t> BlastFamilyModelSimple::getSelectedChunks()
{
	std::vector<uint32_t> selectedChunks;
	size_t count = m_chunks.size();
	for (size_t chunkIndex = 0; chunkIndex < count; ++chunkIndex)
	{
		std::vector<Renderable*>& renderables = m_chunks[chunkIndex].renderables;
		for (Renderable* r : renderables)
		{
			if (r->isSelected())
			{
				selectedChunks.push_back(chunkIndex);
				break;
			}
		}
	}
	return selectedChunks;
}

void BlastFamilyModelSimple::setActorScale(const ExtPxActor& actor, PxMat44& scale, bool replace)
{
	const uint32_t* chunkIndices = actor.getChunkIndices();
	uint32_t chunkCount = actor.getChunkCount();
	for (uint32_t i = 0; i < chunkCount; i++)
	{
		uint32_t chunkIndex = chunkIndices[i];
		std::vector<Renderable*>& renderables = m_chunks[chunkIndex].renderables;
		for (Renderable* r : renderables)
		{
			r->setMeshScale(scale, replace);
		}
	}
}

bool BlastFamilyModelSimple::isChunkVisible(uint32_t chunkIndex)
{
	if (chunkIndex < 0 || chunkIndex >= m_chunks.size())
	{
		return false;
	}

	bool bVisible = false;
	std::vector<Renderable*>& renderables = m_chunks[chunkIndex].renderables;
	if (renderables.size() > 0)
	{
		bVisible = !renderables[0]->isHidden();
	}
	return bVisible;
}

void BlastFamilyModelSimple::setChunkVisible(uint32_t chunkIndex, bool bVisible)
{
	if (chunkIndex < 0 || chunkIndex >= m_chunks.size())
	{
		return;
	}

	std::vector<Renderable*>& renderables = m_chunks[chunkIndex].renderables;
	for (Renderable* r : renderables)
	{
		r->setHidden(!bVisible);
	}
}

void BlastFamilyModelSimple::setChunkVisible(std::vector<uint32_t> depths, bool bVisible)
{
	for (size_t i = 0; i < depths.size(); ++i)
	{
		const std::vector<uint32_t> indexes = m_blastAsset.getChunkIndexesByDepth(depths[i]);
		for (size_t j = 0; j < indexes.size(); ++j)
		{
			setChunkVisible(indexes[j], bVisible);
		}
	}
}

void BlastFamilyModelSimple::initTransform(physx::PxTransform t)
{
	int chunkSize = m_chunks.size();
	for (int i = 0; i < chunkSize; i++)
	{
		std::vector<Renderable*>& renderables = m_chunks[i].renderables;
		for (Renderable* r : renderables)
		{
			r->setTransform(t);
		}
	}
}

void BlastFamilyModelSimple::getMaterial(RenderMaterial** ppRenderMaterial, bool externalSurface)
{
	*ppRenderMaterial = nullptr;

	int chunkSize = m_chunks.size();
	if (chunkSize == 0)
	{
		return;
	}

	if (externalSurface)
	{
		for (int i = 0; i < chunkSize; i++)
		{
			std::vector<Renderable*>& renderables = m_chunks[i].ex_Renderables;
			if (renderables.size() > 0)
			{
				RenderMaterial& m = renderables[0]->getMaterial();
				*ppRenderMaterial = &m;
				return;
			}
		}
	}
	else
	{
		for (int i = 0; i < chunkSize; i++)
		{
			std::vector<Renderable*>& renderables = m_chunks[i].in_Renderables;
			if (renderables.size() > 0)
			{
				RenderMaterial& m = renderables[0]->getMaterial();
				*ppRenderMaterial = &m;
				return;
			}
		}
	}
}

void BlastFamilyModelSimple::setMaterial(RenderMaterial* pRenderMaterial, bool externalSurface)
{
	RenderMaterial* p = pRenderMaterial;
	if (p == nullptr)
	{
		p = RenderMaterial::getDefaultRenderMaterial();
	}

	int chunkSize = m_chunks.size();
	if (externalSurface)
	{
		for (int i = 0; i < chunkSize; i++)
		{
			std::vector<Renderable*>& renderables = m_chunks[i].ex_Renderables;
			for (Renderable* r : renderables)
			{
				r->setMaterial(*p);
			}
		}
	}
	else
	{
		for (int i = 0; i < chunkSize; i++)
		{
			std::vector<Renderable*>& renderables = m_chunks[i].in_Renderables;
			for (Renderable* r : renderables)
			{
				r->setMaterial(*p);
			}
		}
	}
}
// Add By Lixu End