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
// Copyright (c) 2017 NVIDIA Corporation. All rights reserved.


// This warning arises when using some stl containers with older versions of VC
// c:\program files (x86)\microsoft visual studio 12.0\vc\include\xtree(1826): warning C4702: unreachable code
#include "NvPreprocessor.h"
#if NV_VC && NV_VC < 14
#pragma warning(disable : 4702)
#endif

#include <NvBlastExtAuthoringBondGeneratorImpl.h>
#include <NvBlast.h>
#include "NvBlastExtTriangleProcessor.h"
#include "NvBlastExtApexSharedParts.h"
#include "NvBlastExtAuthoringCollisionBuilderImpl.h"
#include "NvBlastExtAuthoringInternalCommon.h"
#include "NvBlastExtAuthoringTypes.h"
#include <vector>
#include <map>
#include <PxPlane.h>
#include <algorithm>
#include <cmath>
#include <memory>

using physx::PxVec3;
using physx::PxBounds3;

#define SAFE_ARRAY_NEW(T, x) ((x) > 0) ? new T[x] : nullptr;

//#define DEBUG_OUTPUT
#ifdef DEBUG_OUTPUT

void saveGeometryToObj(std::vector<PxVec3>& triangles, const char* filepath)
{
	
	FILE* outStream = fopen(filepath, "w");

	for (uint32_t i = 0; i < triangles.size(); ++i)
	{
		fprintf(outStream, "v %lf %lf %lf\n", triangles[i].x, triangles[i].y, triangles[i].z);
		++i;
		fprintf(outStream, "v %lf %lf %lf\n", triangles[i].x, triangles[i].y, triangles[i].z);
		++i;
		fprintf(outStream, "v %lf %lf %lf\n", triangles[i].x, triangles[i].y, triangles[i].z);
	}
	for (uint32_t i = 0; i < triangles.size() / 3; ++i)
	{
		PxVec3 normal = (triangles[3 * i + 2] - triangles[3 * i]).cross((triangles[3 * i + 1] - triangles[3 * i])).getNormalized();
		fprintf(outStream, "vn %lf %lf %lf\n", normal.x, normal.y, normal.z);
		fprintf(outStream, "vn %lf %lf %lf\n", normal.x, normal.y, normal.z);
		fprintf(outStream, "vn %lf %lf %lf\n", normal.x, normal.y, normal.z);
	}
	int indx = 1;
	for (uint32_t i = 0; i < triangles.size() / 3; ++i)
	{
		fprintf(outStream, "f %d//%d  ", indx, indx);
		indx++;
		fprintf(outStream, "%d//%d  ", indx, indx);
		indx++;
		fprintf(outStream, "%d//%d \n", indx, indx);
		indx++;
	}

	fclose(outStream);

}


std::vector<PxVec3> intersectionBuffer;
std::vector<PxVec3> meshBuffer;
#endif 

namespace Nv
{
	namespace Blast
	{

		#define EPS_PLANE 0.0001f
				
		bool planeComparer(const PlaneChunkIndexer& as, const PlaneChunkIndexer& bs)
			{
				const PxPlane& a = as.plane;
				const PxPlane& b = bs.plane;

				if (a.d + EPS_PLANE < b.d)		return true;
				if (a.d - EPS_PLANE > b.d)		return false;
				if (a.n.x + EPS_PLANE < b.n.x) return true;
				if (a.n.x - EPS_PLANE > b.n.x) return false;
				if (a.n.y + EPS_PLANE < b.n.y) return true;
				if (a.n.y - EPS_PLANE > b.n.y) return false;
				return a.n.z + EPS_PLANE < b.n.z;
			}


		struct Bond
		{
			int32_t m_chunkId;
			int32_t m_planeIndex;
			int32_t triangleIndex;

			bool operator<(const Bond& inp) const
			{
				if (abs(m_planeIndex) == abs(inp.m_planeIndex))
				{
					return m_chunkId < inp.m_chunkId;
				}
				else
				{
					return abs(m_planeIndex) < abs(inp.m_planeIndex);
				}
			}
		};


		struct BondInfo
		{
			float area;
			physx::PxBounds3 m_bb;
			physx::PxVec3 centroid;
			physx::PxVec3 normal;
			int32_t m_chunkId;
		};

		float BlastBondGeneratorImpl::processWithMidplanes(TriangleProcessor* trProcessor, const std::vector<PxVec3>& chunk1Points, const std::vector<PxVec3>& chunk2Points,
			const std::vector<PxVec3>& hull1p, const std::vector<PxVec3>& hull2p, PxVec3& normal, PxVec3& centroid)
		{
			PxBounds3 bounds;
			PxBounds3 aBounds;
			PxBounds3 bBounds;
			bounds.setEmpty();
			aBounds.setEmpty();
			bBounds.setEmpty();

			PxVec3 chunk1Centroid(0, 0, 0);
			PxVec3 chunk2Centroid(0, 0, 0);

			///////////////////////////////////////////////////////////////////////////////////
			if (chunk1Points.size() < 4 || chunk2Points.size() < 4)
			{
				return 0.0;
			}

			for (uint32_t i = 0; i < chunk1Points.size(); ++i)
			{
				chunk1Centroid += chunk1Points[i];
				bounds.include(chunk1Points[i]);
				aBounds.include(chunk1Points[i]);
			}
			for (uint32_t i = 0; i < chunk2Points.size(); ++i)
			{
				chunk2Centroid += chunk2Points[i];
				bounds.include(chunk2Points[i]);
				bBounds.include(chunk2Points[i]);
			}


			chunk1Centroid *= (1.0f / chunk1Points.size());
			chunk2Centroid *= (1.0f / chunk2Points.size());

			Separation separation;
			if (!importerHullsInProximityApexFree(hull1p.size(), hull1p.data(), aBounds, PxTransform(PxIdentity), PxVec3(1, 1, 1), hull2p.size(), hull2p.data(), bBounds, PxTransform(PxIdentity), PxVec3(1, 1, 1), 0.000, &separation))
			{
				return 0.0;
			}

			// Build first plane interface
			PxPlane midplane = separation.plane;
			if (!midplane.n.isFinite())
			{
				return 0.0;
			}
			std::vector<PxVec3> interfacePoints;

			float firstCentroidSide = midplane.distance(chunk1Centroid);
			float secondCentroidSide = midplane.distance(chunk2Centroid);

			for (uint32_t i = 0; i < chunk1Points.size(); ++i)
			{
				float dst = midplane.distance(chunk1Points[i]);
				if (dst * firstCentroidSide < 0)
				{
					interfacePoints.push_back(chunk1Points[i]);
				}
			}

			for (uint32_t i = 0; i < chunk2Points.size(); ++i)
			{
				float dst = midplane.distance(chunk2Points[i]);
				if (dst * secondCentroidSide < 0)
				{
					interfacePoints.push_back(chunk2Points[i]);
				}
			}
			std::vector<PxVec3> convexHull;
			trProcessor->buildConvexHull(interfacePoints, convexHull, midplane.n);
			float area = 0;
			PxVec3 centroidLocal(0, 0, 0);
			if (convexHull.size() < 3)
			{
				return 0.0;
			}
			for (uint32_t i = 0; i < convexHull.size() - 1; ++i)
			{
				centroidLocal += convexHull[i];
				area += (convexHull[i] - convexHull[0]).cross((convexHull[i + 1] - convexHull[0])).magnitude();
			}
			centroidLocal += convexHull.back();
			centroidLocal *= (1.0f / convexHull.size());
			float direction = midplane.n.dot(chunk2Centroid - chunk1Centroid);
			if (direction < 0)
			{
				normal = -1.0f * normal;
			}
			normal = midplane.n;
			centroid = centroidLocal;
			return area * 0.5f;
		}

		int32_t BlastBondGeneratorImpl::createFullBondListAveraged(uint32_t meshCount, const uint32_t* geometryOffset, const Triangle* geometry, 
			const bool* supportFlags, NvBlastBondDesc*& resultBondDescs, BondGenerationConfig conf)
		{
			NV_UNUSED(conf);

			std::vector<std::vector<PxVec3> > chunksPoints(meshCount);

			for (uint32_t i = 0; i < meshCount; ++i)
			{
				if (!supportFlags[i])
				{
					continue;
				}
				uint32_t count = geometryOffset[i + 1] - geometryOffset[i];
				for (uint32_t j = 0; j < count; ++j)
				{
					chunksPoints[i].push_back(geometry[geometryOffset[i] + j].a.p);
					chunksPoints[i].push_back(geometry[geometryOffset[i] + j].b.p);
					chunksPoints[i].push_back(geometry[geometryOffset[i] + j].c.p);
				}
			}

			Nv::Blast::ConvexMeshBuilderImpl builder(mPxCooking, mPxInsertionCallback);

			std::vector<CollisionHull*> cHulls(meshCount);

			for (uint32_t i = 0; i < meshCount; ++i)
			{
				if (!supportFlags[i])
				{
					continue;
				}
				cHulls[i] = builder.buildCollisionGeometry(chunksPoints[i].size(), chunksPoints[i].data());
			}

			std::vector<std::vector<PxVec3> > hullPoints(cHulls.size());

			for (uint32_t chunk = 0; chunk < cHulls.size(); ++chunk)
			{
				if (!supportFlags[chunk])
				{
					continue;
				}

				hullPoints[chunk].resize(cHulls[chunk]->pointsCount);
				for (uint32_t i = 0; i < cHulls[chunk]->pointsCount; ++i)
				{
					hullPoints[chunk][i] = cHulls[chunk]->points[i];
				}
				cHulls[chunk]->release();
			}

			TriangleProcessor trProcessor;
			std::vector<NvBlastBondDesc> mResultBondDescs;
			for (uint32_t i = 0; i < meshCount; ++i)
			{
				if (!supportFlags[i])
				{
					continue;
				}
				for (uint32_t j = i + 1; j < meshCount; ++j)
				{
					if (!supportFlags[i])
					{
						continue;
					}
					PxVec3 normal;
					PxVec3 centroid;

					float area = processWithMidplanes(&trProcessor, chunksPoints[i], chunksPoints[j], hullPoints[i], hullPoints[j], normal, centroid);

					if (area > 0)
					{
						NvBlastBondDesc bDesc;
						bDesc.chunkIndices[0] = i;
						bDesc.chunkIndices[1] = j;
						bDesc.bond.area = area;
						bDesc.bond.centroid[0] = centroid.x;
						bDesc.bond.centroid[1] = centroid.y;
						bDesc.bond.centroid[2] = centroid.z;

						bDesc.bond.normal[0] = normal.x;
						bDesc.bond.normal[1] = normal.y;
						bDesc.bond.normal[2] = normal.z;


						mResultBondDescs.push_back(bDesc);
					}

				}
			}
			resultBondDescs = SAFE_ARRAY_NEW(NvBlastBondDesc, mResultBondDescs.size());
			memcpy(resultBondDescs, mResultBondDescs.data(), sizeof(NvBlastBondDesc)*mResultBondDescs.size());
			return mResultBondDescs.size();
		}

		uint32_t isSamePlane(PxPlane& a, PxPlane& b)
		{
			if (PxAbs(a.d - b.d) > EPS_PLANE) return 0;
			if (PxAbs(a.n.x - b.n.x) > EPS_PLANE) return 0;
			if (PxAbs(a.n.y - b.n.y) > EPS_PLANE) return 0;
			if (PxAbs(a.n.z - b.n.z) > EPS_PLANE) return 0;
			return 1;
		}

		int32_t BlastBondGeneratorImpl::createFullBondListExact(uint32_t meshCount, const uint32_t* geometryOffset, const Triangle* geometry, 
			const bool* supportFlags, NvBlastBondDesc*& resultBondDescs, BondGenerationConfig conf)
		{		
			std::vector < PlaneChunkIndexer > planeTriangleMapping;
			NV_UNUSED(conf);
			for (uint32_t i = 0; i < meshCount; ++i)
			{
				if (!supportFlags[i])
				{
					continue;
				}
				uint32_t count = geometryOffset[i + 1] - geometryOffset[i];
				for (uint32_t j = 0; j < count; ++j)
				{
#ifdef DEBUG_OUTPUT
					meshBuffer.push_back(geometry[geometryOffset[i] + j].a.p );
					meshBuffer.push_back(geometry[geometryOffset[i] + j].b.p);
					meshBuffer.push_back(geometry[geometryOffset[i] + j].c.p );
#endif

					PxPlane nPlane = PxPlane(geometry[geometryOffset[i] + j].a.p, geometry[geometryOffset[i] + j].b.p, geometry[geometryOffset[i] + j].c.p);
					planeTriangleMapping.push_back(PlaneChunkIndexer(i, j, nPlane));
				}
			}

			std::sort(planeTriangleMapping.begin(), planeTriangleMapping.end(), planeComparer);
			return createFullBondListExactInternal(meshCount, geometryOffset, geometry, planeTriangleMapping, resultBondDescs);
		}

		void BlastBondGeneratorImpl::buildGeometryCache(uint32_t meshCount, const uint32_t* geometryOffset, const Triangle* geometry)
		{
			uint32_t geometryCount = geometryOffset[meshCount];
			for (uint32_t i = 0; i < meshCount; i++)
			{
				mGeometryCache.push_back(std::vector<Triangle>());
				uint32_t count = geometryOffset[i + 1] - geometryOffset[i];
				mGeometryCache.back().resize(count);
				memcpy(mGeometryCache.back().data(), geometry + geometryOffset[i], sizeof(Triangle) * count);
			}
			mHullsPointsCache.resize(geometryCount);
			mBoundsCache.resize(geometryCount);
			mCHullCache.resize(geometryCount);
			for (uint32_t i = 0; i < mGeometryCache.size(); ++i)
			{
				for (uint32_t j = 0; j < mGeometryCache[i].size(); ++j)
				{

					PxPlane nPlane = PxPlane(mGeometryCache[i][j].a.p, mGeometryCache[i][j].b.p, mGeometryCache[i][j].c.p);
					mPlaneCache.push_back(PlaneChunkIndexer(i, j, nPlane));
				}
			}

			for (uint32_t ch = 0; ch < mGeometryCache.size(); ++ch)
			{
				std::vector<PxVec3>  chunksPoints(mGeometryCache[ch].size() * 3);

				int32_t sp = 0;
				for (uint32_t i = 0; i < mGeometryCache[ch].size(); ++i)
				{
					chunksPoints[sp++] = mGeometryCache[ch][i].a.p;
					chunksPoints[sp++] = mGeometryCache[ch][i].b.p;
					chunksPoints[sp++] = mGeometryCache[ch][i].c.p;
				}

				Nv::Blast::ConvexMeshBuilderImpl builder(mPxCooking, mPxInsertionCallback);

				mCHullCache[ch] = builder.buildCollisionGeometry(chunksPoints.size(), chunksPoints.data());

				mHullsPointsCache[ch].resize(mCHullCache[ch]->pointsCount);

				mBoundsCache[ch].setEmpty();
				for (uint32_t i = 0; i < mCHullCache[ch]->pointsCount; ++i)
				{
					mHullsPointsCache[ch][i] = mCHullCache[ch]->points[i];
					mBoundsCache[ch].include(mHullsPointsCache[ch][i]);
				}
			}
		}

		void BlastBondGeneratorImpl::resetGeometryCache()
		{			
			mGeometryCache.clear();
			mPlaneCache.clear();
			mHullsPointsCache.clear();
			for (auto h : mCHullCache)
			{
				h->release();
			}
			mCHullCache.clear();
			mBoundsCache.clear();
		}

		int32_t BlastBondGeneratorImpl::createFullBondListExactInternal(uint32_t meshCount, const uint32_t* geometryOffset, const Triangle* geometry, 
			std::vector<PlaneChunkIndexer>& planeTriangleMapping, NvBlastBondDesc*& resultBondDescs)
		{
			NV_UNUSED(meshCount);

			std::map<std::pair<int32_t, int32_t>, std::pair<NvBlastBondDesc, int32_t> > bonds;

			TriangleProcessor trPrc;
			std::vector<PxVec3> intersectionBufferLocal;

			NvBlastBondDesc cleanBond;
			memset(&cleanBond, 0, sizeof(NvBlastBondDesc));
			for (uint32_t tIndex = 0; tIndex < planeTriangleMapping.size(); ++tIndex)
			{
				
				PlaneChunkIndexer opp = planeTriangleMapping[tIndex];

				opp.plane.d *= -1;
				opp.plane.n *= -1;

				uint32_t startIndex = (uint32_t)(std::lower_bound(planeTriangleMapping.begin(), planeTriangleMapping.end(), opp, planeComparer) - planeTriangleMapping.begin());
				uint32_t endIndex = (uint32_t)(std::upper_bound(planeTriangleMapping.begin(), planeTriangleMapping.end(), opp, planeComparer) - planeTriangleMapping.begin());
				//	uint32_t startIndex = 0;
				//	uint32_t endIndex = (uint32_t)planeTriangleMapping.size();

				PlaneChunkIndexer& mappedTr = planeTriangleMapping[tIndex];
				const Triangle& trl = geometry[geometryOffset[mappedTr.chunkId] + mappedTr.trId];
				PxPlane pln = mappedTr.plane;
				TrPrcTriangle trp(trl.a.p, trl.b.p, trl.c.p);
				PxVec3 trCentroid = (trl.a.p + trl.b.p + trl.c.p) * (1.0f / 3.0f);
				trp.points[0] -= trCentroid;
				trp.points[1] -= trCentroid;
				trp.points[2] -= trCentroid;
				ProjectionDirections pDir = getProjectionDirection(pln.n);
				TrPrcTriangle2d trp2d;
				trp2d.points[0] = getProjectedPointWithWinding(trp.points[0], pDir);
				trp2d.points[1] = getProjectedPointWithWinding(trp.points[1], pDir);
				trp2d.points[2] = getProjectedPointWithWinding(trp.points[2], pDir);

				for (uint32_t i = startIndex; i <= endIndex && i < planeTriangleMapping.size(); ++i)
				{
					PlaneChunkIndexer& mappedTr2 = planeTriangleMapping[i];
					if (mappedTr2.trId == opp.chunkId)
					{
						continue;
					}

					if (!isSamePlane(opp.plane, mappedTr2.plane))
					{
						continue;
					}
					
					if (mappedTr.chunkId == mappedTr2.chunkId)
					{
						continue;
					}
					std::pair<int32_t, int32_t> bondEndPoints = std::make_pair(mappedTr.chunkId, mappedTr2.chunkId);
					if (bondEndPoints.second < bondEndPoints.first) continue;
					std::pair<int32_t, int32_t> bondEndPointsSwapped = std::make_pair(mappedTr2.chunkId, mappedTr.chunkId);
					if (bonds.find(bondEndPoints) == bonds.end() && bonds.find(bondEndPointsSwapped) != bonds.end())
					{
						continue; // We do not need account interface surface twice
					}
					if (bonds.find(bondEndPoints) == bonds.end())
					{
						bonds[bondEndPoints].second = 0;
						bonds[bondEndPoints].first = cleanBond;
						bonds[bondEndPoints].first.chunkIndices[0] = bondEndPoints.first;
						bonds[bondEndPoints].first.chunkIndices[1] = bondEndPoints.second;
						bonds[bondEndPoints].first.bond.normal[0] = pln.n[0];
						bonds[bondEndPoints].first.bond.normal[1] = pln.n[1];
						bonds[bondEndPoints].first.bond.normal[2] = pln.n[2];
					}
					const Triangle& trl2 = geometry[geometryOffset[mappedTr2.chunkId] + mappedTr2.trId];

					TrPrcTriangle trp2(trl2.a.p, trl2.b.p, trl2.c.p);

					intersectionBufferLocal.clear();
					intersectionBufferLocal.reserve(32);
					trPrc.getTriangleIntersection(trp, trp2d, trp2, trCentroid, intersectionBufferLocal, pln.n);
					PxVec3 centroidPoint(0, 0, 0);
					int32_t collectedVerticesCount = 0;
					float area = 0;
					if (intersectionBufferLocal.size() >= 3)
					{
#ifdef DEBUG_OUTPUT
						for (uint32_t p = 1; p < intersectionBufferLocal.size() - 1; ++p)
						{
							intersectionBuffer.push_back(intersectionBufferLocal[0]);
							intersectionBuffer.push_back(intersectionBufferLocal[p]);
							intersectionBuffer.push_back(intersectionBufferLocal[p + 1]);
						}
#endif
						centroidPoint = intersectionBufferLocal[0] + intersectionBufferLocal.back();
						collectedVerticesCount = 2;

						for (uint32_t j = 1; j < intersectionBufferLocal.size() - 1; ++j)
						{
							++collectedVerticesCount;
							centroidPoint += intersectionBufferLocal[j];
							area += (intersectionBufferLocal[j + 1] - intersectionBufferLocal[0]).cross(intersectionBufferLocal[j] - intersectionBufferLocal[0]).magnitude();
						}
					}
					if (area > 0.00001f)
					{
						bonds[bondEndPoints].second += collectedVerticesCount;

						bonds[bondEndPoints].first.bond.area += area * 0.5f;
						bonds[bondEndPoints].first.bond.centroid[0] += (centroidPoint.x);
						bonds[bondEndPoints].first.bond.centroid[1] += (centroidPoint.y);
						bonds[bondEndPoints].first.bond.centroid[2] += (centroidPoint.z);
					}
				}
			}

			std::vector<NvBlastBondDesc> mResultBondDescs;
			for (auto it : bonds)
			{
				if (it.second.first.bond.area > 0)
				{
					float mlt = 1.0f / (it.second.second);
					it.second.first.bond.centroid[0] *= mlt;
					it.second.first.bond.centroid[1] *= mlt;
					it.second.first.bond.centroid[2] *= mlt;

					mResultBondDescs.push_back(it.second.first);
				}

			}
#ifdef DEBUG_OUTPUT
			saveGeometryToObj(meshBuffer, "Mesh.obj");
			saveGeometryToObj(intersectionBuffer, "inter.obj");
#endif
			resultBondDescs = SAFE_ARRAY_NEW(NvBlastBondDesc, mResultBondDescs.size());
			memcpy(resultBondDescs, mResultBondDescs.data(), sizeof(NvBlastBondDesc)*mResultBondDescs.size());
			return mResultBondDescs.size();
		}

		int32_t BlastBondGeneratorImpl::createBondForcedInternal(const std::vector<PxVec3>& hull0, const std::vector<PxVec3>& hull1,
															const CollisionHull& cHull0,const CollisionHull& cHull1,
															PxBounds3 bound0, PxBounds3 bound1, NvBlastBond& resultBond, float overlapping)
		{

			TriangleProcessor trProcessor;
			Separation separation;
			importerHullsInProximityApexFree(hull0.size(), hull0.data(), bound0, PxTransform(PxIdentity), PxVec3(1, 1, 1), hull1.size(), hull1.data(), bound1, PxTransform(PxIdentity), PxVec3(1, 1, 1), 0.000, &separation);

			if (std::isnan(separation.plane.d))
			{				
				importerHullsInProximityApexFree(hull0.size(), hull0.data(), bound0, PxTransform(PxVec3(0.000001f, 0.000001f, 0.000001f)), PxVec3(1, 1, 1), hull1.size(), hull1.data(), bound1, PxTransform(PxIdentity), PxVec3(1, 1, 1), 0.000, &separation);
				if (std::isnan(separation.plane.d))
				{
					return 1;
				}
			}

			PxPlane pl = separation.plane;
			std::vector<PxVec3> ifsPoints[2];

			float dst[2][2];

			dst[0][0] = 0;
			dst[0][1] = MAXIMUM_EXTENT;
			for (uint32_t p = 0; p < cHull0.pointsCount; ++p)
			{
				float d = pl.distance(cHull0.points[p]);
				if (PxAbs(d) > PxAbs(dst[0][0]))
				{
					dst[0][0] = d;
				}
				if (PxAbs(d) < PxAbs(dst[0][1]))
				{
					dst[0][1] = d;
				}
			}

			dst[1][0] = 0;
			dst[1][1] = MAXIMUM_EXTENT;
			for (uint32_t p = 0; p < cHull1.pointsCount; ++p)
			{
				float d = pl.distance(cHull0.points[p]);
				if (PxAbs(d) > PxAbs(dst[1][0]))
				{
					dst[1][0] = d;
				}
				if (PxAbs(d) < PxAbs(dst[1][1]))
				{
					dst[1][1] = d;
				}
			}

		
			float cvOffset[2] = { dst[0][1] + (dst[0][0] - dst[0][1]) * overlapping, dst[1][1] + (dst[1][0] - dst[1][1]) * overlapping };

			for (uint32_t i = 0; i < cHull0.polygonDataCount; ++i)
			{
				auto& pd = cHull0.polygonData[i];
				PxVec3 result;
				for (uint32_t j = 0; j < pd.mNbVerts; ++j)
				{
					uint32_t nxj = (j + 1) % pd.mNbVerts;
					const uint32_t* ind = cHull0.indices;
					PxVec3 a = hull0[ind[j + pd.mIndexBase]] - pl.n * cvOffset[0];
					PxVec3 b = hull0[ind[nxj + pd.mIndexBase]] - pl.n * cvOffset[0];

					if (getPlaneSegmentIntersection(pl, a, b, result))
					{
						ifsPoints[0].push_back(result);
					}
				}
			}

			for (uint32_t i = 0; i < cHull1.polygonDataCount; ++i)
			{
				auto& pd = cHull1.polygonData[i];
				PxVec3 result;
				for (uint32_t j = 0; j < pd.mNbVerts; ++j)
				{
					uint32_t nxj = (j + 1) % pd.mNbVerts;
					const uint32_t* ind = cHull1.indices;
					PxVec3 a = hull1[ind[j + pd.mIndexBase]] - pl.n * cvOffset[1];
					PxVec3 b = hull1[ind[nxj + pd.mIndexBase]] - pl.n * cvOffset[1];

					if (getPlaneSegmentIntersection(pl, a, b, result))
					{
						ifsPoints[1].push_back(result);
					}
				}
			}


			std::vector<PxVec3> convexes[2];

			trProcessor.buildConvexHull(ifsPoints[0], convexes[0], pl.n);
			trProcessor.buildConvexHull(ifsPoints[1], convexes[1], pl.n);

			float areas[2] = { 0, 0 };
			PxVec3 centroids[2] = { PxVec3(0, 0, 0), PxVec3(0, 0, 0) };

			for (uint32_t cv = 0; cv < 2; ++cv)
			{
				if (convexes[cv].size() == 0)
				{
					continue;
				}
				centroids[cv] = convexes[cv][0] + convexes[cv].back();
				for (uint32_t i = 1; i < convexes[cv].size() - 1; ++i)
				{
					centroids[cv] += convexes[cv][i];
					areas[cv] += (convexes[cv][i + 1] - convexes[cv][0]).cross(convexes[cv][i] - convexes[cv][0]).magnitude();
#ifdef DEBUG_OUTPUT
					intersectionBuffer.push_back(convexes[cv][0]);
					intersectionBuffer.push_back(convexes[cv][i]);
					intersectionBuffer.push_back(convexes[cv][i + 1]);
#endif

				}
				centroids[cv] *= (1.0f / convexes[cv].size());
				areas[cv] = PxAbs(areas[cv]);
			}

			resultBond.area = (areas[0] + areas[1]) * 0.5f;
			resultBond.centroid[0] = (centroids[0][0] + centroids[1][0]) * 0.5f;
			resultBond.centroid[1] = (centroids[0][1] + centroids[1][1]) * 0.5f;
			resultBond.centroid[2] = (centroids[0][2] + centroids[1][2]) * 0.5f;
			resultBond.normal[0] = pl.n[0];
			resultBond.normal[1] = pl.n[1];
			resultBond.normal[2] = pl.n[2];

#ifdef DEBUG_OUTPUT
			saveGeometryToObj(meshBuffer, "ArbitMeshes.obj");
			saveGeometryToObj(intersectionBuffer, "inter.obj");
#endif


			return 0;				
		}

		int32_t	BlastBondGeneratorImpl::buildDescFromInternalFracture(FractureTool* tool, const bool* chunkIsSupport,
			NvBlastBondDesc*& resultBondDescs, NvBlastChunkDesc*& resultChunkDescriptors)
		{
			uint32_t chunkCount = tool->getChunkCount();
			std::vector<uint32_t> trianglesCount(chunkCount);
			std::vector<Triangle*> trianglesBuffer(chunkCount);

			for (uint32_t i = 0; i < trianglesBuffer.size(); ++i)
			{
				trianglesCount[i] = tool->getBaseMesh(i, trianglesBuffer[i]);
			}

			if (chunkCount == 0)
			{
				return 0;
			}
			resultChunkDescriptors = SAFE_ARRAY_NEW(NvBlastChunkDesc, trianglesBuffer.size());
			std::vector<Bond>	bondDescriptors;
			resultChunkDescriptors[0].parentChunkIndex = UINT32_MAX;
			resultChunkDescriptors[0].userData = 0;
			resultChunkDescriptors[0].flags = NvBlastChunkDesc::NoFlags;

			{
				PxVec3 chunkCentroid(0, 0, 0);
				for (uint32_t tr = 0; tr < trianglesCount[0]; ++tr)
				{
					chunkCentroid += trianglesBuffer[0][tr].a.p;
					chunkCentroid += trianglesBuffer[0][tr].b.p;
					chunkCentroid += trianglesBuffer[0][tr].c.p;
				}
				chunkCentroid *= (1.0f / (3 * trianglesCount[0]));
				resultChunkDescriptors[0].centroid[0] = chunkCentroid[0];
				resultChunkDescriptors[0].centroid[1] = chunkCentroid[1];
				resultChunkDescriptors[0].centroid[2] = chunkCentroid[2];
			}

			for (uint32_t i = 1; i < chunkCount; ++i)
			{
				NvBlastChunkDesc& desc = resultChunkDescriptors[i];
				desc.userData = i;
				desc.parentChunkIndex = tool->getChunkIndex(tool->getChunkInfo(i).parent);
				desc.flags = NvBlastChunkDesc::NoFlags;
				if (chunkIsSupport[i])
					desc.flags = NvBlastChunkDesc::SupportFlag;
				PxVec3 chunkCentroid(0, 0, 0);
				for (uint32_t tr = 0; tr < trianglesCount[i]; ++tr)
				{
					chunkCentroid += trianglesBuffer[i][tr].a.p;
					chunkCentroid += trianglesBuffer[i][tr].b.p;
					chunkCentroid += trianglesBuffer[i][tr].c.p;

					Triangle& trRef = trianglesBuffer[i][tr];
					int32_t id = trRef.userData;
					if (id == 0)
						continue;
					bondDescriptors.push_back(Bond());
					Bond& bond = bondDescriptors.back();
					bond.m_chunkId = i;
					bond.m_planeIndex = id;
					bond.triangleIndex = tr;
				}
				chunkCentroid *= (1.0f / (3 * trianglesCount[i]));
				desc.centroid[0] = chunkCentroid[0];
				desc.centroid[1] = chunkCentroid[1];
				desc.centroid[2] = chunkCentroid[2];
			}
			std::sort(bondDescriptors.begin(), bondDescriptors.end());
			if (bondDescriptors.empty())
			{
				return 0;
			}
			int32_t chunkId, planeId;
			chunkId = bondDescriptors[0].m_chunkId;
			planeId = bondDescriptors[0].m_planeIndex;
			std::vector<BondInfo> forwardChunks;
			std::vector<BondInfo> backwardChunks;

			float area = 0;
			PxVec3 normal(0, 0, 0);
			PxVec3 centroid(0, 0, 0);
			int32_t collected = 0;
			PxBounds3 bb = PxBounds3::empty();

			chunkId = -1;
			planeId = bondDescriptors[0].m_planeIndex;
			std::vector<NvBlastBondDesc> mResultBondDescs;
			for (uint32_t i = 0; i <= bondDescriptors.size(); ++i)
			{
				if (i == bondDescriptors.size() || (chunkId != bondDescriptors[i].m_chunkId || abs(planeId) != abs(bondDescriptors[i].m_planeIndex)))
				{
					if (chunkId != -1)
					{
						if (bondDescriptors[i - 1].m_planeIndex > 0) {
							forwardChunks.push_back(BondInfo());
							forwardChunks.back().area = area;
							forwardChunks.back().normal = normal;
							forwardChunks.back().centroid = centroid * (1.0f / 3.0f / collected);
							forwardChunks.back().m_chunkId = chunkId;
							forwardChunks.back().m_bb = bb;

						}
						else
						{
							backwardChunks.push_back(BondInfo());
							backwardChunks.back().area = area;
							backwardChunks.back().normal = normal;
							backwardChunks.back().centroid = centroid * (1.0f / 3.0f / collected);
							backwardChunks.back().m_chunkId = chunkId;
							backwardChunks.back().m_bb = bb;
						}
					}
					bb.setEmpty();
					collected = 0;
					area = 0;
					normal = PxVec3(0, 0, 0);
					centroid = PxVec3(0, 0, 0);
					if (i != bondDescriptors.size())
						chunkId = bondDescriptors[i].m_chunkId;
				}
				if (i == bondDescriptors.size() || abs(planeId) != abs(bondDescriptors[i].m_planeIndex))
				{
					for (uint32_t fchunk = 0; fchunk < forwardChunks.size(); ++fchunk)
					{
						for (uint32_t bchunk = 0; bchunk < backwardChunks.size(); ++bchunk)
						{
							if (weakBoundingBoxIntersection(forwardChunks[fchunk].m_bb, backwardChunks[bchunk].m_bb) == 0)
							{
								continue;
							}
							if (chunkIsSupport[forwardChunks[fchunk].m_chunkId] == false || chunkIsSupport[backwardChunks[bchunk].m_chunkId] == false)
							{
								continue;
							}
							mResultBondDescs.push_back(NvBlastBondDesc());
							mResultBondDescs.back().bond.area = std::min(forwardChunks[fchunk].area, backwardChunks[bchunk].area);
							mResultBondDescs.back().bond.normal[0] = forwardChunks[fchunk].normal.x;
							mResultBondDescs.back().bond.normal[1] = forwardChunks[fchunk].normal.y;
							mResultBondDescs.back().bond.normal[2] = forwardChunks[fchunk].normal.z;

							mResultBondDescs.back().bond.centroid[0] = (forwardChunks[fchunk].centroid.x + backwardChunks[bchunk].centroid.x ) * 0.5;
							mResultBondDescs.back().bond.centroid[1] = (forwardChunks[fchunk].centroid.y + backwardChunks[bchunk].centroid.y) * 0.5;
							mResultBondDescs.back().bond.centroid[2] = (forwardChunks[fchunk].centroid.z + backwardChunks[bchunk].centroid.z) * 0.5;


							mResultBondDescs.back().chunkIndices[0] = forwardChunks[fchunk].m_chunkId;
							mResultBondDescs.back().chunkIndices[1] = backwardChunks[bchunk].m_chunkId;
						}
					}
					forwardChunks.clear();
					backwardChunks.clear();
					if (i != bondDescriptors.size())
					{
						planeId = bondDescriptors[i].m_planeIndex;
					}
					else
					{
						break;
					}
				}

				collected++;
				int32_t tr = bondDescriptors[i].triangleIndex;
				PxVec3 n = trianglesBuffer[chunkId][tr].getNormal();
				area += n.magnitude();
				normal = n.getNormalized();
				centroid += trianglesBuffer[chunkId][tr].a.p;
				centroid += trianglesBuffer[chunkId][tr].b.p;
				centroid += trianglesBuffer[chunkId][tr].c.p;

				bb.include(trianglesBuffer[chunkId][tr].a.p);
				bb.include(trianglesBuffer[chunkId][tr].b.p);
				bb.include(trianglesBuffer[chunkId][tr].c.p);
			}

			resultBondDescs = SAFE_ARRAY_NEW(NvBlastBondDesc, mResultBondDescs.size());
			memcpy(resultBondDescs, mResultBondDescs.data(), sizeof(NvBlastBondDesc) * mResultBondDescs.size());

			return mResultBondDescs.size();
		}

		int32_t	BlastBondGeneratorImpl::createBondBetweenMeshes(uint32_t meshCount, const uint32_t* geometryOffset, const Triangle* geometry,
			uint32_t overlapsCount, const uint32_t* overlapsA, const uint32_t* overlapsB, NvBlastBondDesc*& resultBond, BondGenerationConfig cfg)
		{
			if (cfg.bondMode == BondGenerationConfig::AVERAGE)
			{
				resetGeometryCache();
				buildGeometryCache(meshCount, geometryOffset, geometry);
			}
			resultBond = SAFE_ARRAY_NEW(NvBlastBondDesc, overlapsCount);
		
			if (cfg.bondMode == BondGenerationConfig::EXACT)
			{
				for (uint32_t i = 0; i < overlapsCount; ++i)
				{
					NvBlastBondDesc& desc = resultBond[i];
					desc.chunkIndices[0] = overlapsA[i];
					desc.chunkIndices[1] = overlapsB[i];
					uint32_t meshACount = geometryOffset[overlapsA[i] + 1] - geometryOffset[overlapsA[i]];
					uint32_t meshBCount = geometryOffset[overlapsB[i] + 1] - geometryOffset[overlapsB[i]];
					createBondBetweenMeshes(meshACount, geometry + geometryOffset[overlapsA[i]],
						meshBCount, geometry + geometryOffset[overlapsB[i]], desc.bond, cfg);
				}
			}
			else
			{
				for (uint32_t i = 0; i < overlapsCount; ++i)
				{
					NvBlastBondDesc& desc = resultBond[i];
					desc.chunkIndices[0] = overlapsA[i];
					desc.chunkIndices[1] = overlapsB[i];
					createBondForcedInternal(mHullsPointsCache[overlapsA[i]], mHullsPointsCache[overlapsB[i]], *mCHullCache[overlapsA[i]], *mCHullCache[overlapsB[i]],
						mBoundsCache[overlapsA[i]], mBoundsCache[overlapsB[i]], desc.bond, 0.3f);
				}
			}
		
			return overlapsCount;
		}

		int32_t	BlastBondGeneratorImpl::createBondBetweenMeshes(uint32_t meshACount, const Triangle* meshA, uint32_t meshBCount, const Triangle* meshB,
			NvBlastBond& resultBond, BondGenerationConfig conf)
		{
			float overlapping = 0.3;
			if (conf.bondMode == BondGenerationConfig::EXACT)
			{
				std::vector<uint32_t> chunksOffsets = { 0, meshACount, meshACount + meshBCount };
				std::vector<Triangle> chunks;
				chunks.resize(meshACount + meshBCount);
				memcpy(chunks.data(), meshA, sizeof(Triangle) * meshACount);
				memcpy(chunks.data() + meshACount, meshB, sizeof(Triangle) * meshBCount);
				std::shared_ptr<bool> isSupport(new bool[2] {true, true}, [](bool* b) { delete[] b; });
				NvBlastBondDesc* desc;
				uint32_t descSize = createFullBondListExact(2, chunksOffsets.data(), chunks.data(), isSupport.get(), desc, conf);
				if (descSize > 0)
				{
					resultBond = desc->bond; 
				}
				else
				{
					memset(&resultBond, 0, sizeof(NvBlastBond));
					return 1;
				}
				return 0;
			}
		
			std::vector<PxVec3>  chunksPoints1(meshACount * 3);
			std::vector<PxVec3>  chunksPoints2(meshBCount * 3);

			int32_t sp = 0;
			for (uint32_t i = 0; i < meshACount; ++i)
			{
				chunksPoints1[sp++] = meshA[i].a.p;
				chunksPoints1[sp++] = meshA[i].b.p;
				chunksPoints1[sp++] = meshA[i].c.p;
#ifdef DEBUG_OUTPUT
				meshBuffer.push_back(meshA[i].a.p);
				meshBuffer.push_back(meshA[i].b.p);
				meshBuffer.push_back(meshA[i].c.p);
#endif


			}
			sp = 0;
			for (uint32_t i = 0; i < meshBCount; ++i)
			{
				chunksPoints2[sp++] = meshB[i].a.p;
				chunksPoints2[sp++] = meshB[i].b.p;
				chunksPoints2[sp++] = meshB[i].c.p;
#ifdef DEBUG_OUTPUT
				meshBuffer.push_back(meshB[i].a.p);
				meshBuffer.push_back(meshB[i].b.p);
				meshBuffer.push_back(meshB[i].c.p);
#endif
			}


			Nv::Blast::ConvexMeshBuilderImpl builder(mPxCooking, mPxInsertionCallback);

			CollisionHull* cHull[2];

			cHull[0] = builder.buildCollisionGeometry(chunksPoints1.size(), chunksPoints1.data());
			cHull[1] = builder.buildCollisionGeometry(chunksPoints2.size(), chunksPoints2.data());

			std::vector<PxVec3> hullPoints[2];
			hullPoints[0].resize(cHull[0]->pointsCount);
			hullPoints[1].resize(cHull[1]->pointsCount);


			PxBounds3 bb[2];
			bb[0].setEmpty();
			bb[1].setEmpty();

			for (uint32_t cv = 0; cv < 2; ++cv)
			{
				for (uint32_t i = 0; i < cHull[cv]->pointsCount; ++i)
				{
					hullPoints[cv][i] = cHull[cv]->points[i];
					bb[cv].include(hullPoints[cv][i]);
				}
			}		
			auto ret = createBondForcedInternal(hullPoints[0], hullPoints[1], *cHull[0], *cHull[1], bb[0], bb[1], resultBond, overlapping);

			cHull[0]->release();
			cHull[1]->release();

			return ret;
		}

		int32_t	BlastBondGeneratorImpl::bondsFromPrefractured(uint32_t meshCount, const uint32_t* geometryCount, const Triangle* geometry,
			const bool*& chunkIsSupport, NvBlastBondDesc*& resultBondDescs, BondGenerationConfig conf)
		{
			int32_t ret_val = 0;
			switch (conf.bondMode)
			{
			case BondGenerationConfig::AVERAGE:
				ret_val = createFullBondListAveraged(meshCount, geometryCount, geometry, chunkIsSupport, resultBondDescs, conf);
				break;
			case BondGenerationConfig::EXACT:
				ret_val = createFullBondListExact(meshCount, geometryCount, geometry, chunkIsSupport, resultBondDescs, conf);
				break;
			}
			return ret_val;
		}

		void BlastBondGeneratorImpl::release()
		{
			delete this;
		}

	}
}
