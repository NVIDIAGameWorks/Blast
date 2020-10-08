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


#include "AssetGenerator.h"
#include <cstring>


void CubeAssetGenerator::generate(GeneratorAsset& asset, const Settings& settings)
{
	// cleanup
	asset.solverChunks.clear();
	asset.solverBonds.clear();
	asset.chunks.clear();

	// initial params
	std::vector<uint32_t> depthStartIDs;
	std::vector<GeneratorAsset::Vec3> depthSlicesPerAxisTotal;
	uint32_t currentID = 0;
	GeneratorAsset::Vec3 extents = settings.extents;
	asset.extents = extents;

	// Iterate over depths and create children
	for (uint32_t depth = 0; depth < settings.depths.size(); depth++)
	{
		GeneratorAsset::Vec3 slicesPerAxis = settings.depths[depth].slicesPerAxis;
		GeneratorAsset::Vec3 slicesPerAxisTotal = (depth == 0) ? slicesPerAxis : slicesPerAxis * (depthSlicesPerAxisTotal[depth - 1]);
		depthSlicesPerAxisTotal.push_back(slicesPerAxisTotal);

		depthStartIDs.push_back(currentID);

		extents.x /= slicesPerAxis.x;
		extents.y /= slicesPerAxis.y;
		extents.z /= slicesPerAxis.z;

		for (uint32_t z = 0; z < (uint32_t)slicesPerAxisTotal.z; ++z)
		{
			uint32_t parent_z = z / (uint32_t)slicesPerAxis.z;
			for (uint32_t y = 0; y < (uint32_t)slicesPerAxisTotal.y; ++y)
			{
				uint32_t parent_y = y / (uint32_t)slicesPerAxis.y;
				for (uint32_t x = 0; x < (uint32_t)slicesPerAxisTotal.x; ++x)
				{
					uint32_t parent_x = x / (uint32_t)slicesPerAxis.x;
					uint32_t parentID = depth == 0 ? UINT32_MAX :
						depthStartIDs[depth - 1] + parent_x + (uint32_t)depthSlicesPerAxisTotal[depth - 1].x*(parent_y + (uint32_t)depthSlicesPerAxisTotal[depth - 1].y*parent_z);

					GeneratorAsset::Vec3 position;
					position.x = ((float)x - (slicesPerAxisTotal.x / 2) + 0.5f) * extents.x;
					position.y = ((float)y - (slicesPerAxisTotal.y / 2) + 0.5f) * extents.y;
					position.z = ((float)z - (slicesPerAxisTotal.z / 2) + 0.5f) * extents.z;

					NvBlastChunkDesc chunkDesc;
					memcpy(chunkDesc.centroid, &position.x, 3 * sizeof(float));
					chunkDesc.volume = extents.x * extents.y * extents.z;
					chunkDesc.flags = settings.depths[depth].flag;
					chunkDesc.userData = currentID++;
					chunkDesc.parentChunkIndex = parentID;
					asset.solverChunks.push_back(chunkDesc);

					if (settings.depths[depth].flag & NvBlastChunkDesc::Flags::SupportFlag)
					{
						// Internal bonds

						// x-neighbor
						if (x > 0 && (settings.bondFlags & BondFlags::X_BONDS))
						{
							GeneratorAsset::Vec3 xNeighborPosition = position - GeneratorAsset::Vec3(extents.x, 0, 0);
							uint32_t neighborID = chunkDesc.userData - 1;
							fillBondDesc(asset.solverBonds, chunkDesc.userData, neighborID, position, xNeighborPosition, extents, extents.y * extents.z);
						}

						// y-neighbor
						if (y > 0 && (settings.bondFlags & BondFlags::Y_BONDS))
						{
							GeneratorAsset::Vec3 yNeighborPosition = position - GeneratorAsset::Vec3(0, extents.y, 0);
							uint32_t neighborID = chunkDesc.userData - (uint32_t)slicesPerAxisTotal.x;
							fillBondDesc(asset.solverBonds, chunkDesc.userData, neighborID, position, yNeighborPosition, extents, extents.z * extents.x);
						}

						// z-neighbor
						if (z > 0 && (settings.bondFlags & BondFlags::Z_BONDS))
						{
							GeneratorAsset::Vec3 zNeighborPosition = position - GeneratorAsset::Vec3(0, 0, extents.z);
							uint32_t neighborID = chunkDesc.userData - (uint32_t)slicesPerAxisTotal.x*(uint32_t)slicesPerAxisTotal.y;
							fillBondDesc(asset.solverBonds, chunkDesc.userData, neighborID, position, zNeighborPosition, extents, extents.x * extents.y);
						}

						// World bonds (only one per chunk is enough, otherwise they will be removed as duplicated, thus 'else if')

						// -x world bond
						if (x == 0 && (settings.bondFlags & BondFlags::X_MINUS_WORLD_BONDS))
						{
							GeneratorAsset::Vec3 xNeighborPosition = position - GeneratorAsset::Vec3(extents.x, 0, 0);
							fillBondDesc(asset.solverBonds, chunkDesc.userData, UINT32_MAX, position, xNeighborPosition, extents, extents.y * extents.z);
						}
						// +x world bond
						else if (x == slicesPerAxisTotal.x - 1 && (settings.bondFlags & BondFlags::X_PLUS_WORLD_BONDS))
						{
							GeneratorAsset::Vec3 xNeighborPosition = position + GeneratorAsset::Vec3(extents.x, 0, 0);
							fillBondDesc(asset.solverBonds, chunkDesc.userData, UINT32_MAX, position, xNeighborPosition, extents, extents.y * extents.z);
						}
						// -y world bond
						else if (y == 0 && (settings.bondFlags & BondFlags::Y_MINUS_WORLD_BONDS))
						{
							GeneratorAsset::Vec3 yNeighborPosition = position - GeneratorAsset::Vec3(0, extents.y, 0);
							fillBondDesc(asset.solverBonds, chunkDesc.userData, UINT32_MAX, position, yNeighborPosition, extents, extents.z * extents.x);
						}
						// +y world bond
						else if (y == slicesPerAxisTotal.y - 1 && (settings.bondFlags & BondFlags::Y_PLUS_WORLD_BONDS))
						{
							GeneratorAsset::Vec3 yNeighborPosition = position + GeneratorAsset::Vec3(0, extents.y, 0);
							fillBondDesc(asset.solverBonds, chunkDesc.userData, UINT32_MAX, position, yNeighborPosition, extents, extents.z * extents.x);
						}
						// -z world bond
						else if (z == 0 && (settings.bondFlags & BondFlags::Z_MINUS_WORLD_BONDS))
						{
							GeneratorAsset::Vec3 zNeighborPosition = position - GeneratorAsset::Vec3(0, 0, extents.z);
							fillBondDesc(asset.solverBonds, chunkDesc.userData, UINT32_MAX, position, zNeighborPosition, extents, extents.x * extents.y);
						}
						// +z world bond
						else if (z == slicesPerAxisTotal.z - 1 && (settings.bondFlags & BondFlags::Z_PLUS_WORLD_BONDS))
						{
							GeneratorAsset::Vec3 zNeighborPosition = position + GeneratorAsset::Vec3(0, 0, extents.z);
							fillBondDesc(asset.solverBonds, chunkDesc.userData, UINT32_MAX, position, zNeighborPosition, extents, extents.x * extents.y);
						}
					}

					asset.chunks.push_back(GeneratorAsset::BlastChunkCube(position, extents/*isStatic*/));
				}
			}
		}
	}

	// Reorder chunks
	std::vector<uint32_t> chunkReorderMap(asset.solverChunks.size());
	std::vector<char> scratch(asset.solverChunks.size() * sizeof(NvBlastChunkDesc));
	NvBlastBuildAssetDescChunkReorderMap(chunkReorderMap.data(), asset.solverChunks.data(), (uint32_t)asset.solverChunks.size(), scratch.data(), nullptr);

	std::vector<GeneratorAsset::BlastChunkCube> chunksTemp = asset.chunks;
	for (uint32_t i = 0; i < chunkReorderMap.size(); ++i)
	{
		asset.chunks[chunkReorderMap[i]] = chunksTemp[i];
	}
	NvBlastApplyAssetDescChunkReorderMapInPlace(asset.solverChunks.data(), (uint32_t)asset.solverChunks.size(), asset.solverBonds.data(), (uint32_t)asset.solverBonds.size(), chunkReorderMap.data(), true, scratch.data(), nullptr);
}

void CubeAssetGenerator::fillBondDesc(std::vector<NvBlastBondDesc>& bondDescs, uint32_t id0, uint32_t id1, GeneratorAsset::Vec3 pos0, GeneratorAsset::Vec3 pos1, GeneratorAsset::Vec3 size, float area)
{
	NV_UNUSED(size);

	NvBlastBondDesc bondDesc;
	bondDesc.chunkIndices[0] = id0;
	bondDesc.chunkIndices[1] = id1;
	bondDesc.bond.area = area;
	GeneratorAsset::Vec3 centroid = (pos0 + pos1) * 0.5f;
	bondDesc.bond.centroid[0] = centroid.x;
	bondDesc.bond.centroid[1] = centroid.y;
	bondDesc.bond.centroid[2] = centroid.z;
	GeneratorAsset::Vec3 normal = (pos0 - pos1).getNormalized();
	bondDesc.bond.normal[0] = normal.x;
	bondDesc.bond.normal[1] = normal.y;
	bondDesc.bond.normal[2] = normal.z;
	bondDescs.push_back(bondDesc);
}


