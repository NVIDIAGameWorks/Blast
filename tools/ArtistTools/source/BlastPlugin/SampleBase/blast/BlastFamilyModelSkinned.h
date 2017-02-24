/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef BLAST_FAMILY_MODEL_SKINNED_H
#define BLAST_FAMILY_MODEL_SKINNED_H

#include "BlastFamily.h"
#include "BlastAssetModelSkinned.h"

class SkinnedRenderMesh;
class Renderable;

class BlastFamilyModelSkinned : public BlastFamily
{
public:
	//////// ctor ////////

	BlastFamilyModelSkinned(PhysXController& physXController, ExtPxManager& pxManager, Renderer& renderer, const BlastAssetModelSkinned& blastAsset, const BlastAsset::ActorDesc& desc);
	virtual ~BlastFamilyModelSkinned();

protected:
	//////// abstract implementation ////////

	virtual void onActorCreated(const ExtPxActor& actor);
	virtual void onActorUpdate(const ExtPxActor& actor);
	virtual void onActorDestroyed(const ExtPxActor& actor);

	virtual void onUpdate();

private:
	//////// internal data ////////

	Renderer& m_renderer;

	struct SubModel
	{
		static const uint32_t INVALID_BONE_ID = ~(uint32_t)0;

		Renderable*	renderable = nullptr;
		SkinnedRenderMesh* skinnedRenderMesh = nullptr;
		std::vector<uint32_t> chunkIdToBoneMap;
	};
	std::vector<SubModel> m_subModels;

	std::set<const ExtPxActor*> m_visibleActors;
	bool m_visibleActorsDirty;

	//////// scratch buffers ////////

	std::vector<uint32_t> m_visibleBones;
	std::vector<PxMat44>  m_visibleBoneTransforms;

};


#endif //BLAST_FAMILY_MODEL_SKINNED_H