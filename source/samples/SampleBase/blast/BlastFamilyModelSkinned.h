// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2008-2023 NVIDIA Corporation. All rights reserved.


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

        Renderable* renderable = nullptr;
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