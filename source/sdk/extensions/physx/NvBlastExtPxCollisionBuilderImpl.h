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
// Copyright (c) 2016-2024 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTEXTPXCOLLISIONBUILDERIMPL_H
#define NVBLASTEXTPXCOLLISIONBUILDERIMPL_H

#include "NvBlastExtPxCollisionBuilder.h"
#include "NvBlastExtAuthoringTypes.h"

namespace physx
{
    class PxCooking;
    class PxInsertionCallback;
}
namespace Nv
{
    namespace Blast
    {

        struct CollisionHullImpl : public CollisionHull
        {
            CollisionHullImpl() {};
            CollisionHullImpl(const CollisionHull& hullToCopy);
            ~CollisionHullImpl();
        };

        class ExtPxCollisionBuilderImpl : public ExtPxCollisionBuilder
        {
        public:
            ExtPxCollisionBuilderImpl(physx::PxCooking* cooking,
                physx::PxInsertionCallback* insertionCallback) : mCooking(cooking), mInsertionCallback(insertionCallback) {}
            virtual ~ExtPxCollisionBuilderImpl() {};
            void release() override;
            CollisionHull* buildCollisionGeometry(uint32_t verticesCount, const NvcVec3* vertexData) override;
            void releaseCollisionHull(CollisionHull* hull) const override;

            physx::PxConvexMesh* buildConvexMesh(const CollisionHull& hull) override;
            void buildPhysicsChunks(uint32_t chunkCount, uint32_t* hullOffsets, CollisionHull** hulls,
                ExtPxChunk* physicsChunks, ExtPxSubchunk* physicsSubchunks) override;
        private:
            physx::PxCooking* mCooking;
            physx::PxInsertionCallback* mInsertionCallback;
        };

    }  // namespace Blast
}  // namespace Nv


#endif  // ifndef NVBLASTEXTPXCOLLISIONBUILDERIMPL_H
