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


#ifndef NVBLASTAUTHORINGRTIMPL_H
#define NVBLASTAUTHORINGRTIMPL_H

#include <NvBlastExtRT.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace nvidia
{
    class PxCooking;
};

namespace Nv
{
    namespace Blast
    {

        class BooleanToolV2;
        class TriangulatorV2;

        struct BooleanToolOutputDataImpl : public BooleanToolOutputData
        {
            BooleanToolOutputDataImpl(bool createOnlyEdgeBuffer = false);

            void release() override;

            void reset() override;
            void resetEdges() override;

            void copyVerticesAndResults(const BooleanToolOutputData* other) override;

            uint32_t edgesCount() const override
            {
                return *mEdgesCount;
            }
            uint32_t verticesCount() const override
            {
                return *mVerticesCount;
            }
            uint32_t ffResultCount() const override
            {
                return *mFfResultCount;
            }

            uint32_t addEdge(const BooleanResultEdge&) override;
            uint32_t addVertex(const Vertex&) override;
            uint32_t addFfResult(const FacetFacetResult&) override;

            BooleanResultEdge& getNewEdge() override;
            Vertex& getNewVertex() override;
            FacetFacetResult& getNewFfResult() override;

            std::atomic<uint32_t>* mEdgesCount;
            std::atomic<uint32_t>* mVerticesCount;
            std::atomic<uint32_t>* mFfResultCount;

            bool isVerticesAndResultsAllocated;
        };

        class FractureRTImpl : public FractureRT
        {
        public:
            
            FractureRTImpl();

            virtual void release() override;
            virtual void processMesh(DamagePattern* pattern, const Mesh* msh) override;

            virtual uint32_t getResultChunkCount() override 
            {
                return resultChunkCount;
            };
            virtual Vertex* getVertexBuffer() override
            {
                return verticesBuffer;
            };
            virtual uint32_t* getVertexOffset() override
            {
                return vertexOffset;
            };
            virtual uint32_t* getIndexBuffer() override
            {
                return indexBuffer;
            };
            virtual uint32_t* getIndexOffset() override 
            {
                return indexOffset;
            };
            PerTriangleAdditionalData* getPerTriangleData() override
            {
                return adata;
            };

            void dumpChunksToObj(const char* path) override;

        private:

            BooleanToolV2* btool;
            TriangulatorV2* triangulator;
            Vertex* verticesBuffer;
            uint32_t* indexBuffer;
            PerTriangleAdditionalData* adata;

            uint32_t triangleCount;
            uint32_t vertexCount;

            uint32_t* indexOffset;
            uint32_t* vertexOffset;
            uint32_t resultChunkCount;

            BooleanToolOutputDataImpl* outputData;
        };
    }
}

Nv::Blast::BooleanToolOutputData* CreateBooleanToolOutputData(bool isMergedMesh = false);

#ifdef USE_MERGED_MESH
void sortResultBuffer(Nv::Blast::FacetFacetResult*& ffResultBuffer, uint32_t ffCount);
void procesOutputEdges(Nv::Blast::BooleanToolOutputData* outputData, Nv::Blast::DamagePattern* pattern, uint32_t facetCount);
#endif

#endif // ifndef NVBLASTAUTHORINGRTIMPL_H
