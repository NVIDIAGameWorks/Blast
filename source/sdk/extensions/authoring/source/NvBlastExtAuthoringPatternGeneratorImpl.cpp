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

#include "NvBlastGlobals.h"
#include "NvBlastAssert.h"
#include "NvBlastExtAuthoringTypes.h"
#include "NvBlastExtAuthoringPatternGeneratorImpl.h"
#include "NvBlastExtAuthoringMeshUtils.h"
#include "NvBlastExtAuthoringMeshImpl.h"
#include "NvBlastExtAuthoringFractureToolImpl.h"
#include "NvBlastExtAuthoringBooleanTool.h"
#include "NvBlastExtAuthoringTriangulator.h"
#include "NvBlastExtAuthoringPerlinNoise.h"
#include <NvBlastPxSharedHelpers.h>

#include <vector>

using namespace Nv::Blast;
using namespace physx;

struct DamagePatternImpl : public DamagePattern
{
	virtual void release() override;
};

DamagePattern* PatternGeneratorImpl::generateUniformPattern(const UniformPatternDesc* desc)
{
	std::vector<NvcVec3> points;
	float radiusDelta = desc->radiusMax - desc->radiusMin;
	for (uint32_t i = 0; i < desc->cellsCount; ++i)
	{
		float rd = desc->RNG() * radiusDelta + desc->radiusMin;

		if (desc->radiusDistr != 1.0f)
		{
			rd = std::pow(rd / desc->radiusMax, desc->radiusDistr) * desc->radiusMax;
		}

		float phi = desc->RNG() * 6.28f;
		float theta = (desc->RNG()) * 6.28f;

		float x = rd * cos(phi) * sin(theta);
		float y = rd * sin(phi) * sin(theta);
		float z = rd * cos(theta);

		points.push_back({x, y, z});
	}


	auto pattern = generateVoronoiPattern((uint32_t)points.size(), points.data(), desc->interiorMaterialId);
	pattern->activationRadius = desc->radiusMax * desc->debrisRadiusMult; 	
	return pattern;
}

DamagePattern* PatternGeneratorImpl::generateVoronoiPattern(uint32_t cellCount, const NvcVec3* inPoints, int32_t interiorMaterialId)
{
	return generateVoronoiPatternInternal(cellCount, inPoints, interiorMaterialId);
}

DamagePattern* PatternGeneratorImpl::generateVoronoiPatternInternal(uint32_t cellCount, const NvcVec3* inPoints, int32_t interiorMaterialId, float angle)
{
	DamagePatternImpl* pattern = NVBLAST_NEW(DamagePatternImpl);

	std::vector<NvcVec3> points(cellCount);
	NvcVec3 orig = {0, 0, 0};
	for (uint32_t i = 0; i < cellCount; ++i)
	{
		points[i] = inPoints[i];
		orig = orig + points[i];
	}
	orig = orig / cellCount;

	std::vector<std::vector<int32_t> > neighboors;
	findCellBasePlanes(points, neighboors);

	Mesh** patterns = (Mesh**)NVBLAST_ALLOC(sizeof(Mesh*) * cellCount);

	//PreparedMesh** prepMeshes = (PreparedMesh**)NVBLAST_ALLOC(sizeof(PreparedMesh*) * cellCount);

	BooleanEvaluator evl;
	for (uint32_t i = 0; i < cellCount; ++i)
	{
		patterns[i] = getCellMesh(evl, 0, i, points, neighboors, interiorMaterialId, orig);
		if (patterns[i] == nullptr)
		{
			continue;
		}
		if (angle != 0)
		{
			auto* vr = patterns[i]->getVerticesWritable();
			for (uint32_t j = 0; j < patterns[i]->getVerticesCount(); ++j)
			{
				float& z = vr[j].p.z;
				z -= 3.8f;
				if (z < -2) // we presume that this vertex has infinite -z position (everything scaled to unit cube).
				{
					if (angle > 0)
					{
						float d = sqrt(vr[j].p.x * vr[j].p.x + vr[j].p.y * vr[j].p.y);

						vr[j].p.x *= (d + 4 * tan(angle * physx::PxPi / 180.f)) / d;
						vr[j].p.y *= (d + 4 * tan(angle * physx::PxPi / 180.f)) / d;
					}
				}
			}
			patterns[i]->recalculateBoundingBox();
		}
	}
	for (int32_t i = cellCount - 1; i >= 0; i--)
	{
		if (patterns[i] == nullptr)
		{
			cellCount--;
			std::swap(patterns[i], patterns[cellCount]);
			//std::swap(prepMeshes[i], prepMeshes[cellCount]);
		}
	}
	pattern->cellsCount = cellCount;
	pattern->cellsMeshes = patterns;
	//pattern->preparedMeshes = prepMeshes;

#ifdef USE_MERGED_MESH
	pattern->outputEdges = NVBLAST_ALLOC(sizeof(BooleanResultEdge) * (cellCount * BLASTRT_MAX_EDGES_PER_CHUNK));
	pattern->outputEdgesCount = (uint32_t*)NVBLAST_ALLOC(sizeof(uint32_t) * cellCount);
#endif

	return pattern;
}

DamagePattern* PatternGeneratorImpl::generateBeamPattern(const BeamPatternDesc* desc)
{
	std::vector<NvcVec3> points;

	float radiusDelta = desc->radiusMax - desc->radiusMin;

	for (uint32_t i = 0; i < desc->cellsCount; ++i)
	{
		float rd = desc->RNG() * radiusDelta + desc->radiusMin;
		float phi = desc->RNG() * 6.28f;

		float x = rd * cos(phi);
		float y = rd * sin(phi);
		float z = desc->RNG() - 1;
		points.push_back({x, y, z});
	}
	auto pattern = generateVoronoiPattern((uint32_t)points.size(), points.data(), desc->interiorMaterialId);
	pattern->activationType = DamagePattern::Line;
	return pattern;
}

DamagePattern* PatternGeneratorImpl::generateRegularRadialPattern(const RegularRadialPatternDesc* desc)
{
	SimplexNoise noise(desc->radialNoiseAmplitude, desc->radialNoiseFrequency, 3, desc->RNG() * 999999);
	std::vector<PxVec3> points;

	float radialDelta = (desc->radiusMax - desc->radiusMin) / desc->radialSteps;
	float angularDelta = 2 * acos(-1.0f) / desc->angularSteps;


	for (uint32_t i = 0; i < desc->radialSteps; ++i)
	{
		for (uint32_t j = 0; j < desc->angularSteps; ++j)
		{
			float angle = j * angularDelta + desc->RNG() * desc->angularNoiseAmplitude;
			float rd = ((i + noise.sample(PxVec3(angle, 0, 0))) * radialDelta + desc->radiusMin);
			float x = rd * cos(angle);
			float y = rd * sin(angle);
			float z = 0;
			points.push_back(PxVec3(x, y, z));
		}
	}
	float mrd = 0.0;
	for (uint32_t i = 0; i < points.size(); ++i)
	{
		mrd = std::max(mrd, points[i].magnitude());
	}
	for (uint32_t i = 0; i < points.size(); ++i)
	{
		points[i] *= desc->radiusMax / mrd;
	}
	
	float ap = std::max(0.0f, desc->aperture);

	auto pattern = generateVoronoiPatternInternal((uint32_t)points.size(), fromPxShared(points.data()), desc->interiorMaterialId, ap);

	pattern->activationRadius = desc->radiusMax * desc->debrisRadiusMult;
	pattern->activationType = (ap == 0) ? DamagePattern::Line : DamagePattern::Cone;
	pattern->angle = ap;
	return pattern;
}



void PatternGeneratorImpl::release()
{
	NVBLAST_DELETE(this, PatternGeneratorImpl);
}

void DamagePatternImpl::release()
{
	if (cellsMeshes)
	{
		for (uint32_t i = 0; i < cellsCount; i++)
		{
			cellsMeshes[i]->release();
		}
		NVBLAST_FREE(cellsMeshes);
	}
#ifdef USE_MERGED_MESH
	if (outputEdges)
	{
		NVBLAST_FREE(outputEdges);
	}
	if (outputEdgesCount)
	{
		NVBLAST_FREE(outputEdgesCount);
	}
	if (mergedMesh)
	{
		mergedMesh->release();
	}
	if (preparedMergedMesh)
	{
		preparedMergedMesh->release();
	}
	if (validFacetsForChunk)
	{
		for (uint32_t i = 0; i < cellsCount; i++)
		{
			if (validFacetsForChunk[i])
			{
				NVBLAST_FREE(validFacetsForChunk[i]);
			}
		}
		NVBLAST_FREE(validFacetsForChunk);
	}
#endif
	NVBLAST_DELETE(this, DamagePatternImpl);
}


namespace Nv 
{
	namespace Blast 
	{
		void savePatternToObj(DamagePattern* pattern)
		{
			FILE* fl = fopen("Pattern.obj", "w");

			std::vector<uint32_t> trc;

			for (uint32_t mesh = 0; mesh < pattern->cellsCount; ++mesh)
			{
				Mesh* m = pattern->cellsMeshes[mesh];
				
				Triangulator  trgl;
				trgl.triangulate(m);

				auto& t = trgl.getBaseMesh();

				for (uint32_t v = 0; v < t.size(); ++v)
				{
					fprintf(fl, "v %f %f %f\n", t[v].a.p.x, t[v].a.p.y, t[v].a.p.z);
					fprintf(fl, "v %f %f %f\n", t[v].b.p.x, t[v].b.p.y, t[v].b.p.z);
					fprintf(fl, "v %f %f %f\n", t[v].c.p.x, t[v].c.p.y, t[v].c.p.z);
				}
				trc.push_back(t.size());
			}

			uint32_t cv = 1;
			for (uint32_t m = 0; m < trc.size(); ++m)
			{
				fprintf(fl, "g %d\n", m);
				for (uint32_t k = 0; k < trc[m]; ++k)
				{

					fprintf(fl, "f %d %d %d \n", cv, cv + 1, cv + 2);
					cv += 3;
				}
			}


			fclose(fl);
		}

	}
}