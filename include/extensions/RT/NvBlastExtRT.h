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


#ifndef NVBLASTEXTRT_H
#define NVBLASTEXTRT_H

#include <cinttypes>
#include <NvBlastTypes.h>
#include <NvCTypes.h>


//#define USE_MERGED_MESH


#define BLASTRT_MAX_VERTICES 262144
#define BLASTRT_MAX_EDGES_PER_CHUNK 16384
#define BLASTRT_MAX_CHUNKS 256

namespace Nv
{
    namespace Blast
    {
        struct Vertex;
        class  Mesh;
        class  SpatialAccelerator;
        class  VertexWelding;
        struct DamagePattern;

        struct FacetFacetResult
        {
            uint32_t parentFacet;
            uint32_t adjacentFacet;
            uint32_t parentEdge;
            int32_t status;
            uint32_t pIdx;

            bool operator<(const FacetFacetResult& in) const
            {
                return parentFacet < in.parentFacet;
            }
        };

        struct BooleanResultEdge
        {
            uint32_t start;
            uint32_t end;
            int32_t parentFacet;
            int32_t adjacentFacet = -1;

            BooleanResultEdge getInversed()
            {
                BooleanResultEdge ret;
                ret.start = end;
                ret.end = start;
                ret.parentFacet = parentFacet;
                ret.adjacentFacet = adjacentFacet;
                return ret;
            }
        };

        struct BooleanToolOutputData
        {
            virtual ~BooleanToolOutputData() {}

            virtual void release() = 0;

            //set edges, vertices and ffResult counters to 0
            virtual void reset() = 0;
            virtual void resetEdges() = 0;

            virtual void copyVerticesAndResults(const BooleanToolOutputData* other) = 0;

            virtual uint32_t edgesCount() const = 0;
            virtual uint32_t verticesCount() const = 0;
            virtual uint32_t ffResultCount() const = 0;

            //Thread safe add, return index in buffer
            virtual uint32_t addEdge(const BooleanResultEdge&) = 0;
            virtual uint32_t addVertex(const Vertex&) = 0;
            virtual uint32_t addFfResult(const FacetFacetResult&) = 0;

            //Thread safe, increment counter and return reference to last element
            virtual BooleanResultEdge& getNewEdge() = 0;
            virtual Vertex& getNewVertex() = 0;
            virtual FacetFacetResult& getNewFfResult() = 0;

            //User allocated buffers should have size more than return values of above function 
            BooleanResultEdge* edges = nullptr;
            Vertex* vertices = nullptr;
            FacetFacetResult* ffResult = nullptr;
        };
        
        /**
        RT fracture LL API. Use it to implement own
        */

        class Fracturer
        {
        public:
            virtual void release() = 0;
        };

        class MeshGenerator
        {
        public:
            virtual void release() = 0;
        };

        struct FractureDesc
        {
            Fracturer* fr = nullptr;
            const Mesh* model = nullptr;
            const Mesh* cell = nullptr;
            SpatialAccelerator* modelAccel = nullptr;
            SpatialAccelerator* cellAccel = nullptr;
            DamagePattern* pattern = nullptr;
            BooleanToolOutputData* outputData = nullptr;
            uint32_t chunkId;
        };

        struct PerTriangleAdditionalData
        {
            int32_t materialIndex;
            int32_t smoothingGroup;
        };

        struct MeshDesc
        {
            MeshGenerator* tr = nullptr;
            const BooleanResultEdge* bEdges = nullptr;
            uint32_t edesCount = 0;
            const Vertex* inVertices = nullptr;
            const Mesh* meshA = nullptr; // used to gather additional data from source mesh, for example material ID 
            const Mesh* meshB = nullptr;
        };

        class FractureRT
        {
        public:

            struct Stage
            {
                enum
                {
                    FACET_FACET_TEST = 1,
                    RETAIN_FROM_FRACTURED_MESH = 2,
                    RETAIN_FROM_PATTERN = 4,

                    ALL = 0xFFFFFFFF
                };
            };

            virtual void release() = 0;
            virtual void processMesh(DamagePattern* pattern, const Mesh* msh) = 0;
            virtual uint32_t getResultChunkCount() = 0;
            virtual Vertex* getVertexBuffer() = 0;
            virtual uint32_t* getIndexBuffer() = 0;
            virtual uint32_t* getVertexOffset() = 0;
            virtual uint32_t* getIndexOffset() = 0;
            virtual PerTriangleAdditionalData* getPerTriangleData() = 0;
            virtual void dumpChunksToObj(const char* path) = 0;
        };

        enum PatternFacetType { GOOD_FACET = 0, INFINITE_FACET = 0xffffff };


        /**
        Graph used to detect islands
        */
        struct ChunkGraphLink
        {
            ChunkGraphLink() = default;
            ChunkGraphLink(uint32_t i, uint32_t j) : l1(i), l2(j) {};
            uint32_t l1;
            uint32_t l2;

            bool operator<(const ChunkGraphLink& lk) const;
            bool operator<(const uint32_t in) const;
        };

        struct ChunkGraph
        {
            virtual void eraseNode(uint32_t index) = 0;
            virtual void release() = 0;

            ChunkGraphLink* links = nullptr;
            uint32_t* dirtyChunks = nullptr;
            uint32_t linksCount = 0;
            uint32_t dirtyChunksCount = 0;
            uint32_t newlyAddedCount = 0;
            uint32_t maxLinksCount = 0;
        };

    }
}


/**
Create real time (RT) fracture. By default creates single thread. For threads > 1 multithreaded implementation based on std lib is used.
*/
NVBLAST_API Nv::Blast::FractureRT* NvBlastExtRTCreateFractureRT(uint32_t threads = 1);

/**
Create RT boolean tool fracturer
*/
NVBLAST_API Nv::Blast::Fracturer* NvBlastExtRTCreateFracturer();

/**
Perform fracture.
*/
NVBLAST_API uint32_t NvBlastExtRTDoFracture(const Nv::Blast::FractureDesc& desc, int32_t stage = Nv::Blast::FractureRT::Stage::ALL, int32_t threadId = 0, int32_t threadCount = 1);

/**
Create mesh generator
*/
NVBLAST_API Nv::Blast::MeshGenerator* NvBlastExtRTCreateMeshGenerator();

/**
Build mesh from output of RT fracture
*/
NVBLAST_API uint32_t NvBlastExtRTBuildMesh(Nv::Blast::MeshDesc dsc, Nv::Blast::Vertex* outVertices, uint32_t& vCount, uint32_t* indices, Nv::Blast::PerTriangleAdditionalData* adata, uint32_t maxICount, uint32_t maxVCount);

/**
TODO
*/
NVBLAST_API uint32_t NvBlastExtRTGetChunksToUnite(Nv::Blast::DamagePattern* pattern, const Nv::Blast::Vertex* vertices, const uint32_t* voffsets, uint32_t chunksCount, uint32_t* chunksToUnite);

/**
TODO
*/
NVBLAST_API uint32_t NvBlastExtRTDetectIslands(Nv::Blast::Vertex* vertices, uint32_t* offsets, NvcBounds3* bounds, uint32_t chunkCount, Nv::Blast::ChunkGraph* graph, uint32_t* islandChunks, uint32_t* islandOffsets);

/**
TODO
*/
NVBLAST_API Nv::Blast::ChunkGraph* NvBlastExtRTCreateChunkGraph(uint32_t maxLinksCount = 4096);

/**
TODO
*/
NVBLAST_API void NvBlastExtRTCookMergedMesh(Nv::Blast::DamagePattern* pattern);



#endif // ifndef NVBLASTEXTRT_H
