#include "NvBlastExtRTGeometry.h"
#include "NvBlastExtAuthoringMesh.h"
#include "NvBlastGlobals.h"
#include <NvBlastAssert.h>
#include "PsVecMath.h"
#include "NvBlastExtAuthoringMeshImpl.h"
#include <map>
#include "NvBlastExtRT.h"
#include "NvBlastPxSharedHelpers.h"

#define USE_PREP_MESH 0

#define SAFE_RELEASE(x) if (x) {(x)->release(); (x) = nullptr;}
#define SAFE_FREE(x) if (x) {NVBLAST_FREE(x); (x) = nullptr;}

using namespace physx;
using namespace physx::shdfnd;

namespace Nv
{
    namespace Blast
    {   
        VertexWelding::VertexWelding(uint32_t maxVertexCount, uint32_t maxBucketCount, float gridCellSize, float weldEpsilon, float auxEpsilon, LOCATE_CALLBACK clbk)
            : maxVertexCount(maxVertexCount), maxBucketCount(maxBucketCount), gridCellSizeInv(1.f / gridCellSize), weldEpsilon(weldEpsilon), auxEpsilon(auxEpsilon), locateCallback(clbk)
        {
            vertex.reserve(maxVertexCount);
            first.resize(maxBucketCount);
            next.resize(maxVertexCount);
            reset();
        }
            
        void VertexWelding::reset()
        {
            memset(first.data(), -1, sizeof(int32_t) * maxBucketCount);
            memset(next.data(), -1, sizeof(int32_t) * vertex.size());
            vertex.clear();
        }

        // Computes hash bucket index in range [0, NUM_BUCKETS-1]
        int32_t VertexWelding::ComputeHashBucketIndex(int32_t x, int32_t y, int32_t z)
        {
            const int32_t h1 = 0x8da6b343; // Large multiplicative constants;
            const int32_t h2 = 0xd8163841; // here arbitrarily chosen primes
            const int32_t h3 = 0xcb1ab31f;
            int32_t p = h1 * x + h2 * y + h3 * z;
            p = p % maxBucketCount;
            if (p < 0)
            {
                p += maxBucketCount;
            }
            return p;
        }

        int32_t VertexWelding::LocateVertexInBucket(const Vertex& v, uint32_t bucket, bool& isAllDataTheSame)
        {
            // Scan through linked list of vertices at this bucket
            for (int32_t index = first[bucket]; index >= 0; index = next[index]) 
            {
                // Weld this vertex to existing vertex if within given distance tolerance
                if (toPxShared(vertex[index].p - v.p).magnitudeSquared() < weldEpsilon * weldEpsilon)
                {
                    isAllDataTheSame = (toPxShared(vertex[index].n - v.n).magnitudeSquared() < auxEpsilon * auxEpsilon
                        && toPxShared(vertex[index].uv[0] - v.uv[0]).magnitudeSquared() < auxEpsilon * auxEpsilon);
                    return index;
                }
            }
            return -1;
        }

        int32_t VertexWelding::LocateVertexInBucketOnlyPosition(const Vertex& v, uint32_t bucket, bool& isAllDataTheSame)
        {
            // Scan through linked list of vertices at this bucket
            for (int32_t index = first[bucket]; index >= 0; index = next[index])
            {
                // Weld this vertex to existing vertex if within given distance tolerance
                if (toPxShared(vertex[index].p - v.p).magnitudeSquared() < weldEpsilon * weldEpsilon)
                {
                    return index;
                }
            }
            return -1;
        }

        void VertexWelding::AddVertexToBucket(const Vertex& v, uint32_t bucket)
        {
            // Fill next available vertex buffer entry and link it into vertex list
            next[vertex.size()] = first[bucket];
            first[bucket] = vertex.size();
            vertex.push_back(v);
        }

        int32_t VertexWelding::WeldVertex(const Vertex *v)
        {
            // Make sure epsilon is not too small for the coordinates used!
            NVBLAST_ASSERT(v->p.x - weldEpsilon != v->p.x && v->p.x + weldEpsilon != v->p.x);
            NVBLAST_ASSERT(v->p.y - weldEpsilon != v->p.y && v->p.y + weldEpsilon != v->p.y);
            NVBLAST_ASSERT(v->p.z - weldEpsilon != v->p.z && v->p.z + weldEpsilon != v->p.z);
            // Compute cell coordinates of bounding box of vertex epsilon neighborhood
            int32_t front = int32_t((v->p.z - weldEpsilon) * gridCellSizeInv);
            int32_t top = int32_t((v->p.y - weldEpsilon) * gridCellSizeInv);
            int32_t left = int32_t((v->p.x - weldEpsilon) * gridCellSizeInv);
            int32_t right = int32_t((v->p.x + weldEpsilon) * gridCellSizeInv);
            int32_t bottom = int32_t((v->p.y + weldEpsilon) * gridCellSizeInv);
            int32_t back = int32_t((v->p.z + weldEpsilon) * gridCellSizeInv);
            // To lessen effects of worst-case behavior, track previously tested buckets
            uint32_t prevBucket[8]; // 4 in 2D, 8 in 3D
            int32_t numPrevBuckets = 0;

            bool isAllDataTheSame = true;
            // Loop over all overlapped cells and test against their buckets
            for (int32_t i = left; i <= right; i++) 
            {
                for (int32_t j = top; j <= bottom; j++) 
                {
                    for (int32_t k = front; k <= back; k++)
                    {
                        uint32_t bucket = ComputeHashBucketIndex(i, j, k);
                        // If this bucket already tested, don’t test it again
                        for (int b = 0; b < numPrevBuckets; b++)
                        {
                            if (bucket == prevBucket[b]) goto skipcell;
                        }
                        // Add this bucket to visited list, then test against its contents
                        prevBucket[numPrevBuckets++] = bucket;
                        //Vertex *weldVertex;
                        // Call function to step through linked list of bucket, testing
                        // if v is within the epsilon of one of the vertices in the bucket
                        {
                            int32_t weldVertexIndex = (this->*locateCallback)(*v, bucket, isAllDataTheSame);
                            if (weldVertexIndex >= 0)
                            {
                                if (isAllDataTheSame)
                                {
                                    return weldVertexIndex;
                                }
                                else
                                {
                                    Vertex newV(*v);
                                    newV.p = vertex[weldVertexIndex].p;
                                    AddVertexToBucket(newV, bucket);
                                    return vertex.size() - 1;
                                }
                            }
                        }
                    skipcell:;
                    }
                }
            }
            // Couldn’t locate vertex, so add it to grid, then return vertex itself
            int32_t x = int32_t(v->p.x * gridCellSizeInv);
            int32_t y = int32_t(v->p.y * gridCellSizeInv);
            int32_t z = int32_t(v->p.z * gridCellSizeInv);
            AddVertexToBucket(*v, ComputeHashBucketIndex(x, y, z));
            
            return vertex.size() - 1;
        }

        /**
            BASEMENT:
        */
        /**
            Vertex level shadowing functions
        */
        NV_FORCE_INLINE aos::VecI32V vertexShadowing(const aos::Vec4V& a, const aos::Vec4V& b)
        {
            return  aos::VecI32V_From_BoolV(aos::FIsGrtrOrEq(b, a));
        }

        /**
            Vertex-edge status functions
        */
        NV_FORCE_INLINE aos::VecI32V veStatus01(const aos::Vec4V&  sEdge, const aos::Vec4V&  eEdge, const aos::Vec4V&  p)
        {
            return aos::VecI32V_Sub(vertexShadowing(p, sEdge), vertexShadowing(p, eEdge));
        }

        NV_FORCE_INLINE aos::VecI32V veStatus10(const aos::Vec4V& sEdge, const aos::Vec4V&  eEdge, const aos::Vec4V&  p)
        {
            return aos::VecI32V_Sub(vertexShadowing(eEdge, p), vertexShadowing(sEdge, p));
        }

        NV_FORCE_INLINE int32_t inclusionValue03(BooleanToolV2::Mode& conf, int32_t xValue)
        {
            return conf.ca + conf.ci * xValue;
        }

        NV_FORCE_INLINE int32_t inclusionValueEdgeFace(BooleanToolV2::Mode& conf, int32_t xValue)
        {
            return conf.ci * xValue;
        }

        NV_FORCE_INLINE int32_t inclusionValue30(BooleanToolV2::Mode& conf, int32_t xValue)
        {
            return conf.cb + conf.ci * xValue;
        }

        NV_FORCE_INLINE int32_t vertexShadowing(const PxVec3& a, const PxVec3& b)
        {
            return (b.x >= a.x) ? 1 : 0;
        }

        NV_FORCE_INLINE int32_t veStatus01(const PxVec3& sEdge, const PxVec3& eEdge, const PxVec3& p)
        {
            return vertexShadowing(p, eEdge) - vertexShadowing(p, sEdge);
        }

        NV_FORCE_INLINE int32_t veStatus10(const PxVec3& sEdge, const PxVec3& eEdge, const PxVec3& p)
        {
            return -vertexShadowing(eEdge, p) + vertexShadowing(sEdge, p);
        }

        NV_FORCE_INLINE aos::Vec4V computeEdgeInterpolationParameter(const aos::Vec4V& sEdge, const aos::Vec4V& eEdge, const aos::Vec4V& p, float* tOutput)
        {
            aos::Vec4V t = aos::V4Sub(p, sEdge);
            aos::Vec4V t2 = aos::V4Sub(eEdge, sEdge);
            t = aos::V4Clamp(aos::V4Div(t, t2), aos::V4Load(0.0f), aos::V4Load(1.0f));
            aos::V4StoreA(t, tOutput);
            return t;
        }

        /**
        Vertex-edge shadowing functions
        */
        void shadowingPretest01(uint32_t CNT, const float* sEdge, const float* eEdge, const float* sEdgeY, const float* eEdgeY, const float* p, const float* py, float* tOutput, int32_t* windingOutput, int32_t* windingShadowedOutput, float* testPointProjectedY)
        {
            for (uint32_t cn = 0; cn < CNT; cn += 4)
            {
                aos::Vec4V sev = aos::V4LoadA(sEdge + cn);
                aos::Vec4V eev = aos::V4LoadA(eEdge + cn);
                aos::Vec4V pv = aos::V4LoadA(p + cn);


                aos::VecI32V winding = veStatus01(sev, eev, pv);
                aos::Vec4V t = computeEdgeInterpolationParameter(sev, eev, pv, tOutput + cn);
                aos::I4StoreA(winding, windingOutput + cn);

                /**
                    Work with Y part
                */
                sev = aos::V4LoadA(sEdgeY + cn);
                eev = aos::V4LoadA(eEdgeY + cn);
                pv = aos::V4LoadA(py + cn);

                t = aos::V4Add(sev, aos::V4Mul(t, aos::V4Sub(eev, sev)));
                aos::V4StoreA(t, testPointProjectedY + cn);
                winding = aos::VecI32V_And(winding, (aos::VecI32V)aos::V4IsGrtrOrEq(t, pv));
                aos::I4StoreA(winding, windingShadowedOutput + cn);
            }
        }

        void shadowingPretest10(uint32_t CNT, const float* sEdge, const float* eEdge, const float* sEdgeY, const float* eEdgeY, const float* p, const float* py, float* tOutput, int32_t* windingOutput, int32_t* windingShadowedOutput, float* testPointProjectedY)
        {
            for (uint32_t cn = 0; cn < CNT; cn += 4)
            {
                aos::Vec4V sev = aos::V4LoadA(sEdge + cn);
                aos::Vec4V eev = aos::V4LoadA(eEdge + cn);
                aos::Vec4V pv = aos::V4LoadA(p + cn);


                aos::VecI32V winding = veStatus10(sev, eev, pv);
                aos::Vec4V t = computeEdgeInterpolationParameter(sev, eev, pv, tOutput + cn);
                aos::I4StoreA(winding, windingOutput + cn);

                /**
                Work with Y part
                */
                sev = aos::V4LoadA(sEdgeY + cn);
                eev = aos::V4LoadA(eEdgeY + cn);
                pv = aos::V4LoadA(py + cn);

                t = aos::V4Add(sev, aos::V4Mul(t, aos::V4Sub(eev, sev)));
                aos::V4StoreA(t, testPointProjectedY + cn);
                winding = aos::VecI32V_And(winding, (aos::VecI32V)aos::V4IsGrtrOrEq(pv, t));
                aos::I4StoreA(winding, windingShadowedOutput + cn);
            }
        }

        NV_FORCE_INLINE void shadowingPretest01Single(const float* sEdge, const float* eEdge, const float* sEdgeY, const float* eEdgeY, const float* p, const float* py, float* tOutput, int32_t* windingOutput, int32_t* windingShadowedOutput, float* testPointProjectedY)
        {
            aos::Vec4V sev = aos::V4LoadA(sEdge);
            aos::Vec4V eev = aos::V4LoadA(eEdge);
            aos::Vec4V pv = aos::V4LoadA(p);


            aos::VecI32V winding = veStatus01(sev, eev, pv);
            aos::Vec4V t = computeEdgeInterpolationParameter(sev, eev, pv, tOutput);
            aos::I4StoreA(winding, windingOutput);

            /**
            Work with Y part
            */
            sev = aos::V4LoadA(sEdgeY);
            eev = aos::V4LoadA(eEdgeY);
            pv = aos::V4LoadA(py);

            t = aos::V4Add(sev, aos::V4Mul(t, aos::V4Sub(eev, sev)));
            aos::V4StoreA(t, testPointProjectedY);
            winding = aos::VecI32V_And(winding, (aos::VecI32V)aos::V4IsGrtrOrEq(t, pv));
            aos::I4StoreA(winding, windingShadowedOutput);
        }

        NV_FORCE_INLINE void shadowingPretest10Single(const float* sEdge, const float* eEdge, const float* sEdgeY, const float* eEdgeY, const float* p, const float* py, float* tOutput, int32_t* windingOutput, int32_t* windingShadowedOutput, float* testPointProjectedY)
        {
            aos::Vec4V sev = aos::V4LoadA(sEdge);
            aos::Vec4V eev = aos::V4LoadA(eEdge);
            aos::Vec4V pv = aos::V4LoadA(p);


            aos::VecI32V winding = veStatus10(sev, eev, pv);
            aos::Vec4V t = computeEdgeInterpolationParameter(sev, eev, pv, tOutput);
            aos::I4StoreA(winding, windingOutput);

            /**
            Work with Y part
            */
            sev = aos::V4LoadA(sEdgeY);
            eev = aos::V4LoadA(eEdgeY);
            pv = aos::V4LoadA(py);

            t = aos::V4Add(sev, aos::V4Mul(t, aos::V4Sub(eev, sev)));
            aos::V4StoreU(t, testPointProjectedY);
            winding = aos::VecI32V_And(winding, (aos::VecI32V)aos::V4IsGrtrOrEq(pv, t));
            aos::I4StoreA(winding, windingShadowedOutput);

        }


#define VE_BUF_SIZE 256

        struct alignas(16) V3al // aligned vec3
        {
            float x, y, z, w; // added w to omit warning

            V3al() {};
            V3al(const physx::PxVec3& in)
            {
                memcpy(this, &in, sizeof(float) * 3);
            }
            V3al(float x, float y, float z) : x(x), y(y), z(z)
            {
            }
        };


        NV_FORCE_INLINE int32_t edgesCrossCheck(int32_t shadowing01Start, int32_t shadowing01End, int32_t shadowing10Start, int32_t shadowing10End)
        {
            return shadowing01End - shadowing01Start + shadowing10End - shadowing10Start;
        }


        NV_FORCE_INLINE void computeInterpolatedPoint(V3al* output, V3al* APair, V3al* BPair, float d)
        {
            aos::Vec4V Bsmd = aos::V4LoadA((float*)BPair);
            aos::Vec4V Asmd = aos::V4LoadA((float*)APair);
            aos::Vec4V temp = aos::V4Sub(Bsmd, Asmd);
            temp = aos::V4Mul(temp, aos::V4Load(d));
            temp = aos::V4Sub(Bsmd, temp);
            aos::V4StoreA(temp, (float*)output);
        }

        NV_FORCE_INLINE void computeInterpolatedPoint(Vertex* output, const Vertex* APair, const Vertex* BPair, const float d)
        {
            aos::Vec4V dv = aos::V4Load(d);
            aos::Vec4V dt1 = aos::V4LoadU((float*)BPair);
            aos::Vec4V dt2 = aos::V4LoadU((float*)APair);

            auto temp = aos::V4Add(dt2, aos::V4Mul(dv, aos::V4Sub(dt1, dt2)));
            aos::V4StoreU(temp, (float*)output);

            dt1 = aos::V4LoadU(((float*)BPair) + 4);
            dt2 = aos::V4LoadU(((float*)APair) + 4);

            temp = aos::V4Add(dt2, aos::V4Mul(dv, aos::V4Sub(dt1, dt2)));
            aos::V4StoreU(temp, ((float*)output) + 4);
        }


        void BooleanToolV2::makeFacetFacetTests(BooleanToolOutputData* outputData, int32_t threadId, int32_t threadCount)
        {
            ////////////////////////////////////////////////////////////////////////////
            auto vrtA = mMeshA->getVertices();
            auto edgesA = mMeshA->getEdges();
            const Nv::Blast::Facet* maFacets = mMeshA->getFacetsBuffer();

            auto vrtB = mMeshB->getVertices();
            auto edgesB = mMeshB->getEdges();

            uint32_t facetsPerThread = (mMeshB->getFacetCount() + threadCount - 1) / threadCount;
            uint32_t facetBIndexBegin = threadId * facetsPerThread;
            uint32_t facetBIndexEnd = std::min((threadId + 1) * facetsPerThread, mMeshB->getFacetCount());

            for (uint32_t facetBIndex = facetBIndexBegin; facetBIndex < facetBIndexEnd; ++facetBIndex)
            {
                auto fcb = mMeshB->getFacet(facetBIndex);
                uint32_t edgeCountB = fcb->edgesCount;

                mAccelA->setState(vrtB, edgesB, *mMeshB->getFacet(facetBIndex));
                int32_t facetAIndex = mAccelA->getNextFacet();

#if USE_PREP_MESH
                auto& bplane = mMeshB->getFacet(facetBIndex)->plane;
                aos::Vec4V bplaneAOSx = aos::V4Load(bplane.n.x);
                aos::Vec4V bplaneAOSy = aos::V4Load(bplane.n.y);
                aos::Vec4V bplaneAOSz = aos::V4Load(bplane.n.z);
                aos::Vec4V bplaneAOSd = aos::V4Load(bplane.d);
                aos::Vec4V V4zero = aos::V4Zero();
#endif


                PxBounds3 facetBBounds = *toPxShared(mMeshB->getFacetBound(facetBIndex));
                facetBBounds.scaleFast(1.001f);

                while (facetAIndex != -1)
                {
                    int32_t newResultsCount = 0;
                    uint32_t CNT = 0;
                    auto fca = maFacets + facetAIndex;

                    if (facetBBounds.intersects(*toPxShared(mMeshA->getFacetBound(facetAIndex))) == false)
                    {
                        facetAIndex = mAccelA->getNextFacet();
                        continue;
                    }

                    uint32_t edgeCountA = fca->edgesCount;
#if USE_PREP_MESH   
                    auto& aplane = mMeshA->getFacet(facetAIndex)->plane;
                    aos::Vec4V aplaneAOS = aos::V4LoadU((float*)(&aplane));
                    /**
                        Compare edges of A against of points from B
                    */

                    {
                        bool hasAboveA = false;
                        bool hasBelowA = false;
                        for (uint32_t eda = 0; eda < edgeCountA; eda += 2)
                        {
                            uint32_t idx = mPreparedA->facetOffsets[facetAIndex] + eda * 2;
                            auto px = aos::V4LoadA(mPreparedA->px + idx);
                            auto py = aos::V4LoadA(mPreparedA->py + idx);
                            auto pz = aos::V4LoadA(mPreparedA->pz + idx);
                            auto d = aos::V4MulAdd(bplaneAOSz, pz, aos::V4MulAdd(bplaneAOSy, py, aos::V4MulAdd(bplaneAOSx, px, bplaneAOSd)));
                            if (aos::BAllEqFFFF(aos::V4IsGrtr(d, V4zero)) == 0)
                            {
                                hasAboveA = true;
                            }
                            if (aos::BAllEqFFFF(aos::V4IsGrtr(V4zero, d)) == 0)
                            {
                                hasBelowA = true;
                            }
                        }
                        if (!hasAboveA || !hasBelowA)
                        {
                            facetAIndex = mAccelA->getNextFacet();
                            continue;
                        }
                        for (uint32_t edb = 0; edb < edgeCountB; edb += 2)
                        {
                            uint32_t idx = mPreparedB->facetOffsets[facetBIndex] + edb * 2;
                            auto px = aos::V4LoadA(mPreparedB->px + idx);
                            auto py = aos::V4LoadA(mPreparedB->py + idx);
                            auto pz = aos::V4LoadA(mPreparedB->pz + idx);
                            auto d = aos::V4MulAdd(bplaneAOSz, pz, aos::V4MulAdd(bplaneAOSy, py, aos::V4MulAdd(bplaneAOSx, px, bplaneAOSd)));
                            if (aos::BAllEqFFFF(aos::V4IsGrtr(d, V4zero)) == 0)
                            {
                                hasAboveA = true;
                            }
                            if (aos::BAllEqFFFF(aos::V4IsGrtr(V4zero, d)) == 0)
                            {
                                hasBelowA = true;
                            }
                        }
                        if (!hasAboveA || !hasBelowA)
                        {
                            facetAIndex = mAccelA->getNextFacet();
                            continue;
                        }
                    }
#endif

                    CNT = 0;
                    for (uint32_t eda = 0; eda < edgeCountA; ++eda)
                    {
                        const Nv::Blast::Vertex& edgeAStart = vrtA[edgesA[fca->firstEdgeNumber + eda].s];
                        const Nv::Blast::Vertex& edgeAEnd = vrtA[edgesA[fca->firstEdgeNumber + eda].e];

                        for (uint32_t edb = 0; edb < edgeCountB; ++edb)
                        {
                            const Nv::Blast::Vertex& edgeBStart = vrtB[edgesB[fcb->firstEdgeNumber + edb].s];
                            const Nv::Blast::Vertex& edgeBEnd = vrtB[edgesB[fcb->firstEdgeNumber + edb].e];
                            {
                                sx1[CNT] = edgeAStart.p.x;
                                sy1[CNT] = edgeAStart.p.y;
                                ex1[CNT] = edgeAEnd.p.x;
                                ey1[CNT] = edgeAEnd.p.y;
                                px1[CNT] = edgeBStart.p.x;
                                py1[CNT] = edgeBStart.p.y;
                            }
                            {
                                sx2[CNT] = edgeBStart.p.x;
                                sy2[CNT] = edgeBStart.p.y;
                                ex2[CNT] = edgeBEnd.p.x;
                                ey2[CNT] = edgeBEnd.p.y;
                                px2[CNT] = edgeAStart.p.x;
                                py2[CNT] = edgeAStart.p.y;
                                CNT++;
                            }
                            {
                                sx1[CNT] = edgeAStart.p.x;
                                sy1[CNT] = edgeAStart.p.y;
                                ex1[CNT] = edgeAEnd.p.x;
                                ey1[CNT] = edgeAEnd.p.y;
                                px1[CNT] = edgeBEnd.p.x;
                                py1[CNT] = edgeBEnd.p.y;
                            }
                            {
                                sx2[CNT] = edgeBStart.p.x;
                                sy2[CNT] = edgeBStart.p.y;
                                ex2[CNT] = edgeBEnd.p.x;
                                ey2[CNT] = edgeBEnd.p.y;
                                px2[CNT] = edgeAEnd.p.x;
                                py2[CNT] = edgeAEnd.p.y;
                                CNT++;
                            }
                        }
                    }


                    /**
                        Do higher levels of compute here.
                    */

                    shadowingPretest10(CNT, sx1, ex1, sy1, ey1, px1, py1, pt1, winding1, projectedWinding1, resy1);
                    shadowingPretest01(CNT, sx2, ex2, sy2, ey2, px2, py2, pt2, winding2, projectedWinding2, resy2);
                    CNT = 0;

                    int32_t toEdgeCnt = 0;
                    uint32_t toEdge[2][2]; // First - A or B

                    for (uint32_t eda = 0; eda < edgeCountA; ++eda)
                    {
                        int32_t accumulatedEdgeFacetStatus = 0;

                        uint32_t vas = edgesA[fca->firstEdgeNumber + eda].s;
                        uint32_t vae = edgesA[fca->firstEdgeNumber + eda].e;
                        int32_t statusAvs = 0;
                        int32_t statusAve = 0;
                        float ymin1 = MAXIMUM_EXTENT;
                        float ymin2 = MAXIMUM_EXTENT;
                        float ymax1 = -MAXIMUM_EXTENT;
                        float ymax2 = -MAXIMUM_EXTENT;

                        int32_t bestEdgeAbove1 = -1;
                        int32_t bestEdgeBelow1 = -1;
                        int32_t bestEdgeAbove2 = -1;
                        int32_t bestEdgeBelow2 = -1;
                        int32_t bestCNTAbove1 = -1;
                        int32_t bestCNTBelow1 = -1;
                        int32_t bestCNTAbove2 = -1;
                        int32_t bestCNTBelow2 = -1;


                        bool EdgeFacethasA = false;
                        bool EdgeFacethasB = false;
                        Vertex EdgeFacetShadowingPairA[2];
                        Vertex EdgeFacetShadowingPairB[2];


                        for (uint32_t edb = 0; edb < edgeCountB; ++edb)
                        {
                            statusAvs -= projectedWinding2[CNT];
                            if (winding2[CNT] != 0)
                            {
                                if (resy2[CNT] < ymin1)
                                {
                                    bestCNTBelow1 = CNT;
                                    bestEdgeBelow1 = edb + fcb->firstEdgeNumber;
                                    ymin1 = resy2[CNT];
                                }
                                if (resy2[CNT] >= ymax1)
                                {
                                    bestCNTAbove1 = CNT;
                                    bestEdgeAbove1 = edb + fcb->firstEdgeNumber;
                                    ymax1 = resy2[CNT];
                                }
                            }

                            /**
                                Lets compute edge edge relations
                            */
                            edgeCrossCheckTest[CNT] = edgesCrossCheck(projectedWinding2[CNT], projectedWinding2[CNT + 1], projectedWinding1[CNT], projectedWinding1[CNT + 1]);

                            if (edgeCrossCheckTest[CNT] != 0) // this edges crossing
                            {
                                /**
                                    Now compute z-coordinate of intersection
                                */

                                Vertex AAbovePair[2];
                                Vertex BAbovePair[2];

                                bool hasB = false;
                                bool hasA = false;

                                uint32_t edgeBVerts[2] = { edgesB[edb + fcb->firstEdgeNumber].s, edgesB[edb + fcb->firstEdgeNumber].e };
                                uint32_t edgeAVerts[2] = { edgesA[eda + fca->firstEdgeNumber].s, edgesA[eda + fca->firstEdgeNumber].e };


                                for (int32_t pofs = 0; pofs < 2 && (!hasA || !hasB); ++pofs)
                                {
                                    uint32_t cpo = CNT + pofs;

                                    if (winding1[cpo]) // at least we have projected point already
                                    {
                                        if (projectedWinding1[cpo] == 0 && hasA == false)
                                        {
                                            AAbovePair[0] = vrtB[edgeBVerts[pofs]];
                                            computeInterpolatedPoint(&AAbovePair[1], &vrtA[edgeAVerts[0]], &vrtA[edgeAVerts[1]], pt1[cpo]);
                                            AAbovePair[1].p.x = AAbovePair[0].p.x;
                                            hasA = true;
                                        }
                                        if (projectedWinding1[cpo] != 0 && hasB == false)
                                        {
                                            BAbovePair[0] = vrtB[edgeBVerts[pofs]];
                                            computeInterpolatedPoint(&BAbovePair[1], &vrtA[edgeAVerts[0]], &vrtA[edgeAVerts[1]], pt1[cpo]);
                                            BAbovePair[1].p.x = BAbovePair[0].p.x;
                                            hasB = true;
                                        }
                                    }

                                    if (winding2[cpo]) // at least we have projected point already
                                    {
                                        if (projectedWinding2[cpo] == 0 && hasA == false)
                                        {
                                            AAbovePair[1] = vrtA[edgeAVerts[pofs]];
                                            computeInterpolatedPoint(&AAbovePair[0], &vrtB[edgeBVerts[0]], &vrtB[edgeBVerts[1]], pt2[cpo]);
                                            AAbovePair[0].p.x = AAbovePair[1].p.x;
                                            hasA = true;
                                        }
                                        if (projectedWinding2[cpo] != 0 && hasB == false)
                                        {
                                            BAbovePair[1] = vrtA[edgeAVerts[pofs]];
                                            computeInterpolatedPoint(&BAbovePair[0], &vrtB[edgeBVerts[0]], &vrtB[edgeBVerts[1]], pt2[cpo]);
                                            BAbovePair[0].p.x = BAbovePair[1].p.x;
                                            hasB = true;
                                        }
                                    }
                                }

                                if (hasA && hasB)
                                {
                                    float deltaPlus = BAbovePair[0].p.y - BAbovePair[1].p.y;
                                    float deltaMinus = AAbovePair[0].p.y - AAbovePair[1].p.y;
                                    float div = 0;
                                    if (deltaPlus > 0)
                                        div = deltaPlus / (deltaPlus - deltaMinus);
                                    else
                                        div = 0;

                                    computeInterpolatedPoint(edgeCrossA + CNT, BAbovePair + 1, AAbovePair + 1, div);
                                    computeInterpolatedPoint(edgeCrossB + CNT, BAbovePair, AAbovePair, div);
                                    edgeCrossB[CNT].p.x = edgeCrossA[CNT].p.x;
                                    edgeCrossB[CNT].p.y = edgeCrossA[CNT].p.y;


                                    int32_t tempTest = edgeCrossCheckTest[CNT];

                                    if (edgeCrossB[CNT].p.z < edgeCrossA[CNT].p.z)
                                    {
                                        tempTest = 0;

                                        EdgeFacethasA = true;
                                        EdgeFacetShadowingPairA[0] = edgeCrossB[CNT];
                                        EdgeFacetShadowingPairA[1] = edgeCrossA[CNT];
                                    }
                                    else
                                    {
                                        EdgeFacethasB = true;
                                        EdgeFacetShadowingPairB[0] = edgeCrossB[CNT];
                                        EdgeFacetShadowingPairB[1] = edgeCrossA[CNT];
                                    }

                                    accumulatedEdgeFacetStatus += tempTest;
                                }
                            }

                            CNT++;

                            statusAve -= projectedWinding2[CNT];

                            if (winding2[CNT] != 0)
                            {
                                if (resy2[CNT] < ymin2)
                                {
                                    bestCNTBelow2 = CNT;
                                    bestEdgeBelow2 = edb + fcb->firstEdgeNumber;
                                    ymin2 = resy2[CNT];
                                }
                                if (resy2[CNT] >= ymax2)
                                {
                                    bestCNTAbove2 = CNT;
                                    bestEdgeAbove2 = edb + fcb->firstEdgeNumber;;
                                    ymax2 = resy2[CNT];
                                }
                            }
                            CNT++;
                        }

                        if (statusAvs != 0)
                        {
                            auto& p = vrtA[vas];
                            float t = (p.p.y - ymin1) / (ymax1 - ymin1);
                            t = PxClamp(t, 0.0f, 1.0f);

                            Vertex v1, v2, np;
                            computeInterpolatedPoint(&v2, &vrtB[edgesB[bestEdgeAbove1].s], &vrtB[edgesB[bestEdgeAbove1].e], pt2[bestCNTAbove1]);
                            computeInterpolatedPoint(&v1, &vrtB[edgesB[bestEdgeBelow1].s], &vrtB[edgesB[bestEdgeBelow1].e], pt2[bestCNTBelow1]);
                            computeInterpolatedPoint(&np, &v1, &v2, t);
                            np.p.x = p.p.x;
                            np.p.y = p.p.y;

                            if (np.p.z < p.p.z)
                            {
                                statusAvs = 0;
                                // Set up shadowing
                                EdgeFacethasA = true;
                                EdgeFacetShadowingPairA[0] = np;
                                EdgeFacetShadowingPairA[1] = p;
                            }
                            else
                            {
                                EdgeFacethasB = true;
                                EdgeFacetShadowingPairB[0] = np;
                                EdgeFacetShadowingPairB[1] = p;
                            }
                        }

                        if (statusAve != 0)
                        {
                            auto& p = vrtA[vae];
                            float t = (p.p.y - ymin2) / (ymax2 - ymin2);
                            t = PxClamp(t, 0.0f, 1.0f);

                            Vertex v1, v2, np;
                            computeInterpolatedPoint(&v2, &vrtB[edgesB[bestEdgeAbove2].s], &vrtB[edgesB[bestEdgeAbove2].e], pt2[bestCNTAbove2]);
                            computeInterpolatedPoint(&v1, &vrtB[edgesB[bestEdgeBelow2].s], &vrtB[edgesB[bestEdgeBelow2].e], pt2[bestCNTBelow2]);
                            computeInterpolatedPoint(&np, &v1, &v2, t);

                            np.p.x = p.p.x;
                            np.p.y = p.p.y;

                            if (np.p.z < p.p.z)
                            {
                                statusAve = 0;
                                // Set up shadowing
                                EdgeFacethasA = true;
                                EdgeFacetShadowingPairA[0] = np;
                                EdgeFacetShadowingPairA[1] = p;
                            }
                            else
                            {
                                EdgeFacethasB = true;
                                EdgeFacetShadowingPairB[0] = np;
                                EdgeFacetShadowingPairB[1] = p;
                            }
                        }

                        /**
                            Now check
                        */
                        int32_t pStatus = statusAvs - statusAve - accumulatedEdgeFacetStatus;
                        if (pStatus != 0)
                        {
                            float deltaPlus = EdgeFacetShadowingPairB[0].p.z - EdgeFacetShadowingPairB[1].p.z;
                            float div = 0;
                            if (deltaPlus != 0)
                            {
                                float deltaMinus = EdgeFacetShadowingPairA[0].p.z - EdgeFacetShadowingPairA[1].p.z;
                                div = deltaPlus / (deltaPlus - deltaMinus);
                            }

                            FacetFacetResult& ffResult = outputData->getNewFfResult();
                            newResultsCount++;

                            Vertex vert;
                            computeInterpolatedPoint(&vert, &EdgeFacetShadowingPairB[1], &EdgeFacetShadowingPairA[1], div);
                            ffResult.pIdx = outputData->addVertex(vert);

                            int32_t inclusionValue = -inclusionValueEdgeFace(mToolMode, pStatus);

                            if (inclusionValue > 0)
                            {
                                computeInterpolatedPoint(&vert, &EdgeFacetShadowingPairB[0], &EdgeFacetShadowingPairA[0], div);
                                toEdge[0][1] = ffResult.pIdx;
                                toEdge[1][1] = outputData->addVertex(vert);
                            }
                            if (inclusionValue < 0)
                            {
                                computeInterpolatedPoint(&vert, &EdgeFacetShadowingPairB[0], &EdgeFacetShadowingPairA[0], div);
                                toEdge[0][0] = ffResult.pIdx;
                                toEdge[1][0] = outputData->addVertex(vert);
                            }

                            toEdgeCnt++;
                            ffResult.parentEdge = eda;
                            ffResult.parentFacet = facetAIndex;
                            ffResult.adjacentFacet = facetBIndex;
                            ffResult.status = pStatus;
                        }

                    }

                    CNT = 0;

                    for (uint32_t edb = 0; edb < edgeCountB; ++edb)
                    {
                        int32_t accumulatedEdgeFacetStatus = 0;
                        uint32_t vbs = edgesB[fcb->firstEdgeNumber + edb].s;
                        uint32_t vbe = edgesB[fcb->firstEdgeNumber + edb].e;
                        int32_t statusBvs = 0;
                        int32_t statusBve = 0;
                        float ymin1 = MAXIMUM_EXTENT;
                        float ymin2 = MAXIMUM_EXTENT;
                        float ymax1 = -MAXIMUM_EXTENT;
                        float ymax2 = -MAXIMUM_EXTENT;

                        int32_t bestEdgeAbove1 = -1;
                        int32_t bestEdgeBelow1 = -1;
                        int32_t bestEdgeAbove2 = -1;
                        int32_t bestEdgeBelow2 = -1;
                        int32_t bestCNTAbove1 = -1;
                        int32_t bestCNTBelow1 = -1;
                        int32_t bestCNTAbove2 = -1;
                        int32_t bestCNTBelow2 = -1;

                        bool EdgeFacethasA = false;
                        bool EdgeFacethasB = false;
                        Vertex EdgeFacetShadowingPairA[2];
                        Vertex EdgeFacetShadowingPairB[2];


                        for (uint32_t eda = 0; eda < edgeCountA; ++eda)
                        {
                            CNT = eda * edgeCountB * 2 + edb * 2;

                            statusBvs += projectedWinding1[CNT];
                            if (winding1[CNT] != 0)
                            {
                                if (resy1[CNT] < ymin1)
                                {
                                    bestCNTBelow1 = CNT;
                                    bestEdgeBelow1 = eda + fca->firstEdgeNumber;
                                    ymin1 = resy1[CNT];
                                }
                                if (resy1[CNT] >= ymax1)
                                {
                                    bestCNTAbove1 = CNT;
                                    bestEdgeAbove1 = eda + fca->firstEdgeNumber;
                                    ymax1 = resy1[CNT];
                                }
                            }

                            if (edgeCrossCheckTest[CNT])
                            {
                                int32_t tempTest = edgeCrossCheckTest[CNT];

                                if (edgeCrossB[CNT].p.z < edgeCrossA[CNT].p.z)
                                {
                                    tempTest = 0;

                                    EdgeFacethasA = true;
                                    EdgeFacetShadowingPairA[0] = edgeCrossB[CNT];
                                    EdgeFacetShadowingPairA[1] = edgeCrossA[CNT];
                                }
                                else
                                {
                                    EdgeFacethasB = true;
                                    EdgeFacetShadowingPairB[0] = edgeCrossB[CNT];
                                    EdgeFacetShadowingPairB[1] = edgeCrossA[CNT];
                                }
                                accumulatedEdgeFacetStatus += tempTest;
                            }

                            CNT++;
                            statusBve += projectedWinding1[CNT];

                            if (winding1[CNT] != 0)
                            {
                                if (resy1[CNT] < ymin2)
                                {
                                    bestCNTBelow2 = CNT;
                                    bestEdgeBelow2 = eda + fca->firstEdgeNumber;
                                    ymin2 = resy1[CNT];
                                }
                                if (resy1[CNT] >= ymax2)
                                {
                                    bestCNTAbove2 = CNT;
                                    bestEdgeAbove2 = eda + fca->firstEdgeNumber;
                                    ymax2 = resy1[CNT];
                                }
                            }
                        }

                        if (statusBvs != 0)
                        {
                            auto& p = vrtB[vbs];
                            float t = (p.p.y - ymin1) / (ymax1 - ymin1);
                            t = PxClamp(t, 0.0f, 1.0f);
                            Vertex p1, p2, np;
                            computeInterpolatedPoint(&p2, &vrtA[edgesA[bestEdgeAbove1].s], &vrtA[edgesA[bestEdgeAbove1].e], pt1[bestCNTAbove1]);
                            computeInterpolatedPoint(&p1, &vrtA[edgesA[bestEdgeBelow1].s], &vrtA[edgesA[bestEdgeBelow1].e], pt1[bestCNTBelow1]);
                            computeInterpolatedPoint(&np, &p1, &p2, t);

                            np.p.x = p.p.x;
                            np.p.y = p.p.y;

                            if (np.p.z >= p.p.z)
                            {
                                statusBvs = 0;
                                EdgeFacethasA = true;
                                EdgeFacetShadowingPairA[1] = np;
                                EdgeFacetShadowingPairA[0] = p;
                            }
                            else
                            {
                                EdgeFacethasB = true;
                                EdgeFacetShadowingPairB[1] = np;
                                EdgeFacetShadowingPairB[0] = p;
                            }
                        }

                        if (statusBve != 0)
                        {
                            auto& p = vrtB[vbe];
                            float t = (p.p.y - ymin2) / (ymax2 - ymin2);
                            t = PxClamp(t, 0.0f, 1.0f);
                            Vertex p1, p2, np;
                            computeInterpolatedPoint(&p2, &vrtA[edgesA[bestEdgeAbove2].s], &vrtA[edgesA[bestEdgeAbove2].e], pt1[bestCNTAbove2]);
                            computeInterpolatedPoint(&p1, &vrtA[edgesA[bestEdgeBelow2].s], &vrtA[edgesA[bestEdgeBelow2].e], pt1[bestCNTBelow2]);
                            computeInterpolatedPoint(&np, &p1, &p2, t);

                            np.p.x = p.p.x;
                            np.p.y = p.p.y;

                            if (np.p.z >= p.p.z)
                            {
                                statusBve = 0;
                                EdgeFacethasA = true;
                                EdgeFacetShadowingPairA[1] = np;
                                EdgeFacetShadowingPairA[0] = p;
                            }
                            else
                            {
                                EdgeFacethasB = true;
                                EdgeFacetShadowingPairB[1] = np;
                                EdgeFacetShadowingPairB[0] = p;
                            }
                        }

                        /**
                        Now check
                        */
                        int32_t pStatus2 = statusBve - statusBvs - accumulatedEdgeFacetStatus;
                        if (pStatus2 != 0)
                        {
                            float deltaPlus = EdgeFacetShadowingPairB[0].p.z - EdgeFacetShadowingPairB[1].p.z;
                            float div = 0;
                            if (deltaPlus != 0)
                            {
                                float deltaMinus = EdgeFacetShadowingPairA[0].p.z - EdgeFacetShadowingPairA[1].p.z;
                                div = deltaPlus / (deltaPlus - deltaMinus);
                            }

                            FacetFacetResult& ffResult = outputData->getNewFfResult();

                            newResultsCount++;

                            Vertex vert;
                            computeInterpolatedPoint(&vert, &EdgeFacetShadowingPairB[0], &EdgeFacetShadowingPairA[0], div);
                            ffResult.pIdx = outputData->addVertex(vert);


                            int32_t inclusionValue = inclusionValueEdgeFace(mToolMode, pStatus2);

                            if (inclusionValue > 0)
                            {
                                computeInterpolatedPoint(&vert, &EdgeFacetShadowingPairB[1], &EdgeFacetShadowingPairA[1], div);
                                toEdge[0][1] = outputData->addVertex(vert);
                                toEdge[1][1] = ffResult.pIdx;
                            }
                            if (inclusionValue < 0)
                            {
                                computeInterpolatedPoint(&vert, &EdgeFacetShadowingPairB[1], &EdgeFacetShadowingPairA[1], div);
                                toEdge[0][0] = outputData->addVertex(vert);
                                toEdge[1][0] = ffResult.pIdx;
                            }

                            toEdgeCnt++;
                            ffResult.parentEdge = edb;
                            ffResult.parentFacet = facetBIndex + mMeshA->getFacetCount();
                            ffResult.adjacentFacet = facetAIndex;
                            ffResult.status = pStatus2;
                        }
                    }

                    if (newResultsCount > 0)
                    {
                        BooleanResultEdge* e = &outputData->getNewEdge();
                        e->start = toEdge[0][0];
                        e->end = toEdge[0][1];
                        e->parentFacet = facetAIndex;
                        e->adjacentFacet = facetBIndex;
                        e = &outputData->getNewEdge();
                        e->start = toEdge[1][1];
                        e->end = toEdge[1][0];
                        e->parentFacet = facetBIndex + mMeshA->getFacetCount();
                        e->adjacentFacet = facetAIndex;
                    }
                    facetAIndex = mAccelA->getNextFacet();
                }
            }
        }


        BooleanToolV2::BooleanToolV2()
        {
            mToolMode = Mode::Intersection();

            sx1 = (float*)NVBLAST_ALLOC(sizeof(float) * VE_BUF_SIZE);
            sy1 = (float*)NVBLAST_ALLOC(sizeof(float) * VE_BUF_SIZE);
            ex1 = (float*)NVBLAST_ALLOC(sizeof(float) * VE_BUF_SIZE);
            ey1 = (float*)NVBLAST_ALLOC(sizeof(float) * VE_BUF_SIZE);
            px1 = (float*)NVBLAST_ALLOC(sizeof(float) * VE_BUF_SIZE);
            py1 = (float*)NVBLAST_ALLOC(sizeof(float) * VE_BUF_SIZE);
            pt1 = (float*)NVBLAST_ALLOC(sizeof(float) * VE_BUF_SIZE);
            resy1 = (float*)NVBLAST_ALLOC(sizeof(float) * VE_BUF_SIZE);
            winding1 = (int32_t*)NVBLAST_ALLOC(sizeof(int32_t) * VE_BUF_SIZE);
            projectedWinding1 = (int32_t*)NVBLAST_ALLOC(sizeof(int32_t) * VE_BUF_SIZE);

            sx2 = (float*)NVBLAST_ALLOC(sizeof(float) * VE_BUF_SIZE);
            sy2 = (float*)NVBLAST_ALLOC(sizeof(float) * VE_BUF_SIZE);
            ex2 = (float*)NVBLAST_ALLOC(sizeof(float) * VE_BUF_SIZE);
            ey2 = (float*)NVBLAST_ALLOC(sizeof(float) * VE_BUF_SIZE);
            px2 = (float*)NVBLAST_ALLOC(sizeof(float) * VE_BUF_SIZE);
            py2 = (float*)NVBLAST_ALLOC(sizeof(float) * VE_BUF_SIZE);
            pt2 = (float*)NVBLAST_ALLOC(sizeof(float) * VE_BUF_SIZE);
            resy2 = (float*)NVBLAST_ALLOC(sizeof(float) * VE_BUF_SIZE);
            winding2 = (int32_t*)NVBLAST_ALLOC(sizeof(int32_t) * VE_BUF_SIZE);
            projectedWinding2 = (int32_t*)NVBLAST_ALLOC(sizeof(int32_t) * VE_BUF_SIZE);



            edgeCrossCheckTest = (int32_t*)NVBLAST_ALLOC(sizeof(int32_t) * VE_BUF_SIZE);
            edgeCrossA = (Vertex*)NVBLAST_ALLOC(sizeof(Vertex) * VE_BUF_SIZE);
            edgeCrossB = (Vertex*)NVBLAST_ALLOC(sizeof(Vertex) * VE_BUF_SIZE);

        };

        void BooleanToolV2::release()
        {
            SAFE_FREE(sx1); SAFE_FREE(sx2);
            SAFE_FREE(sy1); SAFE_FREE(sy2);
            SAFE_FREE(ex1); SAFE_FREE(ex2);
            SAFE_FREE(ey1); SAFE_FREE(ey2);
            SAFE_FREE(px1); SAFE_FREE(px2);
            SAFE_FREE(py1); SAFE_FREE(py2);
            SAFE_FREE(pt1); SAFE_FREE(pt2);
            SAFE_FREE(resy1); SAFE_FREE(resy2);
            SAFE_FREE(winding1); SAFE_FREE(winding2);
            SAFE_FREE(projectedWinding1); SAFE_FREE(projectedWinding2);
            SAFE_FREE(edgeCrossCheckTest);
            SAFE_FREE(edgeCrossA);
            SAFE_FREE(edgeCrossB);

            NVBLAST_DELETE(this, BooleanToolV2);
        }

        int32_t BooleanToolV2::computeV03(const PxVec3& point)
        {
            int32_t status = 0;
            mAccelB->setState(fromPxShared(point));
            int32_t facet = mAccelB->getNextFacet();

            const Facet* facetsBuffer = mMeshB->getFacetsBuffer();
            const Edge* edgeBuffer = mMeshB->getEdges();
            const Vertex* vertexBuffer = mMeshB->getVertices();
            while (facet != -1)
            {
                Facet f = facetsBuffer[facet];
                uint32_t CNT = 0;

                const Edge* tempEdge = edgeBuffer + f.firstEdgeNumber;

                for (uint32_t ed = 0; ed < f.edgesCount; ++ed)
                {
                    sx1[CNT] = vertexBuffer[tempEdge->s].p.x;
                    sy1[CNT] = vertexBuffer[tempEdge->s].p.y;
                    px1[CNT] = point.x;
                    py1[CNT] = point.y;
                    ex1[CNT] = vertexBuffer[tempEdge->e].p.x;
                    ey1[CNT] = vertexBuffer[tempEdge->e].p.y;
                    CNT++;
                    tempEdge++;
                }
                tempEdge = edgeBuffer + f.firstEdgeNumber;
                shadowingPretest01(CNT, sx1, ex1, sy1, ey1, px1, py1, pt1, winding1, projectedWinding1, resy1);
                int32_t localStatus = 0;


                float ymin = MAXIMUM_EXTENT;
                float ymax = -MAXIMUM_EXTENT;
                int32_t yminidx = 0;
                int32_t ymaxidx = 0;


                for (uint32_t i = 0; i < CNT; ++i)
                {
                    if (winding1[i])
                    {
                        localStatus -= projectedWinding1[i];
                        if (ymin > resy1[i])
                        {
                            ymin = resy1[i];
                            yminidx = i;
                        }
                        if (ymax < resy1[i])
                        {
                            ymax = resy1[i];
                            ymaxidx = i;
                        }
                    }
                }


                float t = (point.y - ymin) / (ymax - ymin);
                t = PxClamp(t, 0.0f, 1.0f);

                float z1s = vertexBuffer[(tempEdge + yminidx)->s].p.z;
                float z1e = vertexBuffer[(tempEdge + yminidx)->e].p.z;
                z1s = (z1e - z1s) * pt1[yminidx] + z1s;



                float z2s = vertexBuffer[(tempEdge + ymaxidx)->s].p.z;
                float z2e = vertexBuffer[(tempEdge + ymaxidx)->e].p.z;
                z2s = (z2e - z2s) * pt1[ymaxidx] + z2s;

                float z = (z2s - z1s) * t + z1s;
                if (z < point.z)
                {
                    localStatus = 0;
                }

                status += localStatus;
                facet = mAccelB->getNextFacet();
            }

            return status;
        }

        int32_t BooleanToolV2::computeV30(const PxVec3& point)
        {
            int32_t status = 0;
            mAccelA->setState(fromPxShared(point));
            int32_t facet = mAccelA->getNextFacet();


            const Facet* facetsBuffer = mMeshA->getFacetsBuffer();
            const Edge* edgeBuffer = mMeshA->getEdges();
            const Vertex* vertexBuffer = mMeshA->getVertices();
            while (facet != -1)
            {
                Facet f = facetsBuffer[facet];
                uint32_t CNT = 0;

                const Edge* tempEdge = edgeBuffer + f.firstEdgeNumber;

                for (uint32_t ed = 0; ed < f.edgesCount; ++ed)
                {
                    sx1[CNT] = vertexBuffer[tempEdge->s].p.x;
                    sy1[CNT] = vertexBuffer[tempEdge->s].p.y;
                    px1[CNT] = point.x;
                    py1[CNT] = point.y;
                    ex1[CNT] = vertexBuffer[tempEdge->e].p.x;
                    ey1[CNT] = vertexBuffer[tempEdge->e].p.y;
                    CNT++;
                    tempEdge++;
                }
                tempEdge = edgeBuffer + f.firstEdgeNumber;
                shadowingPretest10(CNT, sx1, ex1, sy1, ey1, px1, py1, pt1, winding1, projectedWinding1, resy1);
                int32_t localStatus = 0;


                float ymin = MAXIMUM_EXTENT;
                float ymax = -MAXIMUM_EXTENT;
                int32_t yminidx = 0;
                int32_t ymaxidx = 0;


                for (uint32_t i = 0; i < CNT; ++i)
                {
                    if (winding1[i])
                    {
                        localStatus += projectedWinding1[i];
                        if (ymin > resy1[i])
                        {
                            ymin = resy1[i];
                            yminidx = i;
                        }
                        if (ymax < resy1[i])
                        {
                            ymax = resy1[i];
                            ymaxidx = i;
                        }
                    }
                }


                float t = (point.y - ymin) / (ymax - ymin);
                t = PxClamp(t, 0.0f, 1.0f);

                float z1s = vertexBuffer[(tempEdge + yminidx)->s].p.z;
                float z1e = vertexBuffer[(tempEdge + yminidx)->e].p.z;
                z1s = (z1e - z1s) * pt1[yminidx] + z1s;



                float z2s = vertexBuffer[(tempEdge + ymaxidx)->s].p.z;
                float z2e = vertexBuffer[(tempEdge + ymaxidx)->e].p.z;
                z2s = (z2e - z2s) * pt1[ymaxidx] + z2s;

                float z = (z2s - z1s) * t + z1s;
                if (z >= point.z)
                {
                    localStatus = 0;
                }

                status -= localStatus;
                facet = mAccelA->getNextFacet();
            }


            return status;
        }

        struct VertexComparatorRT
        {
            VertexComparatorRT(PxVec3 base = PxVec3()) : basePoint(base) {};
            PxVec3 basePoint;
            const Vertex* vertices;
            bool operator()(uint32_t a, uint32_t b)
            {
                return toPxShared(vertices[b].p - vertices[a].p).dot(basePoint) > 0.0;
            }
        };


        struct StrictVertexComparator
        {
            bool operator()(const NvcVec3& a, const NvcVec3& b) const
            {
                if (a.x < b.x) return true;
                if (a.x > b.x) return false;
                if (a.y < b.y) return true;
                if (a.y > b.y) return false;
                return a.z < b.z;
            }

        };

        struct FaceOrientation
        {
            const DamagePattern* pattern;
            uint32_t chunk;

            FaceOrientation(const DamagePattern* inPattern, uint32_t inChunk) : pattern(inPattern), chunk(inChunk) {}

            bool operator()(uint32_t f)
            {
#ifdef USE_MERGED_MESH
                return pattern->mergedFacetToChunkMap[f].second == chunk && pattern->mergedFacetToChunkMap[f].first != pattern->mergedFacetToChunkMap[f].second;
#else
                return false;
#endif
            }
        };

        void BooleanToolV2::computeRetained(const Mesh* mesh, const PxBounds3& bMeshBoudning, int32_t(BooleanToolV2::*computeV3)(const PxVec3&), int32_t btC, int32_t btCI, int32_t parentFacetOffset,
            BooleanToolOutputData* outputData, int32_t threadId, int32_t threadCount, FaceOrientation* fo, const std::vector<bool>* validAdjacentFacet)
        {
            const Vertex* vertices = mesh->getVertices();
            const Edge* facetEdges = mesh->getEdges();
            int32_t statusValue = 0;
            int32_t inclusionValue = 0;

            uint32_t retainedStarts[255];
            uint32_t retainedEnds[255];
            uint32_t rtsCount = 0;
            uint32_t rteCount = 0;
            PxVec3 compositeStart(0, 0, 0);
            PxVec3 compositeEnd(0, 0, 0);
            VertexComparatorRT comp;
            comp.vertices = outputData->vertices;

            std::map<NvcVec3, int32_t, StrictVertexComparator> vertexToValueTest;
            
            uint32_t facetsPerThread = (mesh->getFacetCount() + threadCount - 1) / threadCount;
            uint32_t facetIdBegin = threadId * facetsPerThread;
            uint32_t facetIdEnd = std::min((threadId + 1) * facetsPerThread, mesh->getFacetCount());

            const FacetFacetResult* fEnd = outputData->ffResult + outputData->ffResultCount();
            const FacetFacetResult* fResult = std::lower_bound((const FacetFacetResult*)outputData->ffResult, fEnd, parentFacetOffset + facetIdBegin, [](const FacetFacetResult& ffr, uint32_t val)
            {
                return ffr.parentFacet < val;
            });

            for (uint32_t facetId = facetIdBegin; facetId < facetIdEnd; ++facetId)
            {
                auto ted = facetEdges + mesh->getFacet(facetId)->firstEdgeNumber;
                for (uint32_t ed = 0; ed < mesh->getFacet(facetId)->edgesCount; ++ed)
                {
                    auto& vertS = vertices[ted->s];
                    auto& vertE = vertices[ted->e];
                    rtsCount = 0;
                    rteCount = 0;
                    if (bMeshBoudning.contains(toPxShared(vertS.p)))
                    {
                        auto it = vertexToValueTest.find(vertS.p);
                        if (it == vertexToValueTest.end())
                        {
                            statusValue = (this->*computeV3)(toPxShared(vertS.p));
                            vertexToValueTest[vertS.p] = statusValue;
                        }
                        else
                        {
                            statusValue = it->second;
                        }

                        inclusionValue = -(btC + btCI * statusValue);

                        if (inclusionValue > 0)
                        {
                            for (int32_t ic = 0; ic < inclusionValue; ++ic)
                            {
                                retainedEnds[rteCount++] = outputData->addVertex(vertS);
                                compositeEnd += toPxShared(vertS.p);
                            }
                        }
                        else
                        {
                            if (inclusionValue < 0)
                            {
                                for (int32_t ic = 0; ic < -inclusionValue; ++ic)
                                {
                                    retainedStarts[rtsCount++] = outputData->addVertex(vertS);
                                    compositeStart += toPxShared(vertS.p);
                                }
                            }
                        }
                    }
                    if (bMeshBoudning.contains(toPxShared(vertE.p)))
                    {
                        auto it = vertexToValueTest.find(vertE.p);
                        if (it == vertexToValueTest.end())
                        {
                            statusValue = (this->*computeV3)(toPxShared(vertE.p));
                            vertexToValueTest[vertE.p] = statusValue;
                        }

                        else
                        {
                            statusValue = it->second;
                        }

                        inclusionValue = btC + btCI * statusValue;

                        if (inclusionValue > 0)
                        {
                            for (int32_t ic = 0; ic < inclusionValue; ++ic)
                            {
                                retainedEnds[rteCount++] = outputData->addVertex(vertE);
                                compositeEnd += toPxShared(vertE.p);
                            }
                        }
                        else
                        {
                            if (inclusionValue < 0)
                            {
                                for (int32_t ic = 0; ic < -inclusionValue; ++ic)
                                {
                                    retainedStarts[rtsCount++] = outputData->addVertex(vertE);
                                    compositeStart += toPxShared(vertE.p);
                                }
                            }
                        }
                    }
                    while (fResult->parentFacet == facetId + parentFacetOffset && fResult->parentEdge == ed)
                    {
                        uint32_t adjFacet = fResult->adjacentFacet;
                        if (validAdjacentFacet != nullptr && !(*validAdjacentFacet)[adjFacet >= validAdjacentFacet->size() ? adjFacet - validAdjacentFacet->size() : adjFacet])
                        {
                            fResult++;
                            continue;
                        }

                        inclusionValue = inclusionValueEdgeFace(mToolMode, fResult->status);
                        if (fo != nullptr && (*fo)(adjFacet))
                        {
                            inclusionValue = -inclusionValue;
                        }

                        if (inclusionValue > 0)
                        {
                            for (int32_t ic = 0; ic < inclusionValue; ++ic)
                            {
                                retainedEnds[rteCount++] = fResult->pIdx;
                                compositeEnd += toPxShared(outputData->vertices[fResult->pIdx].p);
                            }
                        }
                        else
                        {
                            if (inclusionValue < 0)
                            {
                                for (int32_t ic = 0; ic < -inclusionValue; ++ic)
                                {
                                    retainedStarts[rtsCount++] = fResult->pIdx;
                                    compositeStart += toPxShared(outputData->vertices[fResult->pIdx].p);
                                }
                            }
                        }

                        fResult++;
                    }
                    if (rtsCount != rteCount)
                    {
                        return;
                    }

                    if (rtsCount > 1)
                    {
                        comp.basePoint = compositeEnd - compositeStart;
                        std::sort(retainedStarts, retainedStarts + rtsCount, comp);
                        std::sort(retainedEnds, retainedEnds + rteCount, comp);
                    }
                    for (uint32_t rv = 0; rv < rtsCount; ++rv)
                    {
                        BooleanResultEdge& e = outputData->getNewEdge();
                        e.start = retainedStarts[rv];
                        e.end = retainedEnds[rv];
                        e.parentFacet = facetId + parentFacetOffset;
                    }
                    ted++;
                }
            }
        }

        void BooleanToolV2::retain(bool isA, BooleanToolOutputData* outputData, int32_t threadId, int32_t threadCount, const DamagePattern* pattern, int32_t chunk)
        {
            if (isA)
            {
#ifdef USE_MERGED_MESH
                if (pattern != nullptr && chunk >= 0)
                {
                    FaceOrientation fo(pattern, chunk);
                    computeRetained(mMeshA, mMeshB->getBoundingBox(), &BooleanToolV2::computeV03, mToolMode.ca, mToolMode.ci, 0, outputData, threadId, threadCount, &fo, &(pattern->validFacetsForChunk[chunk]));
                }
#endif
                computeRetained(mMeshA, toPxShared(mMeshB->getBoundingBox()), &BooleanToolV2::computeV03, mToolMode.ca, mToolMode.ci, 0, outputData, threadId, threadCount);
            }
            else
            {
                computeRetained(mMeshB, toPxShared(mMeshA->getBoundingBox()), &BooleanToolV2::computeV30, mToolMode.cb, mToolMode.ci, mMeshA->getFacetCount(), outputData, threadId, threadCount);
            }
        }

        //Mesh* BooleanToolV2::getMesh()
        //{
        //  std::vector<Edge> edges;
        //  std::vector<Facet> fct;

        //  std::sort(outputEdges, outputEdges + outputEdgeCount, [](const BooleanResultEdge& e1, const BooleanResultEdge& e2)
        //  {
        //      return e1.parentFacet < e2.parentFacet;
        //  });

        //  int32_t lastStart = 0;

        //  for (uint32_t i = 0; i < outputEdgeCount; ++i)
        //  {
        //      edges.push_back(Edge(outputEdges[i].start, outputEdges[i].end));
        //      if (i + 1 >= outputEdgeCount || outputEdges[i].parentFacet != outputEdges[i + 1].parentFacet)
        //      {
        //          fct.push_back(Facet());
        //          fct.back().firstEdgeNumber = lastStart;
        //          fct.back().edgesCount = edges.size() - lastStart;
        //          lastStart = edges.size();
        //      }
        //  }
        //  return new MeshImpl(outputVertices, edges.data(), fct.data(), outputVerticesCount, edges.size(), fct.size());
        //}

        TriangulatorV2::TriangulatorV2() : wldg(BLASTRT_MAX_VERTICES, 512, 0.001, 1e-5f, 1e-3f, &VertexWelding::LocateVertexInBucket)
        {
            memset(visitedFlagValue, 0, sizeof(uint32_t) * 1024);
            currentFlagValue = 1;
            weldedEdges = (Edge*)NVBLAST_ALLOC(sizeof(Edge) * BLASTRT_MAX_EDGES_PER_CHUNK);
            triangleCount = 0;
        }

        void TriangulatorV2::release()
        {
            SAFE_FREE(weldedEdges);
            NVBLAST_DELETE(this, TriangulatorV2);
        }

        NV_FORCE_INLINE bool compareTwoFloats(float a, float b)
        {
            return std::abs(b - a) <= FLT_EPSILON * std::abs(b + a);
        }
        NV_FORCE_INLINE bool compareTwoVertices(const PxVec3& a, const PxVec3& b)
        {
            return compareTwoFloats(a.x, b.x) && compareTwoFloats(a.y, b.y) && compareTwoFloats(a.z, b.z);
        }
        NV_FORCE_INLINE bool compareTwoVertices(const PxVec2& a, const PxVec2& b)
        {
            return compareTwoFloats(a.x, b.x) && compareTwoFloats(a.y, b.y);
        }

        NV_FORCE_INLINE float getRotation(const PxVec2& a, const PxVec2& b)
        {
            return a.x * b.y - a.y * b.x;
        }

        NV_FORCE_INLINE bool pointInside(PxVec2 a, PxVec2 b, PxVec2 c, PxVec2 pnt)
        {
            if (compareTwoVertices(a, pnt) || compareTwoVertices(b, pnt) || compareTwoVertices(c, pnt))
            {
                return false;
            }
            float v1 = (getRotation((b - a), (pnt - a)));
            float v2 = (getRotation((c - b), (pnt - b)));
            float v3 = (getRotation((a - c), (pnt - c)));

            return (v1 >= 0.0f && v2 >= 0.0f && v3 >= 0.0f) ||
                (v1 <= 0.0f && v2 <= 0.0f && v3 <= 0.0f);

        }

#define MIN_ROTATION_THRESHOLD 0.0001f

        void TriangulatorV2::triangulatePolygonWithEarClipping(ProjectionDirections dir)
        {
            if (pointCount < 3)
            {
                return;
            }
            /**
                Build projected points
            */
            for (uint32_t i = 0; i < facetListSize; ++i)
            {
                projectedPointList[i] = getProjectedPoint(weldedVertices[facetList[i].point].p, dir);
            }
            
            /**
                Walk around circle and find triangle with minimal angle
            */
            bool goodWalk = true; // While we found something to build triangle from
            while (goodWalk && pointCount > 2)
            {
                goodWalk = false;
                for (uint32_t j = 0; j < pointCount; ++j)
                {
                    uint32_t cPoint = pointIndicesList[j];
                    if (facetList[facetList[cPoint].nextPoint].point == kNotValidVertexIndex || facetList[facetList[cPoint].prevPoint].point == kNotValidVertexIndex)
                    {
                        continue;
                    }

                    PxVec2& cVp = projectedPointList[cPoint];
                    PxVec2& nVp = projectedPointList[facetList[cPoint].nextPoint];
                    PxVec2& pVp = projectedPointList[facetList[cPoint].prevPoint];
                    // Check wheather curr is ear-tip
                    float rot = getRotation((pVp - nVp).getNormalized(), (cVp - nVp).getNormalized());

                    if (!(dir & OPPOSITE_WINDING)) rot = -rot;
                    if (rot > MIN_ROTATION_THRESHOLD)
                    {
                        bool dontHaveInside = true;
                        for (uint32_t s = 0; s < pointCount; ++s)
                        {
                            if (pointInside(cVp, nVp, pVp, projectedPointList[pointIndicesList[s]]))
                            {
                                dontHaveInside = false;
                                break;
                            }
                        }
                        if (dontHaveInside && triangleCount < maxTriangleCount)
                        {
                            uint32_t* vrt = triangleIndices + triangleCount * 3;
                                                        
                            if (facetList[cPoint].point < weldedCount && facetList[facetList[cPoint].nextPoint].point < weldedCount && facetList[facetList[cPoint].prevPoint].point < weldedCount)
                            {
                                *vrt = facetList[cPoint].point;
                                vrt++;
                                *vrt = facetList[facetList[cPoint].nextPoint].point;
                                vrt++;
                                *vrt = facetList[facetList[cPoint].prevPoint].point;
                                triangleCount++;
                            }
                            /**
                                Reattach vertices in list
                            */
                            facetList[facetList[cPoint].prevPoint].nextPoint = facetList[cPoint].nextPoint;
                            facetList[facetList[cPoint].nextPoint].prevPoint = facetList[cPoint].prevPoint;

                            {
                                /* Remove point */
                                facetList[cPoint].prevPoint = kNotValidVertexIndex;
                                facetList[cPoint].nextPoint = kNotValidVertexIndex;
                                facetList[cPoint].point = kNotValidVertexIndex;

                                std::swap(pointIndicesList[j], pointIndicesList[pointCount - 1]);
                                pointCount--;                               
                            }
                            goodWalk = true;
                            break;
                        }
                    }
                }
            }
        }
                
        uint32_t TriangulatorV2::build(const BooleanResultEdge* edges, uint32_t inEdgeCount, const Vertex* inVertices, Nv::Blast::Vertex* outWeldedVrts, uint32_t& vcount, uint32_t* outTriangles, PerTriangleAdditionalData* adata, uint32_t maxTcount, const Mesh* ma, const Mesh* mb)
        {

            meshA = ma;
            meshB = mb;

            int32_t meshAFacetCount = ma->getFacetCount();
            int32_t meshBFacetCount = mb->getFacetCount();

            weldedCount = 0;
            facetListSize = 0;
            projectedPointCount = 0;
            triangleCount = 0;
            mInpEdges = edges;
            mInpEdgeCount = inEdgeCount;
            maxTriangleCount = maxTcount;

            weldedVertices = outWeldedVrts;
            triangleIndices = outTriangles;


            wldg.reset();

            for (uint32_t i = 0; i < inEdgeCount; i++)
            {
                weldedEdges[i].s = wldg.WeldVertex(&inVertices[edges[i].start]);
                weldedEdges[i].e = wldg.WeldVertex(&inVertices[edges[i].end]);
            }
            vcount = weldedCount = wldg.getVerticesCount();
            memcpy(weldedVertices, wldg.getVertices(), sizeof(Vertex) * weldedCount);


            int32_t lastParent = edges[0].parentFacet;
            int32_t facetStart = 0;

            for (uint32_t ed = 0; ed < inEdgeCount; ++ed)
            {
                if ((ed + 1 < inEdgeCount && mInpEdges[ed + 1].parentFacet != lastParent) || ed + 1 == inEdgeCount)
                {
                    /**
                        Fill linked list to
                    */
                    facetListSize = 0;
                    pointCount = 0;
                    for (uint32_t f = facetStart; f < ed + 1; ++f)
                    {
                        auto& ie = weldedEdges[f];
                        if (ie.e == ie.s) continue;
                        uint32_t start = kNotValidVertexIndex;
                        uint32_t end = kNotValidVertexIndex;
                        for (uint32_t inlink = 0; inlink < facetListSize; ++inlink)
                        {
                            if (facetList[inlink].point == ie.s) start = inlink;
                            if (facetList[inlink].point == ie.e) end = inlink;
                        }
                        if (start == kNotValidVertexIndex)
                        {
                            facetList[facetListSize].point = ie.s;
                            start = facetListSize++;
                            pointIndicesList[pointCount++] = start;
                        }
                        if (end == kNotValidVertexIndex)
                        {
                            facetList[facetListSize].point = ie.e;
                            end = facetListSize++;
                            pointIndicesList[pointCount++] = end;
                        }
                        facetList[start].nextPoint = end;
                        facetList[end].prevPoint = start;
                    }
                    if (pointCount > 2)
                    {
                        PxVec3 normal(0, 0, 0);
                        bool badLoop = false;
                        for (uint32_t i = 0; i < facetListSize; ++i)
                        {
                            if (facetList[i].prevPoint >= weldedCount || facetList[i].nextPoint >= weldedCount || facetList[i].point >= weldedCount)
                            {
                                badLoop = true;
                                break;
                            }
                            PxVec3 base = toPxShared(weldedVertices[facetList[facetList[i].prevPoint].point].p);
                            PxVec3 p1 = toPxShared(weldedVertices[facetList[i].point].p);
                            PxVec3 p2 = toPxShared(weldedVertices[facetList[facetList[i].nextPoint].point].p);
                            normal += (p1 - base).cross(p2 - base);
                        }

                        if (!badLoop)
                        {
                            uint32_t oldCount = triangleCount;
                            triangulatePolygonWithEarClipping(getProjectionDirection(-normal));

                            for (uint32_t tid = oldCount; tid < triangleCount; ++tid)
                            {
                                if (lastParent < meshAFacetCount)
                                {
                                    adata[tid].materialIndex = meshA->getFacet(lastParent)->materialId;
                                    adata[tid].smoothingGroup = meshA->getFacet(lastParent)->smoothingGroup;
                                }
                                else if (lastParent - meshAFacetCount < meshBFacetCount)
                                {
                                    adata[tid].materialIndex = meshB->getFacet(lastParent - meshAFacetCount)->materialId;
                                    adata[tid].smoothingGroup = meshB->getFacet(lastParent - meshAFacetCount)->smoothingGroup;
                                }
                            }
                        }
                        else
                        {
                            for (uint32_t i = 0; i < facetListSize; ++i)
                            {
                                facetList[i].prevPoint = kNotValidVertexIndex;
                                facetList[i].nextPoint = kNotValidVertexIndex;
                            }
                        }
                    }
                    facetStart = ed + 1;
                    lastParent = mInpEdges[ed + 1].parentFacet;
                }
            }
            return triangleCount;
        };


    }
}