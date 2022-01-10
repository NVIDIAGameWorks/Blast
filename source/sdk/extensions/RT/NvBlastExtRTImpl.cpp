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

#define _CRT_SECURE_NO_WARNINGS

#include "NvBlastExtRTImpl.h"
#include "NvBlastExtRTGeometry.h"
#include "NvBlastExtAuthoringMeshImpl.h"
#include "NvBlastExtAuthoringAcceleratorImpl.h"
#include "NvBlastExtAuthoringPatternGenerator.h"
#include "NvBlastPxSharedHelpers.h"

#include "NvBlastGlobals.h"
#include "NvBlastAssert.h"
//#include "PxPhysicsAPI.h"

#define SAFE_RELEASE(x) if (x) {(x)->release(); (x) = nullptr;}
#define SAFE_FREE(x) if (x) {NVBLAST_FREE(x); (x) = nullptr;}

using namespace Nv::Blast;

BooleanToolOutputDataImpl::BooleanToolOutputDataImpl(bool createOnlyEdgeBuffer)
{
    edges = (BooleanResultEdge*)NVBLAST_ALLOC(sizeof(BooleanResultEdge) * BLASTRT_MAX_EDGES_PER_CHUNK);
    mEdgesCount = new std::atomic<uint32_t>;

    if (createOnlyEdgeBuffer)
    {
        vertices = nullptr;
        ffResult = nullptr;
        isVerticesAndResultsAllocated = false;
    }
    else
    {
        vertices = (Vertex*)NVBLAST_ALLOC(sizeof(Vertex) * BLASTRT_MAX_VERTICES);
        ffResult = (FacetFacetResult*)NVBLAST_ALLOC(sizeof(FacetFacetResult) * BLASTRT_MAX_EDGES_PER_CHUNK);
        mVerticesCount = new std::atomic<uint32_t>;
        mFfResultCount = new std::atomic<uint32_t>;
    }

    reset();
}

void BooleanToolOutputDataImpl::release()
{
    SAFE_FREE(edges);
    delete mEdgesCount;

    if (isVerticesAndResultsAllocated)
    {
        SAFE_FREE(vertices);
        SAFE_FREE(ffResult);

        delete mVerticesCount;
        delete mFfResultCount;
    }

    delete this;
}

void BooleanToolOutputDataImpl::reset()
{
    *mEdgesCount = 0;
    if (vertices != nullptr)
    {
        *mVerticesCount = 0;
        *mFfResultCount = 0;
    }
}

void BooleanToolOutputDataImpl::resetEdges()
{
    *mEdgesCount = 0;
}

void BooleanToolOutputDataImpl::copyVerticesAndResults(const BooleanToolOutputData* other)
{
    if (isVerticesAndResultsAllocated)
    {
        SAFE_FREE(vertices);
        SAFE_FREE(ffResult);
        delete mVerticesCount;
        delete mFfResultCount;
    }
    mVerticesCount = ((const BooleanToolOutputDataImpl*)other)->mVerticesCount;
    vertices = other->vertices;
    mFfResultCount = ((const BooleanToolOutputDataImpl*)other)->mFfResultCount;
    ffResult = other->ffResult;
    
}

uint32_t BooleanToolOutputDataImpl::addEdge(const BooleanResultEdge& e)
{
    uint32_t i = (*mEdgesCount)++;
    edges[i] = e;
    return i;
}

uint32_t BooleanToolOutputDataImpl::addVertex(const Vertex& v)
{
    uint32_t i = (*mVerticesCount)++;
    vertices[i] = v;
    return i;
}

uint32_t BooleanToolOutputDataImpl::addFfResult(const FacetFacetResult& r)
{
    uint32_t i = (*mFfResultCount)++;
    ffResult[i] = r;
    return i;
}

BooleanResultEdge& BooleanToolOutputDataImpl::getNewEdge()
{
    return edges[(*mEdgesCount)++];
}

Vertex& BooleanToolOutputDataImpl::getNewVertex()
{
    return vertices[(*mVerticesCount)++];
}

FacetFacetResult& BooleanToolOutputDataImpl::getNewFfResult()
{
    return ffResult[(*mFfResultCount)++];
}


BooleanToolOutputData* CreateBooleanToolOutputData(bool isMergedMesh)
{
    return new BooleanToolOutputDataImpl(isMergedMesh);
}

FractureRTImpl::FractureRTImpl()
{
    verticesBuffer = (Vertex*)NVBLAST_ALLOC(sizeof(Vertex) * BLASTRT_MAX_VERTICES);
    indexBuffer = (uint32_t*)NVBLAST_ALLOC(sizeof(uint32_t) * BLASTRT_MAX_CHUNKS * BLASTRT_MAX_EDGES_PER_CHUNK);

    vertexOffset = (uint32_t*)NVBLAST_ALLOC(sizeof(uint32_t) * (BLASTRT_MAX_CHUNKS + 1));
    indexOffset = (uint32_t*)NVBLAST_ALLOC(sizeof(uint32_t) * (BLASTRT_MAX_CHUNKS + 1));
    adata = (PerTriangleAdditionalData*)NVBLAST_ALLOC(sizeof(PerTriangleAdditionalData) * BLASTRT_MAX_CHUNKS * BLASTRT_MAX_EDGES_PER_CHUNK);

    resultChunkCount = 0;
    btool = (BooleanToolV2*)NvBlastExtRTCreateFracturer();
    triangulator = (TriangulatorV2*)NvBlastExtRTCreateMeshGenerator();
    outputData = (BooleanToolOutputDataImpl*)CreateBooleanToolOutputData(false);
}

void FractureRTImpl::release()
{
    SAFE_FREE(verticesBuffer);
    SAFE_FREE(indexBuffer);
    SAFE_FREE(vertexOffset);
    SAFE_FREE(indexOffset);
    SAFE_FREE(adata);
    SAFE_RELEASE(btool);
    SAFE_RELEASE(triangulator);
    SAFE_RELEASE(outputData);

    NVBLAST_DELETE(this, FractureRTImpl);
}

#ifdef USE_MERGED_MESH
static bool compareEdge(const BooleanResultEdge& e1, const BooleanResultEdge& e2)
{
    return e1.parentFacet < e2.parentFacet;
}
#endif

//Debug code
//struct TmpE
//{
//  physx::PxVec3 v1;
//  physx::PxVec3 v2;
//  int f;
//};
//struct CmpTmpE
//{
//  CmpVec cv;
//  bool operator()(const TmpE& t1, const TmpE& t2) const
//  {
//      if (t1.f == t2.f)
//      {
//          if (cv(t1.v1, t2.v1) == cv(t2.v1, t1.v1))
//          {
//              return cv(t1.v2, t2.v2);
//          }
//          return cv(t1.v1, t2.v1);
//      }
//      return t1.f < t2.f;
//  }
//};

void FractureRTImpl::processMesh(Nv::Blast::DamagePattern* pattern, const Mesh* meshToFracture)
{
    NVBLAST_ASSERT(pattern);
    NVBLAST_ASSERT(meshToFracture);
    //NVBLAST_ASSERT(preparedMeshA);
        
    resultChunkCount = 0;
    outputData->reset();
    
    Grid grd(3);
    grd.setMesh(meshToFracture);
    GridAccelerator accel(&grd);
    
    btool->mAccelA = &accel;
    btool->mMeshA = meshToFracture;
    //btool->mPreparedA = preparedMeshA;

    int32_t spaceLeft = BLASTRT_MAX_CHUNKS * BLASTRT_MAX_EDGES_PER_CHUNK / 3;
    vertexOffset[0] = 0;
    indexOffset[0] = 0;

    Vertex* outputVerticesBuffer = verticesBuffer;
    uint32_t* outputIndexBuffer = indexBuffer;
    PerTriangleAdditionalData* outputAdataBuffer = adata;

    FractureDesc dsc;
    dsc.fr = btool;
    dsc.model = meshToFracture;
    //dsc.prepA = preparedMeshA;
    dsc.modelAccel = &accel;
    dsc.outputData = outputData;

#ifdef USE_MERGED_MESH
    for (uint32_t i = 0; i < pattern->cellsCount; i++)
    {
        pattern->outputEdgesCount[i] = 0;
    }

    dsc.cell = pattern->mergedMesh;
    dsc.prepB = pattern->preparedMergedMesh;
    dsc.chunkId = 0;
    dsc.pattern = pattern;


    NvBlastExtRTDoFracture(dsc, FractureRT::Stage::FACET_FACET_TEST);

    sortResultBuffer(outputData->ffResult, outputData->ffResultCount());


    NvBlastExtRTDoFracture(dsc, FractureRT::Stage::RETAIN_FROM_PATTERN);

    procesOutputEdges(outputData, pattern, meshToFracture->getFacetCount());

#endif

    for (uint32_t chunk = 0; chunk < pattern->cellsCount; ++chunk)
    {
        dsc.cell = pattern->cellsMeshes[chunk];
        dsc.chunkId = chunk;

#ifdef USE_MERGED_MESH
        outputData->resetEdges();

        NvBlastExtRTDoFracture(dsc, FractureRT::Stage::RETAIN_FROM_FRACTURED_MESH);

        auto edges = (BooleanResultEdge*)pattern->outputEdges + chunk * BLASTRT_MAX_EDGES_PER_CHUNK;
        for (uint32_t e = 0; e < outputData->edgesCount(); e++)
        {
            edges[pattern->outputEdgesCount[chunk]++] = outputData->edges[e];
        }
        auto edgesCount = pattern->outputEdgesCount[chunk];
        std::sort(edges, edges + edgesCount, compareEdge);
#else
        NvBlastExtRTDoFracture(dsc);
        auto edges = outputData->edges;
        uint32_t edgesCount = outputData->edgesCount();
#endif

        uint32_t nvc = 0;
        uint32_t newTriangles = triangulator->build(edges, edgesCount, outputData->vertices, outputVerticesBuffer, nvc, outputIndexBuffer, outputAdataBuffer, spaceLeft, meshToFracture, pattern->cellsMeshes[chunk]);
        
        if (newTriangles)
        {
            /**
            Check if generated mesh bounding box is valid. 
            */
            physx::PxBounds3 bds(physx::PxBounds3::empty());
            for (uint32_t i = 0; i < newTriangles * 3; ++i)
            {
                bds.include(toPxShared(outputData->vertices[outputIndexBuffer[i]].p));
            }
            if (bds.isValid())
            {
                spaceLeft -= newTriangles;
                outputIndexBuffer += 3 * newTriangles;
                outputVerticesBuffer += nvc;
                outputAdataBuffer += newTriangles;
                indexOffset[resultChunkCount + 1] = indexOffset[resultChunkCount] + newTriangles * 3;
                vertexOffset[resultChunkCount + 1] = vertexOffset[resultChunkCount] + nvc;
                resultChunkCount++;
            }
        }
    }
}

void FractureRTImpl::dumpChunksToObj(const char* path)
{
    FILE* output = fopen(path, "w");

    for (uint32_t i = 0; i < vertexOffset[resultChunkCount]; ++i)
    {
        fprintf(output, "v %f %f %f\n", verticesBuffer[i].p.x, verticesBuffer[i].p.y, verticesBuffer[i].p.z);
    }
    
    for (uint32_t i = 0; i < vertexOffset[resultChunkCount]; ++i)
    {
        fprintf(output, "vn %f %f %f\n", verticesBuffer[i].n.x, verticesBuffer[i].n.y, verticesBuffer[i].n.z);
    }
    for (uint32_t i = 0; i < vertexOffset[resultChunkCount]; ++i)
    {
        fprintf(output, "vt %f %f\n", verticesBuffer[i].uv[0].x, verticesBuffer[i].uv[0].y);
    }
    
    for (uint32_t chunk = 0; chunk < resultChunkCount; ++chunk)
    {
        fprintf(output, "g chunk_%d\n", chunk);
        uint32_t trc = (indexOffset[chunk + 1] - indexOffset[chunk]) / 3;
        uint32_t* idx = &indexBuffer[indexOffset[chunk]];
        uint32_t vofs = vertexOffset[chunk] + 1;
        for (uint32_t t = 0; t < trc; ++t)
        {
            fprintf(output, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", vofs + idx[0], vofs + idx[0], vofs + idx[0],
                vofs + idx[1], vofs + idx[1], vofs + idx[1],
                vofs + idx[2], vofs + idx[2], vofs + idx[2]);
            idx += 3;
        }
    }
    fclose(output);
}

