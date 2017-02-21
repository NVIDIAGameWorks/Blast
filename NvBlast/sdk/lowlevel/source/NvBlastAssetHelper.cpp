/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastAsset.h"
#include "NvBlastIndexFns.h"
#include "NvBlastAssert.h"
#include "NvBlastMemory.h"

#include <algorithm>


namespace Nv
{
namespace Blast
{

/**
Class to hold chunk descriptor and annotation context for sorting a list of indices
*/
class ChunksOrdered
{
public:
	ChunksOrdered(const NvBlastChunkDesc* descs, const char* annotation) : m_descs(descs), m_annotation(annotation) {}

	bool	operator () (uint32_t i0, uint32_t i1) const
	{
		const bool upperSupport0 = (m_annotation[i0] & Asset::ChunkAnnotation::UpperSupport) != 0;
		const bool upperSupport1 = (m_annotation[i1] & Asset::ChunkAnnotation::UpperSupport) != 0;

		if (upperSupport0 != upperSupport1)
		{
			return upperSupport0;	// If one is uppersupport and one is subsupport, uppersupport should come first
		}

		// Parent chunk index (+1 so that UINT32_MAX becomes the lowest value)
		const uint32_t p0 = m_descs[i0].parentChunkIndex + 1;
		const uint32_t p1 = m_descs[i1].parentChunkIndex + 1;

		return p0 < p1;	// With the same support relationship, order by parent index
	}

private:
	const NvBlastChunkDesc*	m_descs;
	const char*				m_annotation;
};

} // namespace Blast
} // namespace Nv


using namespace Nv::Blast;

extern "C"
{

bool NvBlastBuildAssetDescChunkReorderMap(uint32_t* chunkReorderMap, const NvBlastChunkDesc* chunkDescs, uint32_t chunkCount, void* scratch, NvBlastLog logFn)
{
	NVBLAST_CHECK(chunkCount == 0 || chunkDescs != nullptr, logFn, "NvBlastBuildAssetDescChunkReorderMap: NULL chunkDescs input with non-zero chunkCount", return false);
	NVBLAST_CHECK(chunkReorderMap == nullptr || chunkCount != 0, logFn, "NvBlastBuildAssetDescChunkReorderMap: NULL chunkReorderMap input with non-zero chunkCount", return false);
	NVBLAST_CHECK(chunkCount == 0 || scratch != nullptr, logFn, "NvBlastBuildAssetDescChunkReorderMap: NULL scratch input with non-zero chunkCount", return false);

	uint32_t* chunkMap = static_cast<uint32_t*>(scratch);	scratch = pointerOffset(scratch, chunkCount * sizeof(uint32_t));
	char* chunkAnnotation = static_cast<char*>(scratch);	scratch = pointerOffset(scratch, chunkCount * sizeof(char));

	uint32_t supportChunkCount;
	uint32_t leafChunkCount;
	if (!Asset::ensureExactSupportCoverage(supportChunkCount, leafChunkCount, chunkAnnotation, chunkCount, const_cast<NvBlastChunkDesc*>(chunkDescs), true, logFn))
	{
		NVBLAST_LOG_ERROR(logFn, "NvBlastBuildAssetDescChunkReorderMap: chunk descriptors did not have exact coverage, map could not be built.  Use NvBlastEnsureAssetExactSupportCoverage to fix descriptors.");
		return false;
	}

	// check order for fast out (identity map)
	if (Asset::testForValidChunkOrder(chunkCount, chunkDescs, chunkAnnotation, scratch))
	{
		for (uint32_t i = 0; i < chunkCount; ++i)
		{
			chunkReorderMap[i] = i;
		}

		return true;
	}
	
	for (uint32_t i = 0; i < chunkCount; ++i)
	{
		chunkMap[i] = i;
	}
	std::sort(chunkMap, chunkMap + chunkCount, ChunksOrdered(chunkDescs, chunkAnnotation));

	invertMap(chunkReorderMap, chunkMap, chunkCount);

	return false;
}


void NvBlastApplyAssetDescChunkReorderMap
(
	NvBlastChunkDesc* reorderedChunkDescs,
	const NvBlastChunkDesc* chunkDescs,
	uint32_t chunkCount,
	NvBlastBondDesc* bondDescs,
	uint32_t bondCount,
	const uint32_t* chunkReorderMap,
	NvBlastLog logFn
)
{
	NVBLAST_CHECK(chunkCount == 0 || chunkDescs != nullptr, logFn, "NvBlastApplyAssetDescChunkReorderMap: NULL chunkDescs input with non-zero chunkCount", return);
	NVBLAST_CHECK(reorderedChunkDescs == nullptr || chunkCount != 0, logFn, "NvBlastApplyAssetDescChunkReorderMap: NULL reorderedChunkDescs input with non-zero chunkCount", return);
	NVBLAST_CHECK(chunkReorderMap == nullptr || chunkCount != 0, logFn, "NvBlastApplyAssetDescChunkReorderMap: NULL chunkReorderMap input with non-zero chunkCount", return);
	NVBLAST_CHECK(bondCount == 0 || bondDescs != nullptr, logFn, "NvBlastApplyAssetDescChunkReorderMap: NULL bondDescs input with non-zero bondCount", return);
	NVBLAST_CHECK(bondDescs == nullptr || chunkReorderMap != nullptr, logFn, "NvBlastApplyAssetDescChunkReorderMap: NULL bondDescs input with NULL chunkReorderMap", return);

	// Copy chunk descs 
	if (reorderedChunkDescs)
	{
		for (uint32_t i = 0; i < chunkCount; ++i)
		{
			reorderedChunkDescs[chunkReorderMap[i]] = chunkDescs[i];
			uint32_t& parentIndex = reorderedChunkDescs[chunkReorderMap[i]].parentChunkIndex;
			if (parentIndex < chunkCount)
			{
				parentIndex = chunkReorderMap[parentIndex];	// If the parent index is valid, remap it too to reflect the new order
			}
		}
	}

	if (bondDescs)
	{
		for (uint32_t i = 0; i < bondCount; ++i)
		{
			for (int j = 0; j < 2; ++j)
			{
				uint32_t& index = bondDescs[i].chunkIndices[j];
				if (index < chunkCount)
				{
					index = chunkReorderMap[index];
				}
			}
		}
	}
}


void NvBlastApplyAssetDescChunkReorderMapInplace(NvBlastChunkDesc* chunkDescs, uint32_t chunkCount, NvBlastBondDesc* bondDescs, uint32_t bondCount, const uint32_t* chunkReorderMap, void* scratch, NvBlastLog logFn)
{
	NVBLAST_CHECK(chunkCount == 0 || chunkDescs != nullptr, logFn, "NvBlastApplyAssetDescChunkReorderMapInplace: NULL chunkDescs input with non-zero chunkCount", return);
	NVBLAST_CHECK(chunkCount == 0 || scratch != nullptr, logFn, "NvBlastApplyAssetDescChunkReorderMapInplace: NULL scratch input with non-zero chunkCount", return);

	NvBlastChunkDesc* chunksTemp = static_cast<NvBlastChunkDesc*>(scratch);
	memcpy(chunksTemp, chunkDescs, sizeof(NvBlastChunkDesc) * chunkCount);
	NvBlastApplyAssetDescChunkReorderMap(chunkDescs, chunksTemp, chunkCount, bondDescs, bondCount, chunkReorderMap, logFn);
}


bool NvBlastReorderAssetDescChunks(NvBlastChunkDesc* chunkDescs, uint32_t chunkCount, NvBlastBondDesc* bondDescs, uint32_t bondCount, uint32_t* chunkReorderMap, void* scratch, NvBlastLog logFn)
{
	if (!NvBlastBuildAssetDescChunkReorderMap(chunkReorderMap, chunkDescs, chunkCount, scratch, logFn))
	{
		NvBlastApplyAssetDescChunkReorderMapInplace(chunkDescs, chunkCount, bondDescs, bondCount, chunkReorderMap, scratch, logFn);
		return false;
	}
	return true;
}


bool NvBlastEnsureAssetExactSupportCoverage(NvBlastChunkDesc* chunkDescs, uint32_t chunkCount, void* scratch, NvBlastLog logFn)
{
	NVBLAST_CHECK(chunkCount == 0 || chunkDescs != nullptr, logFn, "NvBlastEnsureAssetExactSupportCoverage: NULL chunkDescs input with non-zero chunkCount", return false);
	NVBLAST_CHECK(chunkCount == 0 || scratch != nullptr, logFn, "NvBlastEnsureAssetExactSupportCoverage: NULL scratch input with non-zero chunkCount", return false);

	uint32_t supportChunkCount;
	uint32_t leafChunkCount;
	return Asset::ensureExactSupportCoverage(supportChunkCount, leafChunkCount, static_cast<char*>(scratch), chunkCount, chunkDescs, false, logFn);
}

} // extern "C"
