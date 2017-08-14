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
#include "MaterialLibraryPanel.h"

// Add By Lixu Begin
#include "NvBlastExtPxManager.h"
#include "SceneController.h"
#include "PhysXController.h"
#include "SampleManager.h"
#include "BlastModel.h"
#include "PxPhysics.h"
#include "PxScene.h"
#include "GizmoToolController.h"
// Add By Lixu End

using namespace physx;

#include "ViewerOutput.h"
// print out all shapes for debug purpose. check actor/shape releasing.
void PrintActors(PhysXController& physXController)
{
	PxPhysics& physx = physXController.getPhysics();
	static int convexNum = 0, shapeNum = 0;
	int convexNumNew = physx.getNbConvexMeshes();
	int shapeNumNew = physx.getNbShapes();
	if (shapeNum != shapeNumNew && shapeNumNew > 0)
	{
		// print shapes
		std::vector<PxShape*> shapes(shapeNumNew);
		physx.getShapes(shapes.data(), shapeNumNew);
		shapeNum = shapeNumNew;
		for (PxU32 u = 0; u < shapeNumNew; ++u)
		{
			PxShape& shape = *shapes[u];
			PxRigidActor* pActor = shape.getActor();
			const char* pName = pActor ? pActor->getName() : nullptr;
			char buf[256];
			if (pName)
			{
				sprintf(buf, "Actor %x shape %x %s", pActor, &shape, pName);
			}
			else
			{
				sprintf(buf, "Actor %x shape %x", pActor, &shape);
			}
			viewer_msg(buf);
		}
	}
}

// only modify unfractured mode mesh
void modifyModelByLocalWay(BlastModel& model, PxTransform& gp_old, PxTransform& gp_new)
{
	BlastModel::Chunk& chunk = model.chunks[0];//unfracture model only has one chunk

	std::vector<BlastModel::Chunk::Mesh>& meshes = chunk.meshes;
	int meshSize = meshes.size();

	if (meshSize == 0)
	{
		return;
	}

	PxTransform gp_newInv = gp_new.getInverse();
	for (int ms = 0; ms < meshSize; ms++)
	{
		BlastModel::Chunk::Mesh& mesh = meshes[ms];
		SimpleMesh& simpleMesh = mesh.mesh;
		std::vector<SimpleMesh::Vertex>& vertices = simpleMesh.vertices;

		int NumVertices = vertices.size();
		for (uint32_t i = 0; i < NumVertices; ++i)
		{
			PxTransform v_old(vertices[i].position);
			PxTransform v_new = gp_newInv * gp_old * v_old;
			physx::PxVec3 pos = v_new.p;

			SimpleMesh::Vertex& vertex = vertices[i];
			vertex.position.x = pos.x;
			vertex.position.y = pos.y;
			vertex.position.z = pos.z;
		}
	}
}


void scaleModel(BlastModel& model, PxMat44& scale)
{
	BlastModel::Chunk& chunk = model.chunks[0];//unfracture model only has one chunk

	std::vector<BlastModel::Chunk::Mesh>& meshes = chunk.meshes;
	int meshSize = meshes.size();

	if (meshSize == 0)
	{
		return;
	}

	for (int ms = 0; ms < meshSize; ms++)
	{
		BlastModel::Chunk::Mesh& mesh = meshes[ms];
		SimpleMesh& simpleMesh = mesh.mesh;
		std::vector<SimpleMesh::Vertex>& vertices = simpleMesh.vertices;

		int NumVertices = vertices.size();
		for (uint32_t i = 0; i < NumVertices; ++i)
		{
			SimpleMesh::Vertex& vertex = vertices[i];
			physx::PxVec3 pos = scale.transform(vertices[i].position);

			vertex.position.x = pos.x;
			vertex.position.y = pos.y;
			vertex.position.z = pos.z;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												SimpleRenderMesh
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class SimpleRenderMesh : public IRenderMesh
{
public:
	SimpleRenderMesh(const SimpleMesh* mesh, int renderableId) : m_mesh(mesh)
	{
		mUniqueId = renderableId;

		m_device = GetDeviceManager()->GetDevice();

		m_inputDesc.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 });
		m_inputDesc.push_back({ "VERTEX_NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 });
		m_inputDesc.push_back({ "FACE_NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 });
		m_inputDesc.push_back({ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 });
		m_inputDesc.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 });
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

		UINT strides[2] = { sizeof(SimpleMesh::Vertex), sizeof(float) };
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
	// Add By Lixu Begin
	SampleManager* pSampleManager = SampleManager::ins();
	std::map<BlastAsset*, std::vector<BlastFamily*>>& AssetFamiliesMap = pSampleManager->getAssetFamiliesMap();
	std::map<BlastAsset*, AssetList::ModelAsset>& AssetDescMap = pSampleManager->getAssetDescMap();

	BlastAsset* pBlastAsset = (BlastAsset*)&getBlastAsset();
	AssetFamiliesMap[pBlastAsset].push_back(this);

	AssetList::ModelAsset& m = AssetDescMap[pBlastAsset];
	// Add By Lixu End

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

		PxActor* actor = pSampleManager->getPhysXController().createEditPhysXActor(meshes, desc.transform);
		m_editActorChunkMap.insert(std::make_pair(actor, chunkIndex));
		m_chunkEditActorMap.insert(std::make_pair(chunkIndex, actor));
// Add By Lixu End
		renderMeshes.resize(meshes.size());
		renderables.resize(meshes.size());

		int renderableId = Renderable::getRenderableId(mUniqueId, chunkIndex);

		for (uint32_t i = 0; i < meshes.size(); i++)
		{
			renderMeshes[i] = new SimpleRenderMesh(&meshes[i].mesh, renderableId);

			uint32_t materialIndex = model.chunks[chunkIndex].meshes[i].materialIndex;

			RenderMaterial* pRenderMaterial = pSampleManager->getRenderMaterial(materials[materialIndex]);

			Renderable* renderable = renderer.createRenderable(*renderMeshes[i], *pRenderMaterial);
			renderable->setHidden(!_getBPPChunkVisible(chunkIndex));
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

// Add By Lixu Begin
	initTransform(desc.transform);
// Add By Lixu End

	// initialize in position
	initialize(desc);
}

BlastFamilyModelSimple::~BlastFamilyModelSimple()
{
// Add By Lixu Begin
	// remove from AssetFamiliesMap
	SampleManager* pSampleManager = SampleManager::ins();
	std::map<BlastAsset*, std::vector<BlastFamily*>>& AssetFamiliesMap = pSampleManager->getAssetFamiliesMap();
	BlastAsset* pBlastAsset = (BlastAsset*)&getBlastAsset();
	std::vector<BlastFamily*>& families = AssetFamiliesMap[pBlastAsset];
	std::vector<BlastFamily*>::iterator itBF;
	for (itBF = families.begin(); itBF != families.end(); itBF++)
	{
		if ((*itBF) == this)
		{
			families.erase(itBF);
			break;
		}
	}
	if (families.size() == 0)
	{
		AssetFamiliesMap.erase(AssetFamiliesMap.find(pBlastAsset));
	}

	// disconnect the material and relative renderable
	const BlastAssetModelSimple& blastAsset = *(BlastAssetModelSimple*)&m_blastAsset;
	const std::vector<std::string>& materials = blastAsset.getRenderMaterials();
	int materialSize = materials.size();
	for (int ms = 0; ms < materialSize; ms++)
	{
		RenderMaterial* pRenderMaterial = pSampleManager->getRenderMaterial(materials[ms]);
		pRenderMaterial->clearRelatedRenderables();
	}

	// remove physx actor for edit mode
	PxScene& editScene = pSampleManager->getPhysXController().getEditPhysXScene();

	for (std::map<PxActor*, uint32_t>::iterator itr = m_editActorChunkMap.begin(); itr != m_editActorChunkMap.end(); ++itr)
	{
		editScene.removeActor(*(itr->first));
		PxRigidDynamic* rigidDynamic = (itr->first)->is<PxRigidDynamic>();
		if (rigidDynamic == nullptr)
		{
			itr->first->release();
			continue;
		}
		
		PxU32 shapeCount = rigidDynamic->getNbShapes();
		std::vector<PxShape*> shapes;
		shapes.resize(shapeCount);
		rigidDynamic->getShapes(shapes.data(), shapeCount);
		for (PxU32 u = 0; u < shapeCount; ++u)
		{
			PxShape& shape = *shapes[u];
			rigidDynamic->detachShape(shape);
			PxConvexMeshGeometry geometry;
			shape.getConvexMeshGeometry(geometry);
			geometry.convexMesh->release();
			shape.release();
		}

		rigidDynamic->release();
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
	// Add By Lixu Begin
	//PrintActors(SampleManager::ins()->getPhysXController());
	// Add By Lixu End
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
			if (SampleManager::ins()->IsSimulating())
			{
				renderables[r]->setHidden(false);
			}
			else
			{
				renderables[r]->setHidden(!_getBPPChunkVisible(chunkIndex));
			}
//			renderables[r]->setColor(colors[r]);

			m_VisibleChangedChunks[chunkIndex] = true;
		}
	}
}

void BlastFamilyModelSimple::onActorUpdate(const ExtPxActor& actor)
{
// Add By Lixu Begin
	if (!SampleManager::ins()->IsSimulating())
		return;

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

		// Add By Lixu Begin
		PxActor* editActor = m_chunkEditActorMap[chunkIndex];
		PxRigidDynamic* rigidDynamic = editActor->is<PxRigidDynamic>();
		rigidDynamic->setGlobalPose(actor.getPhysXActor().getGlobalPose() * lp * subChunks[chunks[chunkIndex].firstSubchunkIndex].transform);
		// Add By Lixu End
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

// Add By Lixu Begin
bool BlastFamilyModelSimple::find(const PxActor& actor)
{
	return -1 != getChunkIndexByPxActor(actor);
}

void BlastFamilyModelSimple::updateActorRenderableTransform(const PxActor& actor, PxTransform& pos, bool local)
{
	uint32_t chunkIndex = getChunkIndexByPxActor(actor);
	if (-1 == chunkIndex)
		return;

	std::vector<Renderable*>& renderables = m_chunks[chunkIndex].renderables;
	for (Renderable* r : renderables)
	{
		if (!local)
		{
			r->setTransform(pos);

			// in edit mode, if change root chunk's orientation in edit physx scene, also change root physx actor in blast physx scene
			if (0 == chunkIndex && !SampleManager::ins()->IsSimulating())
			{
				(*m_actors.begin())->getPhysXActor().setGlobalPose(pos);
			}
		}
		else
		{
			if (0 == chunkIndex)
			{
				PxActor& blastActor = (*m_actors.begin())->getPhysXActor();
				PxRigidDynamic* rigidDynamic = blastActor.is<PxRigidDynamic>();
				if (NULL != rigidDynamic)
				{
					PxTransform gp_new = pos;
					PxTransform gp_old = rigidDynamic->getGlobalPose();
					rigidDynamic->setGlobalPose(pos);
					PxScene& pxScene = SampleManager::ins()->getPhysXController().getPhysXScene();
					modifyPxActorByLocalWay(pxScene, *rigidDynamic, gp_old, gp_new);
					const BlastModel& model = dynamic_cast<BlastAssetModelSimple*>(const_cast<BlastAsset*>(&m_blastAsset))->getModel();
					// update model mesh
					modifyModelByLocalWay(*(const_cast<BlastModel*>(&model)), gp_old, gp_new);
					// update blast asset instance's transform
					BPPAssetInstance* assetInstance = SampleManager::ins()->getInstanceByFamily(this);
					assetInstance->transform.position = nvidia::NvVec3(gp_new.p.x, gp_new.p.y, gp_new.p.z);
					assetInstance->transform.rotation = nvidia::NvVec4(gp_new.q.x, gp_new.q.y, gp_new.q.z, gp_new.q.w);
				}
			}
		}
	}
}

uint32_t BlastFamilyModelSimple::getChunkIndexByPxActor(const PxActor& actor)
{
	std::map<PxActor*, uint32_t>::iterator itr = m_editActorChunkMap.find(const_cast<PxActor*>(&actor));

	if (itr != m_editActorChunkMap.end())
	{
		return itr->second;
	}
	else
	{
		for (ExtPxActor* extPxActor : m_actors)
		{
			if (&(extPxActor->getPhysXActor()) == (&actor)->is<physx::PxRigidDynamic>())
			{
				return extPxActor->getChunkIndices()[0];
			}
		}
	}
	return -1;
}

bool BlastFamilyModelSimple::getPxActorByChunkIndex(uint32_t chunkIndex, PxActor** ppActor)
{
	*ppActor = nullptr;
	std::map<uint32_t, PxActor*>::iterator it =	m_chunkEditActorMap.find(chunkIndex);
	if (it == m_chunkEditActorMap.end())
	{
		return false;
	}

	*ppActor = it->second;
	return true;
}

void BlastFamilyModelSimple::setActorSelected(const PxActor& actor, bool selected)
{
	uint32_t chunkIndex = getChunkIndexByPxActor(actor);
	if (-1 == chunkIndex)
		return;

	bool selectionDepthTest = BlastProject::ins().getParams().fracture.general.selectionDepthTest;

	std::vector<Renderable*>& renderables = m_chunks[chunkIndex].renderables;
	for (Renderable* r : renderables)
	{
		r->setSelected(selected);

		if (!selectionDepthTest && selected)
		{
			r->setDepthTest(false);
		}
		else
		{
			r->setDepthTest(true);
		}
	}
}

bool BlastFamilyModelSimple::isActorSelected(const PxActor& actor)
{
	uint32_t chunkIndex = getChunkIndexByPxActor(actor);
	if (-1 == chunkIndex)
		return false;

	std::vector<Renderable*>& renderables = m_chunks[chunkIndex].renderables;
	return renderables[0]->isSelected();
}

void BlastFamilyModelSimple::setActorVisible(const PxActor& actor, bool visible)
{
	uint32_t chunkIndex = getChunkIndexByPxActor(actor);
	if (-1 == chunkIndex)
		return;

	std::vector<Renderable*>& renderables = m_chunks[chunkIndex].renderables;
	for (Renderable* r : renderables)
	{
		r->setHidden(!visible);
	}
}

bool BlastFamilyModelSimple::isActorVisible(const PxActor& actor)
{
	uint32_t chunkIndex = getChunkIndexByPxActor(actor);
	if (-1 == chunkIndex)
		return false;

	std::vector<Renderable*>& renderables = m_chunks[chunkIndex].renderables;
	return !renderables[0]->isHidden();
}

void BlastFamilyModelSimple::setChunkSelected(uint32_t chunk, bool selected)
{
	if (chunk > m_chunks.size())
		return;

	bool selectionDepthTest = BlastProject::ins().getParams().fracture.general.selectionDepthTest;

	std::vector<Renderable*>& renderables = m_chunks[chunk].renderables;
	for (Renderable* r : renderables)
	{
		r->setSelected(selected);

		if (!selectionDepthTest && selected)
		{
			r->setDepthTest(false);
		}
		else
		{
			r->setDepthTest(true);
		}
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
			r->setDepthTest(true);
			r->setHighlight(false);
		}
	}
}

bool BlastFamilyModelSimple::isChunkSelected(uint32_t chunk)
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

void BlastFamilyModelSimple::setActorScale(const PxActor& actor, PxMat44& scale, bool replace)
{
	uint32_t chunkIndex = getChunkIndexByPxActor(actor);
	if (-1 == chunkIndex)
		return;
	std::vector<Renderable*>& renderables = m_chunks[chunkIndex].renderables;
	for (Renderable* r : renderables)
	{
		r->setMeshScale(scale, replace);
	}

	if (0 == chunkIndex)
	{
		PxActor& blastActor = (*m_actors.begin())->getPhysXActor();
		PxRigidDynamic* rigidDynamic = blastActor.is<PxRigidDynamic>();
		if (NULL != rigidDynamic)
		{
			PxScene& pxScene = SampleManager::ins()->getPhysXController().getPhysXScene();
			scalePxActor(pxScene, *rigidDynamic, scale);

			if (replace)
			{
				const BlastModel& model = dynamic_cast<BlastAssetModelSimple*>(const_cast<BlastAsset*>(&m_blastAsset))->getModel();
				scaleModel(*(const_cast<BlastModel*>(&model)), scale);
			}
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
	BlastFamily::initTransform(t);
	int chunkSize = m_chunks.size();
	for (int i = 0; i < chunkSize; i++)
	{
		std::vector<Renderable*>& renderables = m_chunks[i].renderables;
		for (Renderable* r : renderables)
		{
			r->setTransform(t);
		}
	}

	for (std::map<PxActor*, uint32_t>::iterator itr = m_editActorChunkMap.begin(); itr != m_editActorChunkMap.end(); ++itr)
	{
		PxRigidDynamic* rigidDynamic = itr->first->is<PxRigidDynamic>();
		rigidDynamic->setGlobalPose(t);
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

void BlastFamilyModelSimple::highlightChunks()
{
	bool selectionDepthTest = BlastProject::ins().getParams().fracture.general.selectionDepthTest;

	for (Chunk chunk : m_chunks)
	{
		std::vector<Renderable*>& renderables = chunk.renderables;
		for (Renderable* r : renderables)
		{
			r->setHighlight(true);
			r->setDepthTest(selectionDepthTest);
		}
	}
}

bool BlastFamilyModelSimple::_getBPPChunkVisible(uint32_t chunkIndex)
{
	std::map<BlastAsset*, AssetList::ModelAsset>& assetDescMap = SampleManager::ins()->getAssetDescMap();
	BlastAsset* blastAsset = (BlastAsset*)&getBlastAsset();
	AssetList::ModelAsset& m = assetDescMap[blastAsset];

	BlastProject& project = BlastProject::ins();
	BPPAsset& bppAsset = *(project.getAsset(m.name.c_str()));

	BPPChunk* bppChunk = project.getChunk(bppAsset, chunkIndex);
	if (bppChunk)
	{
		return bppChunk->visible;
	}
	else
	{
		return true;
	}
}
// Add By Lixu End