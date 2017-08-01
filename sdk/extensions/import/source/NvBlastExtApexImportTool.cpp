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
// Copyright (c) 2016-2017 NVIDIA Corporation. All rights reserved.


#include "NvBlastExtApexImportTool.h"

#if NV_VC
#pragma warning(push)
#pragma warning(disable: 4996) // 'fopen' unsafe warning, from NxFileBuffer.h
#endif

#include "PxFoundation.h"

#include "NvBlastIndexFns.h"
#include "NvBlastGlobals.h"
#include "DestructibleAsset.h"
#include "NvBlastExtApexDestruction.h"
#include <PxConvexMesh.h>
#include "PxPhysics.h"
#include "NvBlastExtAuthoringCollisionBuilder.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlastExtAuthoring.h"
#include "NvBlastExtAuthoringBondGenerator.h"

#include <algorithm>
#include <memory>

using namespace nvidia;
using namespace apex;

namespace Nv
{
namespace Blast
{

namespace ApexImporter
{
	/**
		Should be consistent with IntPair in APEX
	*/
	struct IntPair
	{
		void	set(int32_t _i0, int32_t _i1)
		{
			i0 = _i0;
			i1 = _i1;
		}

		int32_t	i0, i1;

		static	int	compare(const void* a, const void* b)
		{
			const int32_t diff0 = ((IntPair*)a)->i0 - ((IntPair*)b)->i0;
			return diff0 ? diff0 : (((IntPair*)a)->i1 - ((IntPair*)b)->i1);
		}
	};

ApexImportTool::ApexImportTool()
	: m_apexDestruction(NULL)
{
}


bool ApexImportTool::isValid()
{
	return m_apexDestruction && m_apexDestruction->isValid();
}


bool ApexImportTool::initialize()
{
	if (isValid())
	{
		return true;
	}
	NVBLAST_LOG_INFO("APEX initialization");
	m_apexDestruction = new ApexDestruction();
	return isValid();
}


bool ApexImportTool::initialize(nvidia::apex::ApexSDK* apexSdk, nvidia::apex::ModuleDestructible* moduleDestructible)
{
	if (isValid())
	{
		return true;
	}
	NVBLAST_LOG_INFO("APEX initialization");
	m_apexDestruction = new ApexDestruction(apexSdk, moduleDestructible);
	return isValid();
}

DestructibleAsset* ApexImportTool::loadAssetFromFile(physx::PxFileBuf* stream)
{
	return m_apexDestruction->loadAsset(stream);
}


bool ApexImportTool::getCollisionGeometry(const nvidia::apex::DestructibleAsset* apexAsset, uint32_t chunkCount, std::vector<uint32_t>& chunkReorderInvMap,
						const std::vector<uint32_t>& apexChunkFlags, std::vector<ExtPxAssetDesc::ChunkDesc>& physicsChunks,
						std::vector<ExtPxAssetDesc::SubchunkDesc>& physicsSubchunks, std::vector<std::vector<CollisionHull*> >& hullsDesc)
{
	physicsChunks.clear();
	physicsChunks.resize(chunkCount);
	// prepare physics asset desc (convexes, transforms)
	std::shared_ptr<ConvexMeshBuilder> collisionBuilder(
		NvBlastExtAuthoringCreateConvexMeshBuilder(m_apexDestruction->cooking(), &m_apexDestruction->apexSDK()->getPhysXSDK()->getPhysicsInsertionCallback()),
		[](ConvexMeshBuilder* cmb) { cmb->release(); });

	int32_t apexHullCount = 0;
	const uint32_t apexChunkCount = apexAsset->getChunkCount();
	for (uint32_t chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex)
	{
		uint32_t apexChunkIndex = chunkReorderInvMap[chunkIndex];
		if (apexChunkIndex < apexChunkCount)
		{
			uint32_t partIndex = apexAsset->getPartIndex(apexChunkIndex);
			apexHullCount += apexAsset->getPartConvexHullCount(partIndex);
		}
	}
	physicsSubchunks.reserve(chunkCount);
	{
		hullsDesc.clear();
		hullsDesc.resize(chunkCount);
		for (uint32_t chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex)
		{
			uint32_t apexChunkIndex = chunkReorderInvMap[chunkIndex];
			if (apexChunkIndex < apexChunkCount)
			{
				uint32_t partIndex = apexAsset->getPartIndex(apexChunkIndex);
				uint32_t partConvexHullCount = apexAsset->getPartConvexHullCount(partIndex);
				NvParameterized::Interface** cxInterfaceArray = apexAsset->getPartConvexHullArray(partIndex);
				physicsChunks[chunkIndex].subchunkCount = partConvexHullCount;
				for (uint32_t hull = 0; hull < partConvexHullCount; ++hull)
				{
					NvParameterized::Handle paramHandle(cxInterfaceArray[hull]);
					int32_t verticesCount = 0;
					paramHandle.getParameter("vertices");
					paramHandle.getArraySize(verticesCount);
					std::vector<PxVec3> vertexData(verticesCount);
					paramHandle.getParamVec3Array(vertexData.data(), verticesCount);
					hullsDesc[chunkIndex].push_back(nullptr);
					hullsDesc[chunkIndex].back() = collisionBuilder.get()->buildCollisionGeometry(verticesCount, vertexData.data());
					PxConvexMesh* convexMesh = collisionBuilder.get()->buildConvexMesh(verticesCount, vertexData.data());
					
					const ExtPxAssetDesc::SubchunkDesc subchunk =
					{
						PxTransform(PxIdentity),
						PxConvexMeshGeometry(convexMesh)
					};
					physicsSubchunks.push_back(subchunk);
				}
				physicsChunks[chunkIndex].subchunks = partConvexHullCount ? (&physicsSubchunks.back() + 1 - partConvexHullCount) : nullptr;

				// static flag set
				physicsChunks[chunkIndex].isStatic = (apexChunkFlags[apexChunkIndex] & (1 << 1)) != 0;
			}
			else
			{
				NVBLAST_LOG_ERROR("Error: chunk index is invalid.");
			}
		}
	}

	// check that vector didn't grow
	if (static_cast<int32_t>(physicsSubchunks.size()) > apexHullCount)
	{
		NVBLAST_LOG_ERROR("Error: sub chunk count seems to be wrong.");
		return false;
	}
	return true;
}

void gatherChunkHullPoints(const DestructibleAsset* apexAsset, std::vector<std::vector<PxVec3> >& hullPoints)
{
	hullPoints.resize(apexAsset->getChunkCount());
	for (uint32_t chunk = 0; chunk < apexAsset->getChunkCount(); ++chunk)
	{
		int32_t part = apexAsset->getPartIndex(chunk);
		NvParameterized::Interface** cxInterfaceArray = apexAsset->getPartConvexHullArray(part);
		for (uint32_t hull = 0; hull < apexAsset->getPartConvexHullCount(part); ++hull)
		{
			NvParameterized::Handle paramHandle(cxInterfaceArray[hull]);
			int32_t verticesCount = 0;
			paramHandle.getParameter("vertices");
			paramHandle.getArraySize(verticesCount);
			uint32_t oldSize = (uint32_t)hullPoints[chunk].size();
			hullPoints[chunk].resize(hullPoints[chunk].size() + verticesCount);
			paramHandle.getParamVec3Array(hullPoints[chunk].data() + oldSize, verticesCount);
		}
	}
}
PxBounds3 gatherChunkTriangles(const DestructibleAsset* apexAsset, std::vector<uint32_t>& chunkTrianglesOffsets, std::vector<Nv::Blast::Triangle>& chunkTriangles, int32_t posBufferIndex, float scale, PxVec3 offset )
{

	PxBounds3 bnd;
	bnd.setEmpty();
	chunkTrianglesOffsets.clear();
	chunkTrianglesOffsets.resize(apexAsset->getChunkCount() + 1);
	chunkTrianglesOffsets[0] = 0;
	for (uint32_t chunkIndex = 0; chunkIndex < apexAsset->getChunkCount(); ++chunkIndex)
	{
		uint32_t part = apexAsset->getPartIndex(chunkIndex);
		const RenderMeshAsset* rAsset = apexAsset->getRenderMeshAsset();
		uint32_t submeshCount = rAsset->getSubmeshCount();
		for (uint32_t submeshIndex = 0; submeshIndex < submeshCount; ++submeshIndex)
		{
			const RenderSubmesh& currentSubmesh = rAsset->getSubmesh(submeshIndex);
			const uint32_t* indexArray = currentSubmesh.getIndexBuffer(part);
			uint32_t indexCount = currentSubmesh.getIndexCount(part);
			const PxVec3* positions = reinterpret_cast<const PxVec3*>(currentSubmesh.getVertexBuffer().getBuffer(posBufferIndex));

			for (uint32_t i = 0; i < indexCount; i += 3)
			{
				Vertex a;
				Vertex b;
				Vertex c;
				bnd.include(positions[indexArray[i]]);
				bnd.include(positions[indexArray[i + 1]]);
				bnd.include(positions[indexArray[i + 2]]);

				a.p = positions[indexArray[i]] - offset;
				b.p = positions[indexArray[i + 1]] - offset;
				c.p = positions[indexArray[i + 2]] - offset;
				a.p *= scale;
				b.p *= scale;
				c.p *= scale;
				chunkTriangles.push_back(Nv::Blast::Triangle(a, b, c));
			}
		}
		chunkTrianglesOffsets[chunkIndex + 1] = chunkTriangles.size();
	}
	return bnd;
}

bool ApexImportTool::importApexAsset(std::vector<uint32_t>& chunkReorderInvMap, const nvidia::apex::DestructibleAsset* apexAsset,
	std::vector<NvBlastChunkDesc>& chunkDescriptors, std::vector<NvBlastBondDesc>& bondDescriptors, std::vector<uint32_t>& apexChunkFlags)
{
	ApexImporterConfig configDesc;
	configDesc.setDefaults();
	return importApexAsset(chunkReorderInvMap, apexAsset, chunkDescriptors, bondDescriptors, apexChunkFlags, configDesc);
}


bool ApexImportTool::importApexAsset(std::vector<uint32_t>& chunkReorderInvMap, const nvidia::apex::DestructibleAsset* apexAsset,
	std::vector<NvBlastChunkDesc>& chunkDescriptors, std::vector<NvBlastBondDesc>& bondDescriptors, std::vector<uint32_t>& apexChunkFlags, const ApexImporterConfig& configDesc)
{
	return importApexAssetInternal(chunkReorderInvMap, apexAsset, chunkDescriptors, bondDescriptors, apexChunkFlags, configDesc);
}


bool ApexImportTool::importApexAssetInternal(std::vector<uint32_t>& chunkReorderInvMap, const nvidia::apex::DestructibleAsset* apexAsset,
	std::vector<NvBlastChunkDesc>& chunkDescriptors, std::vector<NvBlastBondDesc>& bondsDescriptors, std::vector<uint32_t>& apexChunkFlags, const ApexImporterConfig& configDesc)
{

	if (!apexAsset)
	{
		NVBLAST_LOG_ERROR("Error: attempting to import NULL Apex asset.");
		return false;
	}
	
	// Build chunk descriptors for asset //
	const uint32_t apexChunkCount = apexAsset->getChunkCount();
	chunkDescriptors.clear();
	chunkDescriptors.resize(apexChunkCount);
	uint32_t rootChunkIndex = 0;

	for (uint32_t i = 0; i < apexChunkCount; ++i)
	{
		// Use bounds center for centroid
		const PxBounds3 bounds = apexAsset->getChunkActorLocalBounds(i);
		const PxVec3 center = bounds.getCenter();
		memcpy(chunkDescriptors[i].centroid, &center.x, 3 * sizeof(float));

		// Find chunk volume
		uint32_t partIndex = apexAsset->getPartIndex(i);
		uint32_t partConvexHullCount = apexAsset->getPartConvexHullCount(partIndex);
		NvParameterized::Interface** cxInterfaceArray = apexAsset->getPartConvexHullArray(partIndex);
		chunkDescriptors[i].volume = 0.0f;
		for (uint32_t hull = 0; hull < partConvexHullCount; ++hull)
		{
			NvParameterized::Handle paramHandle(cxInterfaceArray[hull]);
			float hullVolume;
			paramHandle.getParameter("volume");
			paramHandle.getParamF32(hullVolume);
			chunkDescriptors[i].volume += hullVolume;
		}

		int32_t parent = apexAsset->getChunkParentIndex(i);
		if (parent == -1)
		{
			rootChunkIndex = i;
			chunkDescriptors[i].parentChunkIndex = UINT32_MAX;
		}
		else
		{
			chunkDescriptors[i].parentChunkIndex = parent;
		}

		chunkDescriptors[i].flags = 0;
		chunkDescriptors[i].userData = i;
	}
	// Get support graph data from Apex asset //

	const NvParameterized::Interface* assetParameterized = apexAsset->getAssetNvParameterized();
	uint32_t maximumSupportDepth = 0;

	NvParameterized::Handle parameterHandle(*assetParameterized);
	parameterHandle.getParameter("supportDepth");
	parameterHandle.getParamU32(maximumSupportDepth);
	std::vector<std::pair<uint32_t, uint32_t> > overlapsBuffer;
	uint32_t overlapsCount = apexAsset->getCachedOverlapCountAtDepth(maximumSupportDepth);
	if (overlapsCount != 0)
	{
		const IntPair* overlap = reinterpret_cast<const IntPair*>(apexAsset->getCachedOverlapsAtDepth(maximumSupportDepth));
		for (uint32_t i = 0; i < overlapsCount; ++i)
		{
			chunkDescriptors[overlap[i].i0].flags = NvBlastChunkDesc::SupportFlag;
			chunkDescriptors[overlap[i].i1].flags = NvBlastChunkDesc::SupportFlag;
			overlapsBuffer.push_back(std::make_pair(overlap[i].i0, overlap[i].i1));
		}
	}

	// Format all connections as (chunk with lower index) -> (chunk with higher index) // 

	for (uint32_t i = 0; i < overlapsBuffer.size(); ++i)
	{
		if (overlapsBuffer[i].first > overlapsBuffer[i].second)
		{
			std::swap(overlapsBuffer[i].first, overlapsBuffer[i].second);
		}
	}

	// Unique all connections //
	std::sort(overlapsBuffer.begin(), overlapsBuffer.end());
	overlapsBuffer.resize(std::unique(overlapsBuffer.begin(), overlapsBuffer.end()) - overlapsBuffer.begin());

	// Build bond descriptors (acquire area, normal, centroid)
	bondsDescriptors.clear();
	bondsDescriptors.resize(overlapsBuffer.size());

	std::shared_ptr<Nv::Blast::BlastBondGenerator> bondGenTool(
		NvBlastExtAuthoringCreateBondGenerator(GetApexSDK()->getCookingInterface(), &GetApexSDK()->getPhysXSDK()->getPhysicsInsertionCallback()),
		[](Nv::Blast::BlastBondGenerator* bg) {bg->release(); });

	std::vector<uint32_t> chunkTrianglesOffsets;
	std::vector<Nv::Blast::Triangle> chunkTriangles;
	
	PxBounds3 bnds = apexAsset->getRenderMeshAsset()->getBounds();
	PxVec3 offset = bnds.getCenter();
	float scale = 1.0f / PxMax(PxAbs(bnds.getExtents(0)), PxMax(PxAbs(bnds.getExtents(1)), PxAbs(bnds.getExtents(2))));

	bnds = gatherChunkTriangles(apexAsset, chunkTrianglesOffsets, chunkTriangles, 0, scale, offset);


	BondGenerationConfig cf;
	cf.bondMode = BondGenerationConfig::AVERAGE;
	if (configDesc.infSearchMode == configDesc.EXACT)
	{
		cf.bondMode = BondGenerationConfig::EXACT;
	}
	NvBlastBondDesc* bondsDesc;
	std::vector<uint32_t> overlapsA, overlapsB;
	for (auto it : overlapsBuffer)
	{
		overlapsA.push_back(it.first);
		overlapsB.push_back(it.second);
	}
	bondGenTool.get()->createBondBetweenMeshes(chunkTrianglesOffsets.size() - 1, chunkTrianglesOffsets.data(), chunkTriangles.data(), 
		overlapsBuffer.size(), overlapsA.data(), overlapsB.data(), bondsDesc, cf);
	memcpy(bondsDescriptors.data(), bondsDesc, sizeof(NvBlastBondDesc) * bondsDescriptors.size());
	delete[] bondsDesc;


	float inverScale = 1.0f / scale;

	for (uint32_t i = 0; i < bondsDescriptors.size(); ++i)
	{
		bondsDescriptors[i].bond.area *= inverScale * inverScale;
		bondsDescriptors[i].bond.centroid[0] *= inverScale;
		bondsDescriptors[i].bond.centroid[1] *= inverScale;
		bondsDescriptors[i].bond.centroid[2] *= inverScale;

		bondsDescriptors[i].bond.centroid[0] += offset.x;
		bondsDescriptors[i].bond.centroid[1] += offset.y;
		bondsDescriptors[i].bond.centroid[2] += offset.z;

	}
	
	/// Delete all bonds with zero area ///
	for (uint32_t i = 0; i < bondsDescriptors.size(); ++i)
	{
		if (bondsDescriptors[i].bond.area == 0)
		{
			bondsDescriptors[i].chunkIndices[0] = bondsDescriptors.back().chunkIndices[0];
			bondsDescriptors[i].chunkIndices[1] = bondsDescriptors.back().chunkIndices[1];
			bondsDescriptors[i].bond = bondsDescriptors.back().bond;
			bondsDescriptors.pop_back();
			--i;
		}
	}



	apexChunkFlags.clear();
	apexChunkFlags.resize(chunkDescriptors.size());
	// externally supported chunks
	{
		for (uint32_t i = 0; i < chunkDescriptors.size(); i++)
		{
			uint32_t chunkID = i;
			const NvParameterized::Interface* assetInterface = apexAsset->getAssetNvParameterized();
			NvParameterized::Handle chunksHandle(*assetInterface, "chunks");
			chunksHandle.set(chunkID);
			NvParameterized::Handle flagsHandle(*assetInterface);
			chunksHandle.getChildHandle(assetInterface, "flags", flagsHandle);
			uint32_t flags;
			flagsHandle.getParamU32(flags);

			apexChunkFlags[chunkID] = flags;

			// world support flag
			if (flags & (1 << 0))
			{
				NvBlastBondDesc bond;
				bond.chunkIndices[0] = i;
				bond.chunkIndices[1] = UINT32_MAX;	// invalid index for "world"
				bond.bond.area = 0.1f; // ???
				PxVec3 center = apexAsset->getChunkActorLocalBounds(chunkID).getCenter();
				memcpy(&bond.bond.centroid, &center.x, sizeof(PxVec3));
				PxVec3 normal = PxVec3(0, 0, 1);
				memcpy(&bond.bond.normal, &normal.x, sizeof(PxVec3));
				bondsDescriptors.push_back(bond);
			}
		}
	}

	const uint32_t chunkCount = static_cast<uint32_t>(chunkDescriptors.size());
	const uint32_t bondCount = static_cast<uint32_t>(bondsDescriptors.size());
	std::vector<uint32_t> chunkReorderMap(chunkCount);
	std::vector<NvBlastChunkDesc> scratch(chunkCount);
	NvBlastEnsureAssetExactSupportCoverage(chunkDescriptors.data(), chunkCount, scratch.data(), logLL);
	NvBlastBuildAssetDescChunkReorderMap(chunkReorderMap.data(), chunkDescriptors.data(), chunkCount, scratch.data(), logLL);
	NvBlastApplyAssetDescChunkReorderMapInPlace(chunkDescriptors.data(), chunkCount, bondsDescriptors.data(), bondCount, chunkReorderMap.data(), true, scratch.data(), logLL);
	chunkReorderInvMap.resize(chunkReorderMap.size());
	Nv::Blast::invertMap(chunkReorderInvMap.data(), chunkReorderMap.data(), static_cast<uint32_t>(chunkReorderMap.size()));
	return true;
}


bool ApexImportTool::saveAsset(const NvBlastAsset* asset, PxFileBuf* stream)
{
	if (!asset)
	{
		NVBLAST_LOG_ERROR("Error: attempting to serialize NULL asset.");
		return false;
	}
	if (!stream)
	{
		NVBLAST_LOG_ERROR("Error: bad output stream.");
		return false;
	}
	const void* assetData = asset;
	uint32_t assetDataSize = NvBlastAssetGetSize(asset, logLL);
	stream->write(assetData, assetDataSize);
	stream->close();
	NVBLAST_LOG_INFO("Saving finished.");
	return true;
}


ApexImportTool::~ApexImportTool()
{
	delete m_apexDestruction;
}

} // namespace ApexImporter

} // namespace Blast
} // namespace Nv
