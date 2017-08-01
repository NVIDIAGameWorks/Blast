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


#ifndef BLAST_FAMILY_MODEL_SIMPLE_H
#define BLAST_FAMILY_MODEL_SIMPLE_H

#include "BlastFamily.h"
#include "BlastAssetModelSimple.h"

class SimpleRenderMesh;
class Renderable;
class Renderer;
// Add By Lixu Begin
namespace physx
{
	class PxActor;
}
using namespace physx;
// Add By Lixu End

class BlastFamilyModelSimple : public BlastFamily
{
public:
	//////// ctor ////////

	BlastFamilyModelSimple(PhysXController& physXController, ExtPxManager& pxManager, Renderer& renderer, const BlastAssetModelSimple& blastAsset, const BlastAsset::ActorDesc& desc);
	virtual ~BlastFamilyModelSimple();

// Add By Lixu Begin
	virtual bool find(const PxActor& actor);
	virtual void updateActorRenderableTransform(const PxActor& actor, PxTransform& pos, bool local);
	virtual uint32_t getChunkIndexByPxActor(const PxActor& actor);
	virtual bool getPxActorByChunkIndex(uint32_t chunkIndex, PxActor** ppActor);
	virtual void setActorSelected(const PxActor& actor, bool selected);
	virtual bool isActorSelected(const PxActor& actor);
	virtual void setActorVisible(const PxActor& actor, bool visible);
	virtual bool isActorVisible(const PxActor& actor);
	virtual void clearChunksSelected();
	virtual void setChunkSelected(uint32_t chunk, bool selected);
	virtual void setChunkSelected(std::vector<uint32_t> depths, bool selected);
	virtual bool isChunkSelected(uint32_t chunk);
	virtual std::vector<uint32_t> getSelectedChunks();
	virtual void setActorScale(const PxActor& actor, PxMat44& scale, bool replace);
	virtual bool isChunkVisible(uint32_t chunkIndex);
	virtual void setChunkVisible(uint32_t chunkIndex, bool bVisible);
	virtual void setChunkVisible(std::vector<uint32_t> depths, bool bVisible);
	virtual void initTransform(physx::PxTransform t);
	virtual void getMaterial(RenderMaterial** ppRenderMaterial, bool externalSurface);
	virtual void setMaterial(RenderMaterial* pRenderMaterial, bool externalSurface);
	virtual void highlightChunks();
// Add By Lixu End

protected:
	//////// abstract implementation ////////

	virtual void onActorCreated(const ExtPxActor& actor);
	virtual void onActorUpdate(const ExtPxActor& actor);
	virtual void onActorDestroyed(const ExtPxActor& actor);
	virtual void onActorHealthUpdate(const ExtPxActor& pxActor);

private:
	// Add By Lixu Begin
	bool _getBPPChunkVisible(uint32_t chunkIndex);
	// Add By Lixu End

	//////// internal data ////////

	Renderer& m_renderer;

	struct Chunk
	{
		std::vector<SimpleRenderMesh*> renderMeshes;
		std::vector<Renderable*> renderables;
// Add By Lixu Begin
		std::vector<Renderable*> ex_Renderables;
		std::vector<Renderable*> in_Renderables;
// Add By Lixu End
	};

	std::vector<Chunk> m_chunks;
	std::map<PxActor*, uint32_t> m_editActorChunkMap;// only for edit mode
	std::map<uint32_t, PxActor*> m_chunkEditActorMap;// only for edit mode
};


#endif //BLAST_FAMILY_MODEL_SIMPLE_H