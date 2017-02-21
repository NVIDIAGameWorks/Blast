/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

// This warning arises when using some stl containers with older versions of VC
// c:\program files (x86)\microsoft visual studio 12.0\vc\include\xtree(1826): warning C4702: unreachable code
#include "NvPreprocessor.h"
#if NV_VC && NV_VC < 14
#pragma warning(disable : 4702)
#endif

#include <NvBlastExtAuthoringBondGenerator.h>
#include <NvBlastTypes.h>
#include <NvBlast.h>
#include "NvBlastExtTriangleProcessor.h"
#include "NvBlastExtApexSharedParts.h"
#include "NvBlastExtAuthoringCollisionBuilder.h"
#include "NvBlastExtAuthoringInternalCommon.h"
#include <vector>
#include <map>
#include <PxPlane.h>
#include <algorithm>
#include <cmath>

using physx::PxVec3;
using physx::PxBounds3;

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


		float BlastBondGenerator::processWithMidplanes(TriangleProcessor* trProcessor, const std::vector<PxVec3>& chunk1Points, const std::vector<PxVec3>& chunk2Points,
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
			if (!importerHullsInProximityApexFree(hull1p, aBounds, PxTransform(PxIdentity), PxVec3(1, 1, 1), hull2p, bBounds, PxTransform(PxIdentity), PxVec3(1, 1, 1), 0.000, &separation))
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


		int32_t BlastBondGenerator::bondsFromPrefractured(const std::vector<std::vector<Triangle>>& geometry, const std::vector<bool>& chunkIsSupport, std::vector<NvBlastBondDesc>& resultBondDescs, BondGenerationConfig conf)
		{
			int32_t ret_val = 0;
			switch (conf.bondMode)
			{
			case BondGenerationConfig::AVERAGE:
				ret_val = createFullBondListAveraged(geometry, chunkIsSupport, resultBondDescs, conf);
				break;
			case BondGenerationConfig::EXACT:
				ret_val = createFullBondListExact(geometry, chunkIsSupport, resultBondDescs, conf);
				break;
			}
			return ret_val;
		}

		int32_t BlastBondGenerator::createFullBondListAveraged(const std::vector<std::vector<Triangle>>& chunksGeometry, const std::vector<bool>& supportFlags, std::vector<NvBlastBondDesc>& mResultBondDescs, BondGenerationConfig conf)
		{
			NV_UNUSED(conf);

			std::vector<std::vector<PxVec3> > chunksPoints(chunksGeometry.size());

			for (uint32_t i = 0; i < chunksGeometry.size(); ++i)
			{
				if (!supportFlags[i])
				{
					continue;
				}
				for (uint32_t j = 0; j < chunksGeometry[i].size(); ++j)
				{
					chunksPoints[i].push_back(chunksGeometry[i][j].a.p);
					chunksPoints[i].push_back(chunksGeometry[i][j].b.p);
					chunksPoints[i].push_back(chunksGeometry[i][j].c.p);
				}
			}

			Nv::Blast::ConvexMeshBuilder builder(mPxCooking, mPxInsertionCallback);

			std::vector<CollisionHull> cHulls(chunksGeometry.size());

			for (uint32_t i = 0; i < chunksGeometry.size(); ++i)
			{
				if (!supportFlags[i])
				{
					continue;
				}
				builder.buildCollisionGeometry(chunksPoints[i], cHulls[i]);
			}

			std::vector<std::vector<PxVec3> > hullPoints(cHulls.size());

			for (uint32_t chunk = 0; chunk < cHulls.size(); ++chunk)
			{
				if (!supportFlags[chunk])
				{
					continue;
				}

				hullPoints[chunk].resize(cHulls[chunk].points.size());
				for (uint32_t i = 0; i < cHulls[chunk].points.size(); ++i)
				{
					hullPoints[chunk][i].x = cHulls[chunk].points[i].x;
					hullPoints[chunk][i].y = cHulls[chunk].points[i].y;
					hullPoints[chunk][i].z = cHulls[chunk].points[i].z;
				}
			}

			TriangleProcessor trProcessor;

			for (uint32_t i = 0; i < chunksGeometry.size(); ++i)
			{
				if (!supportFlags[i])
				{
					continue;
				}
				for (uint32_t j = i + 1; j < chunksGeometry.size(); ++j)
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

			return 0;
		}

		uint32_t isSamePlane(PxPlane& a, PxPlane& b)
		{
			if (PxAbs(a.d - b.d) > EPS_PLANE) return 0;
			if (PxAbs(a.n.x - b.n.x) > EPS_PLANE) return 0;
			if (PxAbs(a.n.y - b.n.y) > EPS_PLANE) return 0;
			if (PxAbs(a.n.z - b.n.z) > EPS_PLANE) return 0;
			return 1;
		}

		int32_t BlastBondGenerator::createFullBondListExact(const std::vector<std::vector<Triangle>>& chunksGeometry, const std::vector<bool>& supportFlags, std::vector<NvBlastBondDesc>& mResultBondDescs, BondGenerationConfig conf)
		{		
			std::vector < PlaneChunkIndexer > planeTriangleMapping;
			NV_UNUSED(conf);
			for (uint32_t i = 0; i < chunksGeometry.size(); ++i)
			{
				if (!supportFlags[i])
				{
					continue;
				}
				for (uint32_t j = 0; j < chunksGeometry[i].size(); ++j)
				{
#ifdef DEBUG_OUTPUT
					meshBuffer.push_back(chunksGeometry[i][j].a.p );
					meshBuffer.push_back(chunksGeometry[i][j].b.p);
					meshBuffer.push_back(chunksGeometry[i][j].c.p );
#endif

					PxPlane nPlane = PxPlane(chunksGeometry[i][j].a.p, chunksGeometry[i][j].b.p, chunksGeometry[i][j].c.p);
					planeTriangleMapping.push_back(PlaneChunkIndexer(i, j, nPlane));
				}
			}

			std::sort(planeTriangleMapping.begin(), planeTriangleMapping.end(), planeComparer);
			return createFullBondListExactInternal(chunksGeometry, planeTriangleMapping, mResultBondDescs);
		}

		void BlastBondGenerator::buildGeometryCache(const std::vector<std::vector<Triangle> >& geometry)
		{
			mGeometryCache = geometry;
			mHullsPointsCache.resize(geometry.size());
			mBoundsCache.resize(geometry.size());
			mCHullCache.resize(geometry.size());
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

				Nv::Blast::ConvexMeshBuilder builder(mPxCooking, mPxInsertionCallback);

				CollisionHull& cHull = mCHullCache[ch];

				builder.buildCollisionGeometry(chunksPoints, cHull);

				mHullsPointsCache[ch].resize(cHull.points.size());

				mBoundsCache[ch].setEmpty();
				for (uint32_t i = 0; i < cHull.points.size(); ++i)
				{
					mHullsPointsCache[ch][i].x = cHull.points[i].x;
					mHullsPointsCache[ch][i].y = cHull.points[i].y;
					mHullsPointsCache[ch][i].z = cHull.points[i].z;
					mBoundsCache[ch].include(mHullsPointsCache[ch][i]);
				}
			}
		}

		void BlastBondGenerator::resetGeometryCache()
		{			
			mGeometryCache.clear();
			mPlaneCache.clear();
			mHullsPointsCache.clear();
			mCHullCache.clear();
			mBoundsCache.clear();
		}

		int32_t BlastBondGenerator::createFullBondListExactInternal(const std::vector<std::vector<Triangle>>& chunksGeometry, std::vector < PlaneChunkIndexer >& planeTriangleMapping, std::vector<NvBlastBondDesc>& mResultBondDescs)
		{
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
				const Triangle& trl = chunksGeometry[mappedTr.chunkId][mappedTr.trId];
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

					const Triangle& trl2 = chunksGeometry[mappedTr2.chunkId][mappedTr2.trId];

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
			return 0;
		}

		int32_t BlastBondGenerator::createBondForcedInternal(const std::vector<PxVec3>& hull0, const std::vector<PxVec3>& hull1,
															const CollisionHull& cHull0,const CollisionHull& cHull1,
															PxBounds3 bound0, PxBounds3 bound1, NvBlastBond& resultBond, float overlapping)
		{

			TriangleProcessor trProcessor;
			Separation separation;
			importerHullsInProximityApexFree(hull0, bound0, PxTransform(PxIdentity), PxVec3(1, 1, 1), hull1, bound1, PxTransform(PxIdentity), PxVec3(1, 1, 1), 0.000, &separation);

			if (std::isnan(separation.plane.d))
			{				
				importerHullsInProximityApexFree(hull0, bound0, PxTransform(PxVec3(0.000001f, 0.000001f, 0.000001f)), PxVec3(1, 1, 1), hull1, bound1, PxTransform(PxIdentity), PxVec3(1, 1, 1), 0.000, &separation);
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
			for (uint32_t p = 0; p < cHull0.points.size(); ++p)
			{
				float d = pl.distance(PxVec3(cHull0.points[p].x, cHull0.points[p].y, cHull0.points[p].z));
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
			for (uint32_t p = 0; p < cHull1.points.size(); ++p)
			{
				float d = pl.distance(PxVec3(cHull1.points[p].x, cHull1.points[p].y, cHull1.points[p].z));
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

			for (uint32_t i = 0; i < cHull0.polygonData.size(); ++i)
			{
				uint32_t offset = cHull0.polygonData[i].mIndexBase;
				PxVec3 result;
				for (uint32_t j = 0; j < cHull0.polygonData[i].mNbVerts; ++j)
				{
					uint32_t nxj = (j + 1) % cHull0.polygonData[i].mNbVerts;
					const uint32_t* ind = &cHull0.indices[0];
					PxVec3 a = hull0[ind[j + offset]] - pl.n * cvOffset[0];
					PxVec3 b = hull0[ind[nxj + offset]] - pl.n * cvOffset[0];

					if (getPlaneSegmentIntersection(pl, a, b, result))
					{
						ifsPoints[0].push_back(result);
					}
				}
			}

			for (uint32_t i = 0; i < cHull1.polygonData.size(); ++i)
			{
				uint32_t offset = cHull1.polygonData[i].mIndexBase;
				PxVec3 result;
				for (uint32_t j = 0; j < cHull1.polygonData[i].mNbVerts; ++j)
				{
					uint32_t nxj = (j + 1) % cHull1.polygonData[i].mNbVerts;
					const uint32_t* ind = &cHull1.indices[0];
					PxVec3 a = hull1[ind[j + offset]] - pl.n * cvOffset[1];
					PxVec3 b = hull1[ind[nxj + offset]] - pl.n * cvOffset[1];

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


		int32_t BlastBondGenerator::buildDescFromInternalFracture(FractureTool* tool, const std::vector<bool>& chunkIsSupport, std::vector<NvBlastBondDesc>& mResultBondDescs, std::vector<NvBlastChunkDesc>& mResultChunkDescriptors)
		{
			const std::vector<ChunkInfo>& chunkData = tool->getChunkList();
			std::vector<std::vector<Triangle> > trianglesBuffer(chunkData.size());

			for (uint32_t i = 0; i < trianglesBuffer.size(); ++i)
			{
				tool->getBaseMesh(i, trianglesBuffer[i]);
			}

			if (chunkData.empty() || trianglesBuffer.empty())
			{
				return 1;
			}
			mResultChunkDescriptors.resize(trianglesBuffer.size());
			std::vector<Bond>	bondDescriptors;
			mResultChunkDescriptors[0].parentChunkIndex = UINT32_MAX;
			mResultChunkDescriptors[0].userData = 0;

			{
				PxVec3 chunkCentroid(0, 0, 0);
				for (uint32_t tr = 0; tr < trianglesBuffer[0].size(); ++tr)
				{
					chunkCentroid += trianglesBuffer[0][tr].a.p;
					chunkCentroid += trianglesBuffer[0][tr].b.p;
					chunkCentroid += trianglesBuffer[0][tr].c.p;
				}
				chunkCentroid *= (1.0f / (3 * trianglesBuffer[0].size()));
				mResultChunkDescriptors[0].centroid[0] = chunkCentroid[0];
				mResultChunkDescriptors[0].centroid[1] = chunkCentroid[1];
				mResultChunkDescriptors[0].centroid[2] = chunkCentroid[2];
			}

			for (uint32_t i = 1; i < chunkData.size(); ++i)
			{

				mResultChunkDescriptors[i].userData = i;
				mResultChunkDescriptors[i].parentChunkIndex = tool->getChunkIndex(chunkData[i].parent);
				if (chunkIsSupport[i])
					mResultChunkDescriptors[i].flags = NvBlastChunkDesc::SupportFlag;
				PxVec3 chunkCentroid(0, 0, 0);
				for (uint32_t tr = 0; tr < trianglesBuffer[i].size(); ++tr)
				{
					chunkCentroid += trianglesBuffer[i][tr].a.p;
					chunkCentroid += trianglesBuffer[i][tr].b.p;
					chunkCentroid += trianglesBuffer[i][tr].c.p;

					Triangle& trRef = trianglesBuffer[i][tr];
					int32_t id = trRef.userInfo;
					if (id == 0)
						continue;
					bondDescriptors.push_back(Bond());
					Bond& bond = bondDescriptors.back();
					bond.m_chunkId = i;
					bond.m_planeIndex = id;
					bond.triangleIndex = tr;
				}
				chunkCentroid *= (1.0f / (3 * trianglesBuffer[i].size()));
				mResultChunkDescriptors[i].centroid[0] = chunkCentroid[0];
				mResultChunkDescriptors[i].centroid[1] = chunkCentroid[1];
				mResultChunkDescriptors[i].centroid[2] = chunkCentroid[2];
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

			return 0;
		}

		int32_t BlastBondGenerator::createBondBetweenMeshes(const std::vector<std::vector<Triangle> >& geometry, std::vector<NvBlastBondDesc>& resultBond,const std::vector<std::pair<uint32_t, uint32_t> >& overlaps, BondGenerationConfig cfg)
		{
			if (cfg.bondMode == BondGenerationConfig::AVERAGE)
			{
				resetGeometryCache();
				buildGeometryCache(geometry);
			}
			resultBond.clear();
			resultBond.resize(overlaps.size());
		
			if (cfg.bondMode == BondGenerationConfig::EXACT)
			{
				for (uint32_t i = 0; i < overlaps.size(); ++i)
				{
					resultBond[i].chunkIndices[0] = overlaps[i].first;
					resultBond[i].chunkIndices[1] = overlaps[i].second;					
					createBondBetweenMeshes(geometry[overlaps[i].first], geometry[overlaps[i].second], resultBond[i].bond, cfg);
				}
			}
			else
			{
				for (uint32_t i = 0; i < overlaps.size(); ++i)
				{
					resultBond[i].chunkIndices[0] = overlaps[i].first;
					resultBond[i].chunkIndices[1] = overlaps[i].second;
					createBondForcedInternal(mHullsPointsCache[overlaps[i].first], mHullsPointsCache[overlaps[i].second], mCHullCache[overlaps[i].first], mCHullCache[overlaps[i].second],
						mBoundsCache[overlaps[i].first], mBoundsCache[overlaps[i].second], resultBond[i].bond, 0.3f);
				}
			}
		
			return 0;
		}


		int32_t BlastBondGenerator::createBondBetweenMeshes(const std::vector<Triangle>& meshA, const std::vector<Triangle>& meshB, NvBlastBond& resultBond, BondGenerationConfig conf)
		{
			float overlapping = 0.3;
			if (conf.bondMode == BondGenerationConfig::EXACT)
			{
				std::vector<std::vector<Triangle> > chunks;
				chunks.push_back(meshA);
				chunks.push_back(meshB);
				std::vector<bool> isSupport(2, true);
				std::vector<NvBlastBondDesc> desc;
				createFullBondListExact(chunks, isSupport, desc, conf);
				if (desc.size() > 0)
				{
					resultBond = desc.back().bond; 
				}
				else
				{
					return 1;
				}
				return 0;
			}
		
			std::vector<PxVec3>  chunksPoints1(meshA.size() * 3);
			std::vector<PxVec3>  chunksPoints2(meshB.size() * 3);

			int32_t sp = 0;
			for (uint32_t i = 0; i < meshA.size(); ++i)
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
			for (uint32_t i = 0; i < meshB.size(); ++i)
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


			Nv::Blast::ConvexMeshBuilder builder(mPxCooking, mPxInsertionCallback);

			CollisionHull cHull[2];

			builder.buildCollisionGeometry(chunksPoints1, cHull[0]);
			builder.buildCollisionGeometry(chunksPoints2, cHull[1]);

			std::vector<PxVec3> hullPoints[2];
			hullPoints[0].resize(cHull[0].points.size());
			hullPoints[1].resize(cHull[1].points.size());


			PxBounds3 bb[2];
			bb[0].setEmpty();
			bb[1].setEmpty();

			for (uint32_t cv = 0; cv < 2; ++cv)
			{
				for (uint32_t i = 0; i < cHull[cv].points.size(); ++i)
				{
					hullPoints[cv][i].x = cHull[cv].points[i].x;
					hullPoints[cv][i].y = cHull[cv].points[i].y;
					hullPoints[cv][i].z = cHull[cv].points[i].z;
					bb[cv].include(hullPoints[cv][i]);
				}
			}		
			return createBondForcedInternal(hullPoints[0], hullPoints[1], cHull[0], cHull[1], bb[0], bb[1], resultBond, overlapping);
		}



	}
}
