/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTEXTASSETBLOCKVERSIONCONVERTER_V0_V1_H
#define NVBLASTEXTASSETBLOCKVERSIONCONVERTER_V0_V1_H


#include "NvBlastExtBinaryBlockConverter.h"


namespace Nv
{
namespace Blast
{

/*
	WARNING: THIS CLASS IS AN EXAMPLE.
	REPLACE WITH ACTUAL CONVERSION CODE ONCE NEEDED.
*/
class NvBlastAssetBlockVersionConverter_v0_v1 : public BinaryBlockConverter::VersionConverter
{
public:
	virtual uint32_t getVersionFrom() const { return NvBlastAssetDataFormat::Initial; }

	virtual uint32_t getVersionTo() const { return 1/*NvBlastAssetDataFormat::BondCountSwap*/; }

	// remains the same
	struct SupportGraph
	{
		uint32_t	m_nodeCount;
		uint32_t	m_chunkIndicesOffset;
		uint32_t	m_adjacencyPartitionOffset;
		uint32_t	m_adjacentNodeIndicesOffset;
		uint32_t	m_adjacentBondIndicesOffset;
	};

	// prev version 
	struct AssetDataHeaderPrev
	{
		uint32_t		m_formatVersion;
		uint32_t		m_size;
		NvBlastID		m_ID;
		uint32_t		m_totalChunkCount;
		SupportGraph	m_graph;
		uint32_t		m_leafChunkCount;
		uint32_t		m_firstSubsupportChunkIndex; // 0
		uint32_t		m_bondCount; // 1
	};

	// new version
	struct AssetDataHeaderNew
	{
		uint32_t		m_formatVersion;
		uint32_t		m_size;
		NvBlastID		m_ID;
		uint32_t		m_totalChunkCount;
		SupportGraph	m_graph;
		uint32_t		m_leafChunkCount;
		uint32_t		m_bondCount; // 1
		uint32_t		m_firstSubsupportChunkIndex; // 0
	};

	bool convert(const std::vector<char>& from, std::vector<char>& to) const
	{
		to = from;

		const AssetDataHeaderPrev* headerPrev = reinterpret_cast<const AssetDataHeaderPrev*>(from.data());
		AssetDataHeaderNew* headerNew = reinterpret_cast<AssetDataHeaderNew*>(to.data());
		headerNew->m_bondCount = headerPrev->m_bondCount;
		headerNew->m_firstSubsupportChunkIndex = headerPrev->m_firstSubsupportChunkIndex;
		headerNew->m_formatVersion = (uint32_t)getVersionTo();

		return true;
	}
};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTASSETBLOCKVERSIONCONVERTER_V0_V1_H
