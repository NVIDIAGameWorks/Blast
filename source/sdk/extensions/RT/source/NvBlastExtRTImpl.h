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
// Copyright (c) 2016-2020 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTAUTHORINGRTIMPL_H
#define NVBLASTAUTHORINGRTIMPL_H

#include <NvBlastExtRT.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace physx
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
