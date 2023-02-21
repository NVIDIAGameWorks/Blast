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
// Copyright (c) 2016-2023 NVIDIA Corporation. All rights reserved.



#include "NvBlastExtRT.h"
#include "NvBlastExtRTImpl.h"
#include "NvBlastExtRTMultithreadedImpl.h"
#include "NvBlastExtAuthoringMeshImpl.h"
#include <NvBlastAssert.h>
#include "NvBlastExtRTGeometry.h"
#include "NvBlastGlobals.h"
#include "NvBlastExtAuthoringPatternGenerator.h"
#include "NvBlastNvSharedHelpers.h"

#include <map>
#include <set>
#include <algorithm>
#include <iostream>


using namespace Nv::Blast;


void sortResultBuffer(FacetFacetResult*& ffResultBuffer, uint32_t ffCount)
{
    std::sort(ffResultBuffer, ffResultBuffer + ffCount, [](const FacetFacetResult& ffr1, const FacetFacetResult& ffr2)
    {
        if (ffr1.parentFacet == ffr2.parentFacet)
        {
            if (ffr1.parentEdge == ffr2.parentEdge)
            {
                return ffr1.adjacentFacet < ffr2.adjacentFacet;
            }
            return ffr1.parentEdge < ffr2.parentEdge;
        }
        return ffr1.parentFacet < ffr2.parentFacet;
    });


    ffResultBuffer[ffCount].parentFacet = 0xFFFFFFFF;
}

#ifdef USE_MERGED_MESH
void procesOutputEdges(BooleanToolOutputData* outputData, DamagePattern* pattern, uint32_t facetCount)
{
    for (uint32_t e = 0; e < outputData->edgesCount(); e++)
    {
        uint32_t bFacet = outputData->edges[e].parentFacet;
        bool inverseNormal = false;
        if (bFacet >= facetCount)
        {
            bFacet -= facetCount;
            inverseNormal = true;
        }
        else
        {
            bFacet = outputData->edges[e].adjacentFacet;
        }
        auto& chunks = pattern->mergedFacetToChunkMap[bFacet];
        ((BooleanResultEdge*)pattern->outputEdges)[chunks.first * BLASTRT_MAX_EDGES_PER_CHUNK + pattern->outputEdgesCount[chunks.first]++] = outputData->edges[e];
        if (chunks.first != chunks.second)
        {
            BooleanResultEdge& edge = ((BooleanResultEdge*)pattern->outputEdges)[chunks.second * BLASTRT_MAX_EDGES_PER_CHUNK + pattern->outputEdgesCount[chunks.second]++];
            edge = outputData->edges[e].getInversed();
            if (inverseNormal)
            {
                Vertex st = outputData->vertices[outputData->edges[e].end]; st.n = -st.n;
                Vertex end = outputData->vertices[outputData->edges[e].start]; end.n = -end.n;
                edge.start = outputData->addVertex(st);
                edge.end = outputData->addVertex(end);
            }
        }
    }
}
#endif

//PreparedMesh* NvBlastExtRTCreatePreparedMesh(const Nv::Blast::Mesh* in)
//{
//  PreparedMesh* ret = NVBLAST_NEW(PreparedMesh);
//  uint32_t edgesCount = 0;
//  for (uint32_t i = 0; i < in->getFacetCount(); ++i)
//  {
//      uint32_t ec = in->getFacet(i)->edgesCount;
//      edgesCount += ec & 1 ? ec + 1 : ec;
//  }
//  edgesCount *= 2;
//  ret->px = (float*)NVBLAST_ALLOC(edgesCount * sizeof(float));
//  ret->py = (float*)NVBLAST_ALLOC(edgesCount * sizeof(float));
//  ret->pz = (float*)NVBLAST_ALLOC(edgesCount * sizeof(float));
//
//  ret->nx = (float*)NVBLAST_ALLOC(edgesCount * sizeof(float));
//  ret->ny = (float*)NVBLAST_ALLOC(edgesCount * sizeof(float));
//  ret->nz = (float*)NVBLAST_ALLOC(edgesCount * sizeof(float));
//
//  ret->u = (float*)NVBLAST_ALLOC(edgesCount * sizeof(float));
//  ret->v = (float*)NVBLAST_ALLOC(edgesCount * sizeof(float));
//
//  ret->facetOffsets = (uint32_t*)NVBLAST_ALLOC((in->getFacetCount() + 1) * sizeof(uint32_t));
//  uint32_t savedPoints = 0;
//  ret->facetOffsets[0] = 0;
//
//
//  const Vertex* vbuf = in->getVertices();
//  const Edge* ebuf = in->getEdges();
//
//  for (uint32_t i = 0; i < in->getFacetCount(); ++i)
//  {
//      const auto fc = in->getFacet(i);
//      for (uint32_t ed = 0; ed < fc->edgesCount; ++ed)
//      {
//          ret->px[savedPoints] = vbuf[ebuf[fc->firstEdgeNumber + ed].s].p.x;
//          ret->py[savedPoints] = vbuf[ebuf[fc->firstEdgeNumber + ed].s].p.y;
//          ret->pz[savedPoints] = vbuf[ebuf[fc->firstEdgeNumber + ed].s].p.z;
//
//          savedPoints++;
//          ret->px[savedPoints] = vbuf[ebuf[fc->firstEdgeNumber + ed].e].p.x;
//          ret->py[savedPoints] = vbuf[ebuf[fc->firstEdgeNumber + ed].e].p.y;
//          ret->pz[savedPoints] = vbuf[ebuf[fc->firstEdgeNumber + ed].e].p.z;
//          savedPoints++;
//      }
//      if (fc->edgesCount & 1)
//      {
//          ret->px[savedPoints] = ret->px[savedPoints - 1];
//          ret->py[savedPoints] = ret->py[savedPoints - 1];
//          ret->pz[savedPoints] = ret->pz[savedPoints - 1];
//          savedPoints++;
//          ret->px[savedPoints] = ret->px[savedPoints - 1];
//          ret->py[savedPoints] = ret->py[savedPoints - 1];
//          ret->pz[savedPoints] = ret->pz[savedPoints - 1];
//          savedPoints++;
//      }
//      ret->facetOffsets[i + 1] = savedPoints;
//  }
//  return ret;
//}
//
//void PreparedMesh::release()
//{
//  NVBLAST_FREE(px);
//  NVBLAST_FREE(py);
//  NVBLAST_FREE(pz);
//  NVBLAST_FREE(nx);
//  NVBLAST_FREE(ny);
//  NVBLAST_FREE(nz);
//  NVBLAST_FREE(u);
//  NVBLAST_FREE(v);
//  NVBLAST_FREE(facetOffsets);
//
//  NVBLAST_DELETE(this, PreparedMesh);
//}

bool compareEdge(const BooleanResultEdge& e1, const BooleanResultEdge& e2)
{
    return e1.parentFacet < e2.parentFacet;
}

uint32_t NvBlastExtRTDoFracture(const FractureDesc& desc, int32_t stage, int32_t threadId, int32_t threadCount)
{
    if (desc.fr == nullptr)
    {
        return 1;
    }
#ifndef USE_MERGED_MESH
    threadId = 0;
    threadCount = 1;
#endif
    BooleanToolV2* bt = reinterpret_cast<BooleanToolV2*>(desc.fr);

    bt->mMeshA = desc.model;
    bt->mMeshB = desc.cell;

    DummyAccelerator dmb(desc.cell->getFacetCount());
    if (desc.cellAccel == nullptr)
    {
        bt->mAccelB = &dmb;
    }
    else
    {
        bt->mAccelB = desc.cellAccel;
    }

    //bt->mPreparedA = desc.prepA;
    //bt->mPreparedB = desc.prepB;

    DummyAccelerator dma(desc.model->getFacetCount());
    if (desc.modelAccel == nullptr)
    {
        bt->mAccelA = &dma;
    }
    else
    {
        bt->mAccelA = desc.modelAccel;
    }

    if (stage & FractureRT::Stage::FACET_FACET_TEST)
    {
        if (threadCount == 1)
        {
            desc.outputData->reset();
        }

        bt->mAccelA->setPointCmpDirection(-1);
        bt->mAccelB->setPointCmpDirection(1);
        bt->makeFacetFacetTests(desc.outputData, threadId, threadCount);
    }

    if (stage == (int32_t)FractureRT::Stage::ALL)
    {
        sortResultBuffer(desc.outputData->ffResult, desc.outputData->ffResultCount());
    }
    if (stage & FractureRT::Stage::RETAIN_FROM_PATTERN)
    {
        bt->mAccelA->setPointCmpDirection(-1);
        bt->mAccelB->setPointCmpDirection(1);
        bt->retain(false, desc.outputData, threadId, threadCount);
    }
    //if (stage == MultithreadingStage::ALL)
    //{
    //  bt->procesOutputEdges(desc.pattern);
    //}
    if (stage & FractureRT::Stage::RETAIN_FROM_FRACTURED_MESH)
    {
        bt->retain(true, desc.outputData, 0, 1, desc.pattern, desc.chunkId);
    }
    if (stage == (int32_t)FractureRT::Stage::ALL)
    {
        std::sort(desc.outputData->edges, desc.outputData->edges + desc.outputData->edgesCount(), compareEdge);
    }

    return 0;
}

uint32_t NvBlastExtRTBuildMesh(MeshDesc dsc, Nv::Blast::Vertex* outVertices, uint32_t& vCount, uint32_t* indices, PerTriangleAdditionalData* adata, uint32_t maxICount, uint32_t maxVCount)
{
    NV_UNUSED(maxVCount);

    TriangulatorV2* trg = reinterpret_cast<TriangulatorV2*>(dsc.tr);
    return trg->build(dsc.bEdges, dsc.edesCount, dsc.inVertices, outVertices, vCount, indices, adata, maxICount, dsc.meshA, dsc.meshB);
}

Fracturer* NvBlastExtRTCreateFracturer()
{
    return NVBLAST_NEW(BooleanToolV2);
}

MeshGenerator* NvBlastExtRTCreateMeshGenerator()
{
    return NVBLAST_NEW(TriangulatorV2);
}

uint32_t NvBlastExtRTGetChunksToUnite(DamagePattern* pattern, const Vertex* vertices,const uint32_t* voffsets, uint32_t chunksCount, uint32_t* chunksToUnite)
{
    uint32_t count = 0;

    float r2 = pattern->activationRadius * pattern->activationRadius;

    for (uint32_t i = 0; i < chunksCount; ++i)
    {
        uint32_t ofs = voffsets[i];
        uint32_t tc = voffsets[i + 1] - voffsets[i];

        for (uint32_t k = 0; k < tc; ++k)
        {

            float dd = toNvShared(vertices[k + ofs].p).magnitudeSquared();
            if (pattern->activationType == DamagePattern::Line)
            {
                dd -= vertices[k + ofs].p.z *  vertices[k + ofs].p.z;
            }
            if (pattern->activationType == DamagePattern::Cone)
            {
                float adz = nvidia::NvAbs(vertices[k + ofs].p.z) * nvidia::NvTan(pattern->angle * nvidia::NvPi / 180.f); // angle correction

                dd -= vertices[k + ofs].p.z *  vertices[k + ofs].p.z + adz * adz;
            }
            if (dd > r2)
            {
                chunksToUnite[count++] = i;
                break;
            }
        }
    }
    return count;
}

FractureRT* NvBlastExtRTCreateFractureRT(uint32_t threads)
{
    if (threads <= 1)
    {
        return NVBLAST_NEW(FractureRTImpl);
    }
    return NVBLAST_NEW(FractureRTMultithreadedImpl)(threads);
};

bool haveCommonVertices(Vertex* vertices, uint32_t* offsets, uint32_t chunkA, uint32_t chunkB, nvidia::NvBounds3& bA, nvidia::NvBounds3& bB)
{
    NV_UNUSED(bA);

    for (uint32_t i = offsets[chunkA]; i < offsets[chunkA + 1]; ++i)
    {
        if (bB.contains(toNvShared(vertices[i].p)) == false) continue;
        for (uint32_t j = offsets[chunkB]; j < offsets[chunkB + 1]; ++j)
        {
            if (nvidia::NvAbs(vertices[i].p.x - vertices[j].p.x) < 1e-3f && nvidia::NvAbs(vertices[i].p.y - vertices[j].p.y) < 1e-3f && nvidia::NvAbs(vertices[i].p.z - vertices[j].p.z) < 1e-3f)
                return true;
        }
    }
    return false;
}
bool haveCommonVertices(Vertex* vertices, uint32_t* offsets, uint32_t chunkA, uint32_t chunkB)
{

    for (uint32_t i = offsets[chunkA]; i < offsets[chunkA + 1]; ++i)
    {
        for (uint32_t j = offsets[chunkB]; j < offsets[chunkB + 1]; ++j)
        {
            if (nvidia::NvAbs(vertices[i].p.x - vertices[j].p.x) < 1e-3f && nvidia::NvAbs(vertices[i].p.y - vertices[j].p.y) < 1e-3f && nvidia::NvAbs(vertices[i].p.z - vertices[j].p.z) < 1e-3f)
                return true;
        }
    }
    return false;
}

struct ChunkGraphImpl : public ChunkGraph
{
    ChunkGraphImpl(uint32_t maxLinksCount);
    virtual void eraseNode(uint32_t index);
    virtual void release();
};

ChunkGraphImpl::ChunkGraphImpl(uint32_t maxLinksCount)
{
    links = (ChunkGraphLink*)NVBLAST_ALLOC(sizeof(ChunkGraphLink) * maxLinksCount);
    dirtyChunks = (uint32_t*)NVBLAST_ALLOC(sizeof(uint32_t) * maxLinksCount);
    this->maxLinksCount = maxLinksCount;
}

void ChunkGraphImpl::eraseNode(uint32_t index)
{
    for (int32_t i = dirtyChunksCount - 1; i >= 0; i--)
    {
        if (dirtyChunks[i] == index)
        {
            std::swap(dirtyChunks[i], dirtyChunks[--dirtyChunksCount]);
        }
        else if (dirtyChunks[i] > index)
        {
            --dirtyChunks[i];
        }
    }
    for (int32_t i = linksCount - 1; i >= 0; --i)
    {
        if (links[i].l2 == index || links[i].l1 == index)
        {
            if (links[i].l1 == index)
            {

                dirtyChunks[dirtyChunksCount++] = (links[i].l2 > index) ? links[i].l2 - 1 : links[i].l2;
            }
            if (links[i].l2 == index)
            {
                dirtyChunks[dirtyChunksCount++] = (links[i].l1 > index) ? links[i].l1 - 1 : links[i].l1;
            }
            std::swap(links[--linksCount], links[i]);
        }
        else
        {
            if (links[i].l1 > index) links[i].l1--;
            if (links[i].l2 > index) links[i].l2--;
        }
    }
    std::sort(dirtyChunks, &dirtyChunks[dirtyChunksCount]);
    dirtyChunksCount = (uint32_t)(std::unique(dirtyChunks, &dirtyChunks[dirtyChunksCount]) - dirtyChunks);
}

void ChunkGraphImpl::release()
{
    if (links)
    {
        NVBLAST_FREE(links);
    }
    if (dirtyChunks)
    {
        NVBLAST_FREE(dirtyChunks);
    }
    NVBLAST_DELETE(this, ChunkGraphImpl);
}

ChunkGraph* NvBlastExtRTCreateChunkGraph(uint32_t maxLinksCount)
{

    return NVBLAST_NEW(ChunkGraphImpl)(maxLinksCount);
}



//static bool operator<(const uint32_t in, ChunkGraphLink& b)
//{
//  return in < b.l1;
//}

bool ChunkGraphLink::operator<(const ChunkGraphLink& lk) const
{
    return l1 < lk.l1;
}
bool ChunkGraphLink::operator<(const uint32_t in) const
{
    return l1 < in;
}
bool upperBoundCmp(uint32_t left, const ChunkGraphLink& lk)
{
    return left < lk.l1;
}

uint32_t NvBlastExtRTDetectIslands(Vertex* vertices, uint32_t* offsets, nvidia::NvBounds3* bounds, uint32_t chunkCount, ChunkGraph* graph, uint32_t* islandChunks, uint32_t* islandOffsets)
{
    uint32_t firstNewChunk = chunkCount - graph->newlyAddedCount;
    for (uint32_t i = firstNewChunk; i < chunkCount; ++i)
    {
        for (uint32_t j = i + 1; j < chunkCount; ++j)
        {
        
            if (haveCommonVertices(vertices, offsets, i, j, bounds[i], bounds[j]))
            {
                graph->links[graph->linksCount++] = ChunkGraphLink(i, j);
                graph->links[graph->linksCount++] = ChunkGraphLink(j, i);

            }
        }
    }
    for (uint32_t i = firstNewChunk; i < chunkCount; ++i)
    {
        for (uint32_t j = 0; j < graph->dirtyChunksCount; ++j)
        {
            uint32_t dj = graph->dirtyChunks[j];
            if (weakBoundingBoxIntersection(bounds[i], bounds[dj]) == false) continue;
            if (haveCommonVertices(vertices, offsets, i, dj, bounds[i], bounds[dj]))
            {
                graph->links[graph->linksCount++] = ChunkGraphLink(i, dj);
                graph->links[graph->linksCount++] = ChunkGraphLink(dj, i);
                NVBLAST_ASSERT(graph->linksCount < graph->maxLinksCount);
            }
        }
    }
    graph->newlyAddedCount = 0;
    graph->dirtyChunksCount = 0;

    std::sort(graph->links, &graph->links[graph->linksCount]);


    uint32_t islandCount = 0;
    std::vector<uint32_t> islandIndex(chunkCount, 0);
    std::vector<uint32_t> vstack;
    vstack.reserve(1024);



    for (uint32_t ch = 0; ch < chunkCount; ++ch)
    {
        if (islandIndex[ch] != 0) continue;
            
        islandCount++;      
        vstack.push_back(ch);

        while (!vstack.empty())
        {
            uint32_t currentChunk = vstack.back();
            islandIndex[currentChunk] = islandCount;
            vstack.pop_back();

            uint32_t sps = (uint32_t)(std::lower_bound(graph->links, graph->links + graph->linksCount, currentChunk) - graph->links);
            uint32_t eps = (uint32_t)(std::upper_bound(graph->links, graph->links + graph->linksCount, currentChunk, upperBoundCmp) - graph->links);

            for (uint32_t i = sps; i < eps; ++i)
            {
                auto& l = graph->links[i];
                if (l.l1 == currentChunk && islandIndex[l.l2] == 0)
                {
                    vstack.push_back(l.l2);
                    islandIndex[l.l2] = islandCount;
                }
                if (l.l2 == currentChunk && islandIndex[l.l1] == 0)
                {
                    vstack.push_back(l.l1);
                    islandIndex[l.l1] = islandCount;
                }
            }
        }
    }

    uint32_t chcount = 0;
    islandOffsets[0] = 0;
    for (uint32_t ild = 1; ild <= islandCount; ++ild)
    {
        for (uint32_t ch = 0; ch < chunkCount; ++ch)
        {
            if (islandIndex[ch] == ild)
            {
                islandChunks[chcount++] = ch;
            }
        }
        islandOffsets[ild] = chcount;
    }
    return islandCount;
}


void NvBlastExtRTCookMergedMesh(DamagePattern* pattern)
{
    NV_UNUSED(pattern);
#ifdef USE_MERGED_MESH
    VertexWelding wld(8192, 512, 0.001, 0.0001, 0.0001, &VertexWelding::LocateVertexInBucket);

    uint32_t cellsCount = pattern->cellsCount;
    std::vector<Edge> edges;
    std::vector<Facet> facets;
    std::map<uint32_t, uint32_t> facetsMap;

    for (uint32_t i = 0; i < cellsCount; i++)
    {
        auto m = pattern->cellsMeshes[i];
        auto me = m->getEdges();
        auto mv = m->getVertices();
        auto mf = m->getFacetsBuffer();
        for (uint32_t f = 0; f < m->getFacetCount(); f++)
        {
            const Facet& facetOriginal = mf[f];

            auto facetIt = facetsMap.find(-facetOriginal.userData);
            if (facetIt == facetsMap.end())
            {
                if (facetOriginal.userData != 0)
                {
                    facetIt = facetsMap.insert(std::make_pair(facetOriginal.userData, facets.size())).first;
                }
                facets.push_back(facetOriginal);
                pattern->mergedFacetToChunkMap.push_back(std::make_pair(i, i));
                auto& facetRef = facets.back();
                facetRef.firstEdgeNumber = edges.size();
                facetRef.edgesCount = facetOriginal.edgesCount;
                for (uint32_t e = 0; e < facetOriginal.edgesCount; e++)
                {
                    edges.push_back(Edge(wld.WeldVertex(&mv[me[facetOriginal.firstEdgeNumber + e].s]),
                        wld.WeldVertex(&mv[me[facetOriginal.firstEdgeNumber + e].e])));
                }
            }
            else
            {
                NVBLAST_ASSERT(pattern->mergedFacetToChunkMap[facetIt->second].second == pattern->mergedFacetToChunkMap[facetIt->second].first);
                pattern->mergedFacetToChunkMap[facetIt->second].second = i;
            }
        }
    }
    pattern->validFacetsForChunk = (bool**)NVBLAST_ALLOC(sizeof(bool *) * cellsCount);
    for (uint32_t i = 0; i < cellsCount; i++)
    {
        pattern->validFacetsForChunk[i] = (bool*)NVBLAST_ALLOC(sizeof(bool) * facets.size());
        memset(pattern->validFacetsForChunk[i], 0, sizeof(bool) * facets.size());
    }
    for (uint32_t j = 0; j < facets.size(); j++)
    {
        pattern->validFacetsForChunk[pattern->mergedFacetToChunkMap[j].first][j] = true;
        pattern->validFacetsForChunk[pattern->mergedFacetToChunkMap[j].second][j] = true;
    }

    pattern->mergedMesh = new MeshImpl(wld.getVertices(), edges.data(), facets.data(), wld.getVerticesCount(), edges.size(), facets.size());
    pattern->preparedMergedMesh = NvBlastExtRTCreatePreparedMesh(pattern->mergedMesh);
#endif
}
