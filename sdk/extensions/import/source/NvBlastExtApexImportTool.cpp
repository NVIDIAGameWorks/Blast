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


#include "NvBlastExtApexImportTool.h"

#if NV_VC
#pragma warning(push)
#pragma warning(disable : 4996)  // 'fopen' unsafe warning, from NxFileBuffer.h
#endif

#include "PxFoundation.h"

#include "NvBlastIndexFns.h"
#include "NvBlastGlobals.h"
#include <NvBlastExtExporter.h>
#include <PxConvexMesh.h>
#include "PxPhysics.h"
#include "NvBlastExtAuthoringConvexMeshBuilder.h"
#include "NvBlastExtPxManager.h"
#include "NvBlastExtPxCollisionBuilder.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlastExtAuthoring.h"
#include "NvBlastExtAuthoringBondGenerator.h"
#include <nvparameterized\NvParameterized.h>
#include <nvparameterized\NvParamUtils.h>
#include <DestructibleAssetParameters.h>
#include <RenderMeshAssetParameters.h>
#include <VertexBufferParameters.h>
#include <VertexFormatParameters.h>
#include <SubmeshParameters.h>
#include <CachedOverlaps.h>
#include "PsFastXml.h"
#include "PsFileBuffer.h"
#include <BufferF32x3.h>
#include <BufferF32x2.h>
#include <algorithm>
#include <sstream>
#include <memory>
#include <map>


#include <NvDefaultTraits.h>
#include <NvSerializerInternal.h>
#include <PsMutex.h>
#include <ModuleDestructibleRegistration.h>
#include <ModuleDestructibleLegacyRegistration.h>
#include <ModuleCommonRegistration.h>
#include <ModuleCommonLegacyRegistration.h>
#include <ModuleFrameworkRegistration.h>
#include <ModuleFrameworkLegacyRegistration.h>
#include <PxPhysicsAPI.h>

#include "NvBlastPxCallbacks.h"
#include "NvBlastPxSharedHelpers.h"

using namespace nvidia;
using namespace physx;
using namespace apex;

using nvidia::destructible::DestructibleAssetParameters;

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
	void set(int32_t _i0, int32_t _i1)
	{
		i0 = _i0;
		i1 = _i1;
	}

	int32_t i0, i1;

	static int compare(const void* a, const void* b)
	{
		const int32_t diff0 = ((IntPair*)a)->i0 - ((IntPair*)b)->i0;
		return diff0 ? diff0 : (((IntPair*)a)->i1 - ((IntPair*)b)->i1);
	}
};

bool ApexImportTool::loadAssetFromFile(physx::PxFileBuf* stream, NvParameterized::Serializer::DeserializedData& data)
{
	if (stream && stream->isOpen())
	{
		NvParameterized::Serializer::SerializeType serType = NvParameterized::Serializer::peekSerializeType(*stream);
		NvParameterized::Serializer::ErrorType serError;

		NvParameterized::Traits* traits =
		    new NvParameterized::DefaultTraits(NvParameterized::DefaultTraits::BehaviourFlags::DEFAULT_POLICY);

		nvidia::destructible::ModuleDestructibleRegistration::invokeRegistration(traits);
		ModuleDestructibleLegacyRegistration::invokeRegistration(traits);
		ModuleCommonRegistration::invokeRegistration(traits);
		ModuleCommonLegacyRegistration::invokeRegistration(traits);
		ModuleFrameworkLegacyRegistration::invokeRegistration(traits);
		ModuleFrameworkRegistration::invokeRegistration(traits);
		NvParameterized::Serializer* ser = NvParameterized::internalCreateSerializer(serType, traits);

		PX_ASSERT(ser);

		serError = ser->deserialize(*stream, data);

		if (serError == NvParameterized::Serializer::ERROR_NONE && data.size() == 1)
		{
			NvParameterized::Interface* params = data[0];
			if (!physx::shdfnd::strcmp(params->className(), "DestructibleAssetParameters"))
			{
				return true;
			}
			else
			{
				NVBLAST_LOG_ERROR("Error: deserialized data is not an APEX Destructible\n");
			}
		}
		else
		{
			NVBLAST_LOG_ERROR("Error: failed to deserialize\n");
		}
		ser->release();
	}
	return false;
}

bool ApexImportTool::isValid()
{
	return m_Foundation && m_PhysxSDK && m_Cooking;
}

enum ChunkFlags
{
	SupportChunk             = (1 << 0),
	UnfracturableChunk       = (1 << 1),
	DescendantUnfractureable = (1 << 2),
	UndamageableChunk        = (1 << 3),
	UncrumbleableChunk       = (1 << 4),
	RuntimeFracturableChunk  = (1 << 5),
	Instanced                = (1 << 8),
};

uint32_t getPartIndex(const DestructibleAssetParameters* prm, uint32_t id)
{
	auto& sch = prm->chunks.buf[id];

	return (sch.flags & ChunkFlags::Instanced) == 0 ? sch.meshPartIndex :
	                                                  prm->chunkInstanceInfo.buf[sch.meshPartIndex].partIndex;
}

ApexImportTool::ApexImportTool()
{
	m_Foundation =
	    PxCreateFoundation(PX_FOUNDATION_VERSION, NvBlastGetPxAllocatorCallback(), NvBlastGetPxErrorCallback());
	if (!m_Foundation)
	{
		NVBLAST_LOG_ERROR("Error: failed to create Foundation\n");
		return;
	}
	physx::PxTolerancesScale scale;
	m_PhysxSDK = PxCreatePhysics(PX_PHYSICS_VERSION, *m_Foundation, scale, true);
	if (!m_PhysxSDK)
	{
		NVBLAST_LOG_ERROR("Error: failed to create PhysX\n");

		return;
	}

	physx::PxCookingParams cookingParams(scale);
	cookingParams.buildGPUData = true;
	m_Cooking                  = PxCreateCooking(PX_PHYSICS_VERSION, m_PhysxSDK->getFoundation(), cookingParams);
	if (!m_Cooking)
	{
		NVBLAST_LOG_ERROR("Error: failed to create PhysX Cooking\n");
		return;
	}

	m_collisionBuilder = ExtPxManager::createCollisionBuilder(*m_PhysxSDK, *m_Cooking);
}


bool ApexImportTool::getCollisionGeometry(const NvParameterized::Interface* assetPrm, uint32_t chunkCount,
                                          std::vector<uint32_t>& chunkReorderInvMap,
                                          const std::vector<uint32_t>& apexChunkFlags,
                                          std::vector<ExtPxAssetDesc::ChunkDesc>& physicsChunks,
                                          std::vector<ExtPxAssetDesc::SubchunkDesc>& physicsSubchunks,
                                          std::vector<std::vector<CollisionHull*> >& hullsDesc)
{
	physicsChunks.clear();
	physicsChunks.resize(chunkCount);
	// prepare physics asset desc (convexes, transforms)

	const DestructibleAssetParameters* params = static_cast<const DestructibleAssetParameters*>(assetPrm);

	int32_t apexHullCount         = 0;
	const uint32_t apexChunkCount = params->chunks.arraySizes[0];

	for (uint32_t chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex)
	{
		uint32_t apexChunkIndex = chunkReorderInvMap[chunkIndex];
		if (apexChunkIndex < apexChunkCount)
		{
			uint32_t partIndex           = getPartIndex(params, apexChunkIndex);
			uint32_t partConvexHullCount = params->chunkConvexHullStartIndices.buf[partIndex + 1] -
			                               params->chunkConvexHullStartIndices.buf[partIndex];
			apexHullCount += partConvexHullCount;
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
				uint32_t partIndex           = getPartIndex(params, apexChunkIndex);
				uint32_t partConvexHullCount = params->chunkConvexHullStartIndices.buf[partIndex + 1] -
				                               params->chunkConvexHullStartIndices.buf[partIndex];
				NvParameterized::Interface** cxInterfaceArray =
				    params->chunkConvexHulls.buf + params->chunkConvexHullStartIndices.buf[partIndex];
				physicsChunks[chunkIndex].subchunkCount = partConvexHullCount;
				for (uint32_t hull = 0; hull < partConvexHullCount; ++hull)
				{
					NvParameterized::Handle paramHandle(cxInterfaceArray[hull]);
					int32_t verticesCount = 0;
					paramHandle.getParameter("vertices");
					paramHandle.getArraySize(verticesCount);
					std::vector<NvcVec3> vertexData(verticesCount);
					paramHandle.getParamVec3Array(toPxShared(vertexData.data()), verticesCount);
					hullsDesc[chunkIndex].push_back(nullptr);
					hullsDesc[chunkIndex].back() =
					    m_collisionBuilder->buildCollisionGeometry(verticesCount, vertexData.data());
					auto collisionHull = m_collisionBuilder->buildCollisionGeometry(verticesCount, vertexData.data());
					auto convexMesh    = m_collisionBuilder->buildConvexMesh(*collisionHull);
					m_collisionBuilder->releaseCollisionHull(collisionHull);

					const ExtPxAssetDesc::SubchunkDesc subchunk = { PxTransform(PxIdentity),
						                                            PxConvexMeshGeometry(convexMesh) };
					physicsSubchunks.push_back(subchunk);
				}
				physicsChunks[chunkIndex].subchunks =
				    partConvexHullCount ? (&physicsSubchunks.back() + 1 - partConvexHullCount) : nullptr;

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

PxBounds3
gatherChunkTriangles(std::vector<uint32_t>& chunkToPartMp, const nvidia::apex::RenderMeshAssetParameters* rmAsset,
                     std::vector<uint32_t>& chunkTrianglesOffsets, std::vector<Nv::Blast::Triangle>& chunkTriangles,
                     int32_t posBufferIndex, float scale, PxVec3 offset)
{

	PxBounds3 bnd;
	bnd.setEmpty();
	chunkTrianglesOffsets.clear();
	uint32_t chunkCount = chunkToPartMp.size();
	chunkTrianglesOffsets.resize(chunkCount + 1);
	chunkTrianglesOffsets[0] = 0;
	for (uint32_t chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex)
	{
		uint32_t part         = chunkToPartMp[chunkIndex];
		uint32_t submeshCount = rmAsset->submeshes.arraySizes[0];
		for (uint32_t submeshIndex = 0; submeshIndex < submeshCount; ++submeshIndex)
		{
			nvidia::apex::SubmeshParameters* submeshPrm =
			    static_cast<nvidia::apex::SubmeshParameters*>(rmAsset->submeshes.buf[submeshIndex]);

			const uint32_t* indexArray = submeshPrm->indexBuffer.buf + submeshPrm->indexPartition.buf[part];
			uint32_t indexCount = submeshPrm->indexPartition.buf[part + 1] - submeshPrm->indexPartition.buf[part];

			nvidia::apex::VertexBufferParameters* vbuf =
			    static_cast<nvidia::apex::VertexBufferParameters*>(submeshPrm->vertexBuffer);

			nvidia::apex::BufferF32x3* pbuf = static_cast<nvidia::apex::BufferF32x3*>(vbuf->buffers.buf[posBufferIndex]);

			const PxVec3* positions = reinterpret_cast<const PxVec3*>(pbuf->data.buf);

			for (uint32_t i = 0; i < indexCount; i += 3)
			{
				Vertex a;
				Vertex b;
				Vertex c;
				bnd.include(positions[indexArray[i]]);
				bnd.include(positions[indexArray[i + 1]]);
				bnd.include(positions[indexArray[i + 2]]);

				a.p = fromPxShared(positions[indexArray[i]] - offset) * scale;
				b.p = fromPxShared(positions[indexArray[i + 1]] - offset) * scale;
				c.p = fromPxShared(positions[indexArray[i + 2]] - offset) * scale;
				chunkTriangles.push_back({a, b, c});
			}
		}
		chunkTrianglesOffsets[chunkIndex + 1] = chunkTriangles.size();
	}
	return bnd;
}

bool ApexImportTool::importApexAsset(std::vector<uint32_t>& chunkReorderInvMap, NvParameterized::Interface* assetNvIfc,
                                     std::vector<NvBlastChunkDesc>& chunkDescriptors,
                                     std::vector<NvBlastBondDesc>& bondDescriptors, std::vector<uint32_t>& apexChunkFlags)
{
	ApexImporterConfig configDesc;
	configDesc.setDefaults();
	return importApexAsset(chunkReorderInvMap, assetNvIfc, chunkDescriptors, bondDescriptors, apexChunkFlags, configDesc);
}


bool ApexImportTool::importApexAsset(std::vector<uint32_t>& chunkReorderInvMap, NvParameterized::Interface* assetNvIfc,
                                     std::vector<NvBlastChunkDesc>& chunkDescriptors,
                                     std::vector<NvBlastBondDesc>& bondDescriptors,
                                     std::vector<uint32_t>& apexChunkFlags, const ApexImporterConfig& configDesc)
{
	return importApexAssetInternal(chunkReorderInvMap, assetNvIfc, chunkDescriptors, bondDescriptors, apexChunkFlags,
	                               configDesc);
}


bool ApexImportTool::importApexAssetInternal(std::vector<uint32_t>& chunkReorderInvMap,
                                             NvParameterized::Interface* assetNvIfc,
                                             std::vector<NvBlastChunkDesc>& chunkDescriptors,
                                             std::vector<NvBlastBondDesc>& bondsDescriptors,
                                             std::vector<uint32_t>& apexChunkFlags, const ApexImporterConfig& configDesc)
{
	if (!assetNvIfc)
	{
		NVBLAST_LOG_ERROR("Error: attempting to import NULL Apex asset.");
		return false;
	}
	DestructibleAssetParameters* params = static_cast<nvidia::destructible::DestructibleAssetParameters*>(assetNvIfc);

	int32_t apexChunkCount  = params->chunks.arraySizes[0];
	uint32_t rootChunkIndex = 0;

	std::vector<uint32_t> chunkToPartMapping(apexChunkCount);

	chunkDescriptors.resize(apexChunkCount);

	nvidia::apex::RenderMeshAssetParameters* rmParam =
	    static_cast<nvidia::apex::RenderMeshAssetParameters*>(params->renderMeshAsset);

	std::vector<PxBounds3> perChunkBounds(apexChunkCount);
	PxBounds3 allRmBound;
	allRmBound.setEmpty();

	for (uint32_t i = 0; i < (uint32_t)apexChunkCount; ++i)
	{
		// Use bounds center for centroid
		uint32_t partIndex     = getPartIndex(params, i);
		chunkToPartMapping[i]  = partIndex;
		const PxBounds3 bounds = rmParam->partBounds.buf[partIndex];

		perChunkBounds[i] = bounds;
		allRmBound.include(bounds);

		const PxVec3 center = bounds.getCenter();
		memcpy(chunkDescriptors[i].centroid, &center.x, 3 * sizeof(float));

		// Find chunk volume
		uint32_t partConvexHullCount =
		    params->chunkConvexHullStartIndices.buf[partIndex + 1] - params->chunkConvexHullStartIndices.buf[partIndex];
		NvParameterized::Interface** cxInterfaceArray =
		    params->chunkConvexHulls.buf + params->chunkConvexHullStartIndices.buf[partIndex];
		chunkDescriptors[i].volume = 0.0f;
		for (uint32_t hull = 0; hull < partConvexHullCount; ++hull)
		{
			NvParameterized::Handle paramHandle(cxInterfaceArray[hull]);
			float hullVolume;
			paramHandle.getParameter("volume");
			paramHandle.getParamF32(hullVolume);
			chunkDescriptors[i].volume += hullVolume;
		}

		int16_t parent = params->chunks.buf[i].parentIndex;
		if (parent == -1)
		{
			rootChunkIndex                       = i;
			chunkDescriptors[i].parentChunkIndex = UINT32_MAX;
		}
		else
		{
			chunkDescriptors[i].parentChunkIndex = parent;
		}

		chunkDescriptors[i].flags    = 0;
		chunkDescriptors[i].userData = i;
	}
	// Get support graph data from Apex asset //

	const NvParameterized::Interface* assetParameterized = assetNvIfc;
	uint32_t maximumSupportDepth                         = 0;

	NvParameterized::Handle parameterHandle(*assetParameterized);
	parameterHandle.getParameter("supportDepth");
	parameterHandle.getParamU32(maximumSupportDepth);
	std::vector<std::pair<uint32_t, uint32_t> > overlapsBuffer;
	nvidia::destructible::CachedOverlaps* overlapsArray =
	    static_cast<nvidia::destructible::CachedOverlaps*>(params->overlapsAtDepth.buf[maximumSupportDepth]);
	uint32_t overlapsCount = overlapsArray->overlaps.arraySizes[0];
	if (overlapsCount != 0)
	{
		for (uint32_t i = 0; i < overlapsCount; ++i)
		{
			uint32_t ov0 = overlapsArray->overlaps.buf[i].i0;
			uint32_t ov1 = overlapsArray->overlaps.buf[i].i1;

			chunkDescriptors[ov0].flags = NvBlastChunkDesc::SupportFlag;
			chunkDescriptors[ov1].flags = NvBlastChunkDesc::SupportFlag;
			overlapsBuffer.push_back(std::make_pair(ov0, ov1));
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
	    NvBlastExtAuthoringCreateBondGenerator(m_collisionBuilder),
	    [](Nv::Blast::BlastBondGenerator* bg) { bg->release(); });

	std::vector<uint32_t> chunkTrianglesOffsets;
	std::vector<Nv::Blast::Triangle> chunkTriangles;

	PxBounds3 bnds = allRmBound;
	PxVec3 offset  = bnds.getCenter();
	float scale = 1.0f / PxMax(PxAbs(bnds.getExtents(0)), PxMax(PxAbs(bnds.getExtents(1)), PxAbs(bnds.getExtents(2))));

	bnds = gatherChunkTriangles(chunkToPartMapping, rmParam, chunkTrianglesOffsets, chunkTriangles, 0, scale, offset);


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
	bondGenTool.get()->createBondBetweenMeshes(chunkTrianglesOffsets.size() - 1, chunkTrianglesOffsets.data(),
	                                           chunkTriangles.data(), overlapsBuffer.size(), overlapsA.data(),
	                                           overlapsB.data(), bondsDesc, cf);
	memcpy(bondsDescriptors.data(), bondsDesc, sizeof(NvBlastBondDesc) * bondsDescriptors.size());
	NVBLAST_FREE(bondsDesc);

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
			bondsDescriptors[i].bond            = bondsDescriptors.back().bond;
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
			uint32_t chunkID                                 = i;
			const NvParameterized::Interface* assetInterface = assetNvIfc;
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
				bond.chunkIndices[1] = UINT32_MAX;  // invalid index for "world"
				bond.bond.area       = 0.1f;        // ???
				PxVec3 center        = perChunkBounds[i].getCenter();
				memcpy(&bond.bond.centroid, &center.x, sizeof(PxVec3));
				PxVec3 normal = PxVec3(0, 0, 1);
				memcpy(&bond.bond.normal, &normal.x, sizeof(PxVec3));
				bondsDescriptors.push_back(bond);
			}
		}
	}

	const uint32_t chunkCount = static_cast<uint32_t>(chunkDescriptors.size());
	const uint32_t bondCount  = static_cast<uint32_t>(bondsDescriptors.size());
	std::vector<uint32_t> chunkReorderMap(chunkCount);
	std::vector<NvBlastChunkDesc> scratch(chunkCount);
	NvBlastEnsureAssetExactSupportCoverage(chunkDescriptors.data(), chunkCount, scratch.data(), logLL);
	NvBlastBuildAssetDescChunkReorderMap(chunkReorderMap.data(), chunkDescriptors.data(), chunkCount, scratch.data(),
	                                     logLL);
	NvBlastApplyAssetDescChunkReorderMapInPlace(chunkDescriptors.data(), chunkCount, bondsDescriptors.data(), bondCount,
	                                            chunkReorderMap.data(), true, scratch.data(), logLL);
	chunkReorderInvMap.resize(chunkReorderMap.size());
	Nv::Blast::invertMap(chunkReorderInvMap.data(), chunkReorderMap.data(),
	                     static_cast<uint32_t>(chunkReorderMap.size()));
	return true;
}

const float VEC_EPS = 1e-4f;

class MaterialXmlParser : public physx::shdfnd::FastXml::Callback
{
  public:
	std::string textureFile;

  protected:
	// encountered a comment in the XML
	virtual bool processComment(const char* /*comment*/)
	{
		return true;
	}

	virtual bool processClose(const char* /*element*/, unsigned int /*depth*/, bool& /*isError*/)
	{
		return true;
	}

	// return true to continue processing the XML document, false to skip.
	virtual bool processElement(const char* elementName,  // name of the element
	                            const char* elementData,  // element data, null if none
	                            const physx::shdfnd::FastXml::AttributePairs& attr, int /*lineno*/)  // line number in
	                                                                                                 // the source XML
	                                                                                                 // file
	{
		PX_UNUSED(attr);
		if (::strcmp(elementName, "sampler2D") == 0)
		{
			int nameIndex = -1;
			for (int i = 0; i < attr.getNbAttr(); i += 2)
			{
				if (::strcmp(attr.getKey(i), "name") == 0)
				{
					nameIndex = i;
					break;
				}
			}

			if (::strcmp(attr.getValue(nameIndex), "diffuseTexture") == 0)
			{
				textureFile = elementData;
			}
		}

		return true;
	}
};

class PxInputDataFromPxFileBuf : public physx::PxInputData
{
  public:
	PxInputDataFromPxFileBuf(physx::PxFileBuf& fileBuf) : mFileBuf(fileBuf) {}

	// physx::PxInputData interface
	virtual uint32_t getLength() const
	{
		return mFileBuf.getFileLength();
	}

	virtual void seek(uint32_t offset)
	{
		mFileBuf.seekRead(offset);
	}

	virtual uint32_t tell() const
	{
		return mFileBuf.tellRead();
	}

	// physx::PxInputStream interface
	virtual uint32_t read(void* dest, uint32_t count)
	{
		return mFileBuf.read(dest, count);
	}

	PX_NOCOPY(PxInputDataFromPxFileBuf)
  private:
	physx::PxFileBuf& mFileBuf;
};


std::string getTextureFromMaterial(const char* materialPath)
{
	PsFileBuffer fileBuffer(materialPath, general_PxIOStream2::PxFileBuf::OPEN_READ_ONLY);
	PxInputDataFromPxFileBuf inputData(fileBuffer);
	MaterialXmlParser parser;
	physx::shdfnd::FastXml* xml = physx::shdfnd::createFastXml(&parser);
	xml->processXml(inputData, false);

	xml->release();

	// trim folders
	std::string textureFile = parser.textureFile.substr(parser.textureFile.find_last_of("/\\") + 1);

	return textureFile;
}

#define MAX_PATH_LEN 260

bool ApexImportTool::importRendermesh(const std::vector<uint32_t>& chunkReorderInvMap,
                                      const NvParameterized::Interface* assetNvIfc, ExporterMeshData* outputData,
                                      const char* materialsDir)
{
	const nvidia::destructible::DestructibleAssetParameters* dasset =
	    static_cast<const nvidia::destructible::DestructibleAssetParameters*>(assetNvIfc);
	const nvidia::apex::RenderMeshAssetParameters* rmAsset =
	    static_cast<const nvidia::apex::RenderMeshAssetParameters*>(dasset->renderMeshAsset);


	outputData->submeshCount = rmAsset->submeshes.arraySizes[0];
	outputData->submeshMats  = new Material[outputData->submeshCount];
	std::vector<Material> materialArray(outputData->submeshCount);
	std::vector<std::string> materialPathes;
	materialPathes.reserve(outputData->submeshCount);
	// gather materials
	{
		for (uint32_t submeshIndex = 0; submeshIndex < outputData->submeshCount; ++submeshIndex)
		{
			const char* materialName = rmAsset->materialNames.buf[submeshIndex].buf;
			if (materialsDir != nullptr)
			{
				std::ostringstream materialPath;
				materialPath << materialsDir << "\\" << materialName;
				std::string texturePath = getTextureFromMaterial(materialPath.str().c_str());
				int32_t bfs             = texturePath.length();
				char* texPath           = new char[bfs + 1];
				char* matName           = new char[bfs + 1];
				memset(texPath, 0, sizeof(char) * (bfs + 1));
				memset(matName, 0, sizeof(char) * (bfs + 1));
				memcpy(texPath, texturePath.data(), sizeof(char) * bfs);
				memcpy(matName, texturePath.data(), sizeof(char) * bfs);
				outputData->submeshMats[submeshIndex].diffuse_tex = texPath;
				outputData->submeshMats[submeshIndex].name        = matName;
			}
			else
			{
				int32_t bfs   = strnlen(materialName, MAX_PATH_LEN);
				char* texPath = new char[bfs];
				char* matName = new char[bfs];
				memset(texPath, 0, sizeof(char) * (bfs + 1));
				memset(matName, 0, sizeof(char) * (bfs + 1));
				memcpy(texPath, materialName, sizeof(char) * bfs);
				memcpy(matName, materialName, sizeof(char) * bfs);
				outputData->submeshMats[submeshIndex].diffuse_tex = texPath;
				outputData->submeshMats[submeshIndex].name        = matName;
			}
		}
	}
	struct vc3Comp
	{
		bool operator()(const PxVec3& a, const PxVec3& b) const
		{
			if (a.x + VEC_EPS < b.x)
				return true;
			if (a.x - VEC_EPS > b.x)
				return false;
			if (a.y + VEC_EPS < b.y)
				return true;
			if (a.y - VEC_EPS > b.y)
				return false;
			if (a.z + VEC_EPS < b.z)
				return true;
			return false;
		}
	};
	struct vc2Comp
	{
		bool operator()(const PxVec2& a, const PxVec2& b) const
		{
			if (a.x + VEC_EPS < b.x)
				return true;
			if (a.x - VEC_EPS > b.x)
				return false;
			if (a.y + VEC_EPS < b.y)
				return true;
			return false;
		}
	};

	std::vector<PxVec3> compressedPositions;
	std::vector<PxVec3> compressedNormals;
	std::vector<PxVec2> compressedTextures;

	std::vector<uint32_t> positionsMapping;
	std::vector<uint32_t> normalsMapping;
	std::vector<uint32_t> texturesMapping;

	std::map<PxVec3, uint32_t, vc3Comp> posMap;
	std::map<PxVec3, uint32_t, vc3Comp> normMap;
	std::map<PxVec2, uint32_t, vc2Comp> texMap;


	// gather data for export
	{
		for (uint32_t submeshIndex = 0; submeshIndex < outputData->submeshCount; ++submeshIndex)
		{
			nvidia::apex::SubmeshParameters* currentSubmesh =
			    static_cast<nvidia::apex::SubmeshParameters*>(rmAsset->submeshes.buf[submeshIndex]);
			nvidia::apex::VertexBufferParameters* vbuf =
			    static_cast<nvidia::apex::VertexBufferParameters*>(currentSubmesh->vertexBuffer);
			nvidia::apex::VertexFormatParameters* vbufFormat =
			    static_cast<nvidia::apex::VertexFormatParameters*>(vbuf->vertexFormat);
			uint32_t indexCount = vbuf->vertexCount;
			// Find position buffer index
			int32_t vbufIds[3];  // 0 - pos, 1 - normals, 2 - t-coord
			vbufIds[0] = vbufIds[1] = vbufIds[2] = -1;
			{
				for (int32_t bid = 0; bid < vbufFormat->bufferFormats.arraySizes[0]; ++bid)
				{
					if (vbufFormat->bufferFormats.buf[bid].semantic == RenderVertexSemantic::POSITION)
					{
						vbufIds[0] = bid;
					}
					if (vbufFormat->bufferFormats.buf[bid].semantic == RenderVertexSemantic::NORMAL)
					{
						vbufIds[1] = bid;
					}
					if (vbufFormat->bufferFormats.buf[bid].semantic == RenderVertexSemantic::TEXCOORD0)
					{
						vbufIds[2] = bid;
					}
				}
			}
			if (vbufIds[0] != -1)
			{
				BufferF32x3* pbuf        = static_cast<BufferF32x3*>(vbuf->buffers.buf[vbufIds[0]]);
				const PxVec3* posistions = pbuf->data.buf;
				uint32_t oldSize         = (uint32_t)positionsMapping.size();

				positionsMapping.resize(oldSize + indexCount);
				for (uint32_t i = 0; i < indexCount; ++i)
				{
					auto it = posMap.find(posistions[i]);
					if (it == posMap.end())
					{
						posMap[posistions[i]]         = (uint32_t)compressedPositions.size();
						positionsMapping[oldSize + i] = (uint32_t)compressedPositions.size();
						compressedPositions.push_back(posistions[i]);
					}
					else
					{
						positionsMapping[oldSize + i] = it->second;
					}
				}
			}

			if (vbufIds[1] != -1)
			{
				BufferF32x3* pbuf     = static_cast<BufferF32x3*>(vbuf->buffers.buf[vbufIds[1]]);
				const PxVec3* normals = pbuf->data.buf;
				uint32_t oldSize      = (uint32_t)normalsMapping.size();
				normalsMapping.resize(oldSize + indexCount);
				for (uint32_t i = 0; i < indexCount; ++i)
				{
					auto it = normMap.find(normals[i]);
					if (it == normMap.end())
					{
						normMap[normals[i]]         = (uint32_t)compressedNormals.size();
						normalsMapping[oldSize + i] = (uint32_t)compressedNormals.size();
						compressedNormals.push_back(normals[i]);
					}
					else
					{
						normalsMapping[oldSize + i] = it->second;
					}
				}
			}
			if (vbufIds[2] != -1)
			{
				BufferF32x2* pbuf      = static_cast<BufferF32x2*>(vbuf->buffers.buf[vbufIds[2]]);
				const PxVec2* texCoord = reinterpret_cast<PxVec2*>(pbuf->data.buf);
				uint32_t oldSize       = (uint32_t)texturesMapping.size();
				texturesMapping.resize(oldSize + indexCount);
				for (uint32_t i = 0; i < indexCount; ++i)
				{
					auto it = texMap.find(texCoord[i]);
					if (it == texMap.end())
					{
						texMap[texCoord[i]]          = (uint32_t)compressedTextures.size();
						texturesMapping[oldSize + i] = (uint32_t)compressedTextures.size();
						compressedTextures.push_back(texCoord[i]);
					}
					else
					{
						texturesMapping[oldSize + i] = it->second;
					}
				}
			}
		}
	}
	for (uint32_t i = 0; i < compressedTextures.size(); ++i)
	{
		std::swap(compressedTextures[i].x, compressedTextures[i].y);
	}

	outputData->positionsCount = (uint32_t)compressedPositions.size();
	// meshData.positions = compressedPositions.data();
	outputData->positions = new NvcVec3[outputData->positionsCount];
	memcpy(outputData->positions, compressedPositions.data(), sizeof(NvcVec3) * outputData->positionsCount);
	outputData->normalsCount = (uint32_t)compressedNormals.size();
	// meshData.normals = compressedNormals.data();
	outputData->normals = new NvcVec3[outputData->normalsCount];
	memcpy(outputData->normals, compressedNormals.data(), sizeof(NvcVec3) * outputData->normalsCount);
	outputData->uvsCount = (uint32_t)compressedTextures.size();
	// meshData.uvs = compressedTextures.data();
	outputData->uvs = new NvcVec2[outputData->uvsCount];
	memcpy(outputData->uvs, compressedTextures.data(), sizeof(NvcVec2) * outputData->uvsCount);

	uint32_t apexChunkCount    = dasset->chunks.arraySizes[0];
	outputData->meshCount      = (uint32_t)chunkReorderInvMap.size();
	outputData->submeshOffsets = new uint32_t[outputData->meshCount * outputData->submeshCount + 1]{ 0 };

	// count total number of indices
	for (uint32_t chunkIndex = 0; chunkIndex < apexChunkCount; ++chunkIndex)
	{
		uint32_t apexChunkIndex = chunkReorderInvMap[chunkIndex];
		if (apexChunkIndex >= apexChunkCount)
		{
			PX_ALWAYS_ASSERT();
			continue;
		}
		uint32_t part = getPartIndex(dasset, chunkIndex);
		for (uint32_t submeshIndex = 0; submeshIndex < outputData->submeshCount; ++submeshIndex)
		{
			SubmeshParameters* sm = static_cast<SubmeshParameters*>(rmAsset->submeshes.buf[submeshIndex]);

			uint32_t indexCount = sm->indexPartition.buf[part + 1] - sm->indexPartition.buf[part];
			uint32_t* firstIdx  = outputData->submeshOffsets + chunkIndex * outputData->submeshCount + submeshIndex;
			*(firstIdx + 1)     = *firstIdx + indexCount;
		}
	}
	outputData->posIndex  = new uint32_t[outputData->submeshOffsets[outputData->meshCount * outputData->submeshCount]];
	outputData->normIndex = new uint32_t[outputData->submeshOffsets[outputData->meshCount * outputData->submeshCount]];
	outputData->texIndex  = new uint32_t[outputData->submeshOffsets[outputData->meshCount * outputData->submeshCount]];
	// copy indices
	for (uint32_t chunkIndex = 0; chunkIndex < outputData->meshCount; ++chunkIndex)
	{
		uint32_t apexChunkIndex = chunkReorderInvMap[chunkIndex];
		if (apexChunkIndex >= apexChunkCount)
		{
			PX_ALWAYS_ASSERT();
			continue;
		}
		uint32_t part   = getPartIndex(dasset, chunkIndex);
		uint32_t offset = 0;
		for (uint32_t submeshIndex = 0; submeshIndex < outputData->submeshCount; ++submeshIndex)
		{
			SubmeshParameters* sm      = static_cast<SubmeshParameters*>(rmAsset->submeshes.buf[submeshIndex]);
			const uint32_t* indexArray = sm->indexBuffer.buf + sm->indexPartition.buf[part];
			uint32_t indexCount        = sm->indexPartition.buf[part + 1] - sm->indexPartition.buf[part];

			uint32_t firstIdx = outputData->submeshOffsets[chunkIndex * outputData->submeshCount + submeshIndex];

			for (uint32_t i = 0; i < indexCount; ++i)
			{
				outputData->posIndex[firstIdx + i]  = positionsMapping[indexArray[i] + offset];
				outputData->normIndex[firstIdx + i] = normalsMapping[indexArray[i] + offset];
				outputData->texIndex[firstIdx + i]  = texturesMapping[indexArray[i] + offset];
			}
			nvidia::apex::VertexBufferParameters* vbuf =
			    static_cast<nvidia::apex::VertexBufferParameters*>(sm->vertexBuffer);
			offset += vbuf->vertexCount;
		}
	}
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
	const void* assetData  = asset;
	uint32_t assetDataSize = NvBlastAssetGetSize(asset, logLL);
	stream->write(assetData, assetDataSize);
	stream->close();
	NVBLAST_LOG_INFO("Saving finished.");
	return true;
}


ApexImportTool::~ApexImportTool()
{
	m_collisionBuilder->release();
	m_Cooking->release();
	m_PhysxSDK->release();
	m_Foundation->release();
	
}

}  // namespace ApexImporter

}  // namespace Blast
}  // namespace Nv
