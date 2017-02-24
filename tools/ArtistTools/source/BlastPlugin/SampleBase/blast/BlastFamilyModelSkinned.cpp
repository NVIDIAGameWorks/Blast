/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "BlastFamilyModelSkinned.h"
#include "RenderUtils.h"
#include "Renderer.h"
#include "SkinnedRenderMesh.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlastExtPxActor.h"
#include "NvBlastTkActor.h"
#include "PxRigidDynamic.h"

using namespace physx;

BlastFamilyModelSkinned::BlastFamilyModelSkinned(PhysXController& physXController, ExtPxManager& pxManager, Renderer& renderer, const BlastAssetModelSkinned& blastAsset, const BlastAsset::ActorDesc& desc)
	: BlastFamily(physXController, pxManager, blastAsset), m_renderer(renderer), m_visibleActorsDirty(true)
{
	// materials
	auto materials = blastAsset.getRenderMaterials();

	const BlastModel& model = blastAsset.getModel();

	// finalize (create) sub model
	auto finalizeSubModelFunction = [&](SubModel* subModel, std::vector<const SimpleMesh*>& subModelMeshes, RenderMaterial& renderMaterial)
	{
		subModel->skinnedRenderMesh = new SkinnedRenderMesh(subModelMeshes);
		subModel->renderable = renderer.createRenderable(*subModel->skinnedRenderMesh, renderMaterial);
		subModel->renderable->setColor(getRandomPastelColor());
	};

	// create at least one submodel per every material (if mesh count is too high, more then one sub model per material to be created)
	SubModel* subModel = nullptr;
	std::vector<const SimpleMesh*> subModelMeshes;
	subModelMeshes.reserve(model.chunks.size());
	for (uint32_t materialIndex = 0; materialIndex < model.materials.size(); materialIndex++)
	{
		for (uint32_t chunkIndex = 0; chunkIndex < model.chunks.size(); chunkIndex++)
		{
			const BlastModel::Chunk& chunk = model.chunks[chunkIndex];
			for (const BlastModel::Chunk::Mesh& mesh : chunk.meshes)
			{
				if (mesh.materialIndex == materialIndex)
				{
					// init new submodel?
					if (subModel == nullptr)
					{
						m_subModels.push_back(SubModel());
						subModel = &m_subModels.back();
						subModel->chunkIdToBoneMap.resize(model.chunks.size());
						std::fill(subModel->chunkIdToBoneMap.begin(), subModel->chunkIdToBoneMap.end(), SubModel::INVALID_BONE_ID);
						subModelMeshes.clear();
					}

					// add mesh to map and list
					subModel->chunkIdToBoneMap[chunkIndex] = (uint32_t)subModelMeshes.size();
					subModelMeshes.push_back(&(mesh.mesh));

					// mesh reached limit?
					if (subModelMeshes.size() == SkinnedRenderMesh::MeshesCountMax)
					{
						finalizeSubModelFunction(subModel, subModelMeshes, *materials[materialIndex]);
						subModel = nullptr;
					}
				}
			}
		}

		// finalize subModel for this material
		if (subModel && subModelMeshes.size() > 0)
		{
			finalizeSubModelFunction(subModel, subModelMeshes, *materials[materialIndex]);
			subModel = nullptr;
		}
	}

	// reserve for scratch
	m_visibleBones.reserve(model.chunks.size());
	m_visibleBoneTransforms.reserve(model.chunks.size());

	// initialize in position
	initialize(desc);
}

BlastFamilyModelSkinned::~BlastFamilyModelSkinned()
{
	for (uint32_t subModelIndex = 0; subModelIndex < m_subModels.size(); subModelIndex++)
	{
		m_renderer.removeRenderable(m_subModels[subModelIndex].renderable);
		SAFE_DELETE(m_subModels[subModelIndex].skinnedRenderMesh);
	}
}

void BlastFamilyModelSkinned::onActorCreated(const ExtPxActor& actor)
{
	m_visibleActors.insert(&actor);
	m_visibleActorsDirty = true;
}

void BlastFamilyModelSkinned::onActorUpdate(const ExtPxActor& actor)
{
}

void BlastFamilyModelSkinned::onActorDestroyed(const ExtPxActor& actor)
{
	m_visibleActors.erase(&actor);
	m_visibleActorsDirty = true;
}

void BlastFamilyModelSkinned::onUpdate()
{
	// visible actors changed this frame?
	if (m_visibleActorsDirty)
	{
		for (const SubModel& model : m_subModels)
		{
			// pass visible chunks list to render mesh
			m_visibleBones.clear();
			for (const ExtPxActor* actor : m_visibleActors)
			{
				const uint32_t* chunkIndices = actor->getChunkIndices();
				uint32_t chunkCount = actor->getChunkCount();
				for (uint32_t i = 0; i < chunkCount; ++i)
				{
					uint32_t chunkIndex = chunkIndices[i];
					uint32_t boneIndex = model.chunkIdToBoneMap[chunkIndex];
					if (boneIndex != SubModel::INVALID_BONE_ID)
					{
						m_visibleBones.push_back(boneIndex);
					}
				}
			}
			model.skinnedRenderMesh->updateVisibleMeshes(m_visibleBones);
		}

		m_visibleActorsDirty = false;
	}

	// update and pass chunk transforms
	const ExtPxChunk* chunks = m_blastAsset.getPxAsset()->getChunks();
	const ExtPxSubchunk* subChunks = m_blastAsset.getPxAsset()->getSubchunks();
	for (const SubModel& model : m_subModels)
	{
		m_visibleBoneTransforms.clear();
		for (const ExtPxActor* actor : m_visibleActors)
		{
			const uint32_t* chunkIndices = actor->getChunkIndices();
			uint32_t chunkCount = actor->getChunkCount();
			for (uint32_t i = 0; i < chunkCount; ++i)
			{
				uint32_t chunkIndex = chunkIndices[i];
				uint32_t boneIndex = model.chunkIdToBoneMap[chunkIndex];
				if (boneIndex != SubModel::INVALID_BONE_ID)
				{
					m_visibleBoneTransforms.push_back(PxMat44(actor->getPhysXActor().getGlobalPose() * subChunks[chunks[chunkIndex].firstSubchunkIndex].transform));
				}
			}
		}
		model.skinnedRenderMesh->updateVisibleMeshTransforms(m_visibleBoneTransforms);
	}
}
