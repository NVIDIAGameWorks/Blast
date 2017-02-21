/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef BLAST_FAMILY_BOXES
#define BLAST_FAMILY_BOXES

#include "BlastFamily.h"

class BlastAssetBoxes;
class Renderable;


class BlastFamilyBoxes : public BlastFamily
{
public:
	BlastFamilyBoxes(PhysXController& physXController, ExtPxManager& pxManager, Renderer& renderer, const BlastAssetBoxes& blastAsset, const BlastAsset::ActorDesc& desc);
	virtual ~BlastFamilyBoxes();

protected:
	virtual void onActorCreated(const ExtPxActor& actor);
	virtual void onActorUpdate(const ExtPxActor& actor);
	virtual void onActorDestroyed(const ExtPxActor& actor);

private:
	Renderer&				 m_renderer;
	std::vector<Renderable*> m_chunkRenderables;
};


#endif //BLAST_FAMILY_BOXES