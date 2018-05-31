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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.


#include "NvBlastExtAssetUtils.h"
#include "NvBlast.h"
#include "NvBlastIndexFns.h"
#include "NvBlastMemory.h"
#include "NvBlastGlobals.h"
#include "math.h"

using namespace Nv::Blast;


/**
Fill the chunk and bond descriptors from an asset.

\param[out]	chunkDescsWritten	the number of chunk descriptors written to chunkDescs
\param[out]	bondDescsWritten	the number of bond descriptors written to bondDescs
\param[out]	chunkDescs			user-supplied buffer of NvBlastChunkDesc.  Size must be at least NvBlastAssetGetChunkCount(asset, logFn)
\param[out]	bondDescs			user-supplied buffer of NvBlastBondDesc.  Size must be at least NvBlastAssetGetBondCount(asset, logFn)
\param[in]	asset				asset from which to extract descriptors
*/
static void fillChunkAndBondDescriptorsFromAsset
(
	uint32_t& chunkDescsWritten,
	uint32_t& bondDescsWritten,
	NvBlastChunkDesc* chunkDescs,
	NvBlastBondDesc* bondDescs,
	const NvBlastAsset* asset
)
{
	chunkDescsWritten = 0;
	bondDescsWritten = 0;

	// Chunk descs
	const uint32_t assetChunkCount = NvBlastAssetGetChunkCount(asset, logLL);
	const NvBlastChunk* assetChunk = NvBlastAssetGetChunks(asset, logLL);
	for (uint32_t i = 0; i < assetChunkCount; ++i, ++assetChunk)
	{
		NvBlastChunkDesc& chunkDesc = chunkDescs[chunkDescsWritten++];
		memcpy(chunkDesc.centroid, assetChunk->centroid, sizeof(float) * 3);
		chunkDesc.volume = assetChunk->volume;
		chunkDesc.parentChunkIndex = assetChunk->parentChunkIndex;
		chunkDesc.flags = 0;	// To be filled in below
		chunkDesc.userData = assetChunk->userData;
	}

	// Bond descs
	const uint32_t assetBondCount = NvBlastAssetGetBondCount(asset, logLL);
	const NvBlastBond* assetBond = NvBlastAssetGetBonds(asset, logLL);
	for (uint32_t i = 0; i < assetBondCount; ++i, ++assetBond)
	{
		NvBlastBondDesc& bondDesc = bondDescs[bondDescsWritten++];
		memcpy(&bondDesc.bond, assetBond, sizeof(NvBlastBond));
	}

	// Walk the graph and restore connection descriptors
	const NvBlastSupportGraph graph = NvBlastAssetGetSupportGraph(asset, logLL);
	for (uint32_t i = 0; i < graph.nodeCount; ++i)
	{
		const int32_t currentChunk = graph.chunkIndices[i];
		if (isInvalidIndex(currentChunk))
		{
			continue;
		}

		chunkDescs[currentChunk].flags |= NvBlastChunkDesc::SupportFlag;	// Filling in chunk flags here

		for (uint32_t j = graph.adjacencyPartition[i]; j < graph.adjacencyPartition[i + 1]; ++j)
		{
			NvBlastBondDesc& bondDesc = bondDescs[graph.adjacentBondIndices[j]];
			bondDesc.chunkIndices[0] = currentChunk;
			const uint32_t adjacentChunkIndex = graph.chunkIndices[graph.adjacentNodeIndices[j]];
			bondDesc.chunkIndices[1] = adjacentChunkIndex;
		}
	}
}


/**
Scale a 3-vector v in-place.

\param[in,out]	v	The vector to scale.
\param[in]		s	The scale.  Represents the diagonal elements of a diagonal matrix.

The result will be v <- s*v.
*/
static inline void scale(NvcVec3& v, const NvcVec3& s)
{
	v.x *= s.x;
	v.y *= s.y;
	v.z *= s.z;
}


/**
Rotate a 3-vector v in-place using a rotation represented by a quaternion q.

\param[in,out]	v	The vector to rotate.
\param[in]		q	The quaternion representation the rotation.

The format of q is { x, y, z, w } where (x,y,z) is the vector part and w is the scalar part.
The quaternion q MUST be normalized.
*/
static inline void rotate(NvcVec3& v, const NvcQuat& q)
{
	const float vx = 2.0f * v.x;
	const float vy = 2.0f * v.y;
	const float vz = 2.0f * v.z;
	const float w2 = q.w * q.w - 0.5f;
	const float dot2 = (q.x * vx + q.y * vy + q.z * vz);
	v.x = vx * w2 + (q.y * vz - q.z * vy) * q.w + q.x * dot2;
	v.y = vy * w2 + (q.z * vx - q.x * vz) * q.w + q.y * dot2;
	v.z = vz * w2 + (q.x * vy - q.y * vx) * q.w + q.z * dot2;
}


/**
Translate a 3-vector v in-place.

\param[in,out]	v	The vector to translate.
\param[in]		t	The translation.

The result will be v <- v+t.
*/
static inline void translate(NvcVec3& v, const NvcVec3& t)
{
	v.x += t.x;
	v.y += t.y;
	v.z += t.z;
}


NvBlastAsset* NvBlastExtAssetUtilsAddWorldBonds
(
	const NvBlastAsset* asset,
	const uint32_t* worldBoundChunks,
	uint32_t worldBoundChunkCount,
	const NvcVec3* bondDirections,
	const uint32_t* bondUserData
)
{
	const uint32_t chunkCount = NvBlastAssetGetChunkCount(asset, logLL);
	const uint32_t oldBondCount = NvBlastAssetGetBondCount(asset, logLL);
	const uint32_t newBondCount = oldBondCount + worldBoundChunkCount;

	NvBlastChunkDesc* chunkDescs = static_cast<NvBlastChunkDesc*>(NVBLAST_ALLOC(chunkCount * sizeof(NvBlastChunkDesc)));
	NvBlastBondDesc* bondDescs = static_cast<NvBlastBondDesc*>(NVBLAST_ALLOC(newBondCount * sizeof(NvBlastBondDesc)));

	// Create chunk descs
	uint32_t chunkDescsWritten;
	uint32_t bondDescsWritten;
	fillChunkAndBondDescriptorsFromAsset(chunkDescsWritten, bondDescsWritten, chunkDescs, bondDescs, asset);

	// Add world bonds
	uint32_t bondCount = oldBondCount;
	for (uint32_t i = 0; i < worldBoundChunkCount; i++)
	{
		NvBlastBondDesc& bondDesc = bondDescs[bondCount++];
		const uint32_t chunkIndex = worldBoundChunks[i];
		bondDesc.chunkIndices[0] = chunkIndex;
		bondDesc.chunkIndices[1] = invalidIndex<uint32_t>();
		memcpy(&bondDesc.bond.normal, bondDirections + i, sizeof(float) * 3);
		bondDesc.bond.area = 1.0f;	// Should be set by user
		memcpy(&bondDesc.bond.centroid, chunkDescs[chunkIndex].centroid, sizeof(float) * 3);
		bondDesc.bond.userData = bondUserData != nullptr ? bondUserData[i] : 0;
	}

	// Create new asset
	NvBlastAssetDesc assetDesc;
	assetDesc.chunkCount = chunkCount;
	assetDesc.chunkDescs = chunkDescs;
	assetDesc.bondCount = bondCount;
	assetDesc.bondDescs = bondDescs;
	void* scratch = NVBLAST_ALLOC(NvBlastGetRequiredScratchForCreateAsset(&assetDesc, logLL));
	NvBlastAsset* newAsset = NvBlastCreateAsset(NVBLAST_ALLOC(NvBlastGetAssetMemorySize(&assetDesc, logLL)), &assetDesc, scratch, logLL);

	// Free buffers
	NVBLAST_FREE(scratch);
	NVBLAST_FREE(bondDescs);
	NVBLAST_FREE(chunkDescs);

	return newAsset;
}


NvBlastAssetDesc NvBlastExtAssetUtilsMergeAssets
(
	const NvBlastAsset** components,
	const NvcVec3* scales,
	const NvcQuat* rotations,
	const NvcVec3* translations,
	uint32_t componentCount,
	const NvBlastExtAssetUtilsBondDesc* newBondDescs,
	uint32_t newBondCount,
	uint32_t* chunkIndexOffsets,
	uint32_t* chunkReorderMap,
	uint32_t chunkReorderMapSize
)
{
	// Count the total number of chunks and bonds in the new asset
	uint32_t totalChunkCount = 0;
	uint32_t totalBondCount = newBondCount;
	for (uint32_t c = 0; c < componentCount; ++c)
	{
		totalChunkCount += NvBlastAssetGetChunkCount(components[c], logLL);
		totalBondCount += NvBlastAssetGetBondCount(components[c], logLL);
	}

	// Allocate space for chunk and bond descriptors
	NvBlastChunkDesc* chunkDescs = static_cast<NvBlastChunkDesc*>(NVBLAST_ALLOC(totalChunkCount * sizeof(NvBlastChunkDesc)));
	NvBlastBondDesc* bondDescs = static_cast<NvBlastBondDesc*>(NVBLAST_ALLOC(totalBondCount * sizeof(NvBlastBondDesc)));

	// Create a list of chunk index offsets per component
	uint32_t* offsetStackAlloc = static_cast<uint32_t*>(NvBlastAlloca(componentCount * sizeof(uint32_t)));
	if (chunkIndexOffsets == nullptr)
	{
		chunkIndexOffsets = offsetStackAlloc;	// Use local stack alloc if no array is provided
	}

	// Fill the chunk and bond descriptors from the components
	uint32_t chunkCount = 0;
	uint32_t bondCount = 0;
	for (uint32_t c = 0; c < componentCount; ++c)
	{
		chunkIndexOffsets[c] = chunkCount;
		uint32_t componentChunkCount;
		uint32_t componentBondCount;
		fillChunkAndBondDescriptorsFromAsset(componentChunkCount, componentBondCount, chunkDescs + chunkCount, bondDescs + bondCount, components[c]);
		// Fix chunks' parent indices
		for (uint32_t i = 0; i < componentChunkCount; ++i)
		{
			if (!isInvalidIndex(chunkDescs[chunkCount + i].parentChunkIndex))
			{
				chunkDescs[chunkCount + i].parentChunkIndex += chunkCount;
			}
		}
		// Fix bonds' chunk indices
		for (uint32_t i = 0; i < componentBondCount; ++i)
		{
			NvBlastBondDesc& bondDesc = bondDescs[bondCount + i];
			for (int j = 0; j < 2; ++j)
			{
				if (!isInvalidIndex(bondDesc.chunkIndices[j]))
				{
					bondDesc.chunkIndices[j] += chunkCount;
				}
			}
		}
		// Transform geometric data
		if (scales != nullptr)
		{
			const NvcVec3& S = scales[c];
			NvcVec3 cofS = { S.y * S.z, S.z * S.x, S.x * S.y };
			float absDetS = S.x * S.y * S.z;
			const float sgnDetS = absDetS < 0.0f ? -1.0f : 1.0f;
			absDetS *= sgnDetS;
			for (uint32_t i = 0; i < componentChunkCount; ++i)
			{
				scale(reinterpret_cast<NvcVec3&>(chunkDescs[chunkCount + i].centroid), S);
				chunkDescs[chunkCount + i].volume *= absDetS;
			}
			for (uint32_t i = 0; i < componentBondCount; ++i)
			{
				NvBlastBond& bond = bondDescs[bondCount + i].bond;
				scale(reinterpret_cast<NvcVec3&>(bond.normal), cofS);
				float renorm = sqrtf(bond.normal[0] * bond.normal[0] + bond.normal[1] * bond.normal[1] + bond.normal[2] * bond.normal[2]);
				bond.area *= renorm;
				if (renorm != 0)
				{
					renorm = sgnDetS / renorm;
					bond.normal[0] *= renorm;
					bond.normal[1] *= renorm;
					bond.normal[2] *= renorm;
				}
				scale(reinterpret_cast<NvcVec3&>(bond.centroid), S);
			}
		}
		if (rotations != nullptr)
		{
			for (uint32_t i = 0; i < componentChunkCount; ++i)
			{
				rotate(reinterpret_cast<NvcVec3&>(chunkDescs[chunkCount + i].centroid), rotations[c]);
			}
			for (uint32_t i = 0; i < componentBondCount; ++i)
			{
				NvBlastBond& bond = bondDescs[bondCount + i].bond;
				rotate(reinterpret_cast<NvcVec3&>(bond.normal), rotations[c]);	// Normal can be transformed this way since we aren't scaling
				rotate(reinterpret_cast<NvcVec3&>(bond.centroid), rotations[c]);
			}
		}
		if (translations != nullptr)
		{
			for (uint32_t i = 0; i < componentChunkCount; ++i)
			{
				translate(reinterpret_cast<NvcVec3&>(chunkDescs[chunkCount + i].centroid), translations[c]);
			}
			for (uint32_t i = 0; i < componentBondCount; ++i)
			{
				translate(reinterpret_cast<NvcVec3&>(bondDescs[bondCount + i].bond.centroid), translations[c]);
			}
		}
		chunkCount += componentChunkCount;
		bondCount += componentBondCount;
	}

	// Fill the bond descriptors from the new bond descs
	for (uint32_t b = 0; b < newBondCount; ++b)
	{
		const NvBlastExtAssetUtilsBondDesc& newBondDesc = newBondDescs[b];
		NvBlastBondDesc& bondDesc = bondDescs[bondCount++];
		memcpy(&bondDesc.bond, &newBondDesc.bond, sizeof(NvBlastBond));
		bondDesc.chunkIndices[0] = !isInvalidIndex(newBondDesc.chunkIndices[0]) ? newBondDesc.chunkIndices[0] + chunkIndexOffsets[newBondDesc.componentIndices[0]] : invalidIndex<uint32_t>();
		bondDesc.chunkIndices[1] = !isInvalidIndex(newBondDesc.chunkIndices[1]) ? newBondDesc.chunkIndices[1] + chunkIndexOffsets[newBondDesc.componentIndices[1]] : invalidIndex<uint32_t>();
	}

	// Create new asset desriptor
	NvBlastAssetDesc assetDesc;
	assetDesc.chunkCount = chunkCount;
	assetDesc.chunkDescs = chunkDescs;
	assetDesc.bondCount = bondCount;
	assetDesc.bondDescs = bondDescs;

	// Massage the descriptors so that they are valid for scratch creation
	void* scratch = NVBLAST_ALLOC(chunkCount * sizeof(NvBlastChunkDesc));	// Enough for NvBlastEnsureAssetExactSupportCoverage and NvBlastReorderAssetDescChunks

	NvBlastEnsureAssetExactSupportCoverage(chunkDescs, chunkCount, scratch, logLL);

	if (chunkReorderMapSize < chunkCount)
	{
		if (chunkReorderMap != nullptr)
		{
			// Chunk reorder map is not large enough.  Fill it with invalid indices and don't use it.
			memset(chunkReorderMap, 0xFF, chunkReorderMapSize * sizeof(uint32_t));
			NVBLAST_LOG_WARNING("NvBlastExtAssetUtilsMergeAssets: insufficient chunkReorderMap array passed in.  NvBlastReorderAssetDescChunks will not be used.");
		}
		chunkReorderMap = nullptr;	// Don't use
	}

	if (chunkReorderMap != nullptr)
	{
		NvBlastReorderAssetDescChunks(chunkDescs, chunkCount, bondDescs, bondCount, chunkReorderMap, true, scratch, logLL);
	}

	NVBLAST_FREE(scratch);

	return assetDesc;
}


/**
Multiply a 3-vector v in-place by value.

\param[in,out]	v	The vector to multiply.
\param[in]		m	The 3x3 matrix.
*/
static inline void multiply(NvcVec3& v, float value)
{
	v.x *= value;
	v.y *= value;
	v.z *= value;
}


/**
Get Vec3 length
*/
static inline float length(const NvcVec3& p)
{
	return sqrtf(p.x * p.x + p.y * p.y + p.z * p.z);
}


/**
Transform a point in-place: scale, rotate, then translate

\param[in,out]	p	The point to transform.
\param[in]		S	The diagonal elements of a diagonal scale matrix.
\param[in]		R	A quaternion representing the rotation.  Must be normalized.
\param[in]		T	The translation vector.
*/
static inline void transform(NvcVec3& p, const NvcVec3& S, const NvcQuat& R, const NvcVec3& T)
{
	scale(p, S);
	rotate(p, R);
	translate(p, T);
}


/**
Transform a vector in-place: scale, then rotate

\param[in,out]	v	The vector to transform.
\param[in]		S	The diagonal elements of a diagonal scale matrix.
\param[in]		R	A quaternion representing the rotation.  Must be normalized.
*/
static inline void transform(NvcVec3& v, const NvcVec3& S, const NvcQuat& R)
{
	scale(v, S);
	rotate(v, R);
}


void NvBlastExtAssetTransformInPlace(NvBlastAsset* asset, const NvcVec3* scaling, const NvcQuat* rotation, const NvcVec3* translation)
{
	// Local copies of scaling (S), rotation (R), and translation (T)
	NvcVec3 S = { 1, 1, 1 };
	NvcQuat R = { 0, 0, 0, 1 };
	NvcVec3 T = { 0, 0, 0 };
	NvcVec3 cofS = { 1, 1, 1 };
	float absDetS = 1;
	float sgnDetS = 1;

	{
		if (rotation)
		{
			R = *rotation;
		}

		if (scaling)
		{
			S = *scaling;
			cofS.x = S.y * S.z;
			cofS.y = S.z * S.x;
			cofS.z = S.x * S.y;
			absDetS = S.x * S.y * S.z;
			sgnDetS = absDetS < 0.0f ? -1.0f : 1.0f;
			absDetS *= sgnDetS;
		}

		if (translation)
		{
			T = *translation;
		}
	}

	// Chunk descs
	const uint32_t assetChunkCount = NvBlastAssetGetChunkCount(asset, logLL);
	NvBlastChunk* assetChunk = const_cast<NvBlastChunk*>(NvBlastAssetGetChunks(asset, logLL));
	for (uint32_t i = 0; i < assetChunkCount; ++i, ++assetChunk)
	{
		transform(reinterpret_cast<NvcVec3&>(assetChunk->centroid), S, R, T);
		assetChunk->volume *= absDetS;	// Use |detS| to keep the volume positive
	}

	// Bond descs
	const uint32_t assetBondCount = NvBlastAssetGetBondCount(asset, logLL);
	NvBlastBond* assetBond = const_cast<NvBlastBond*>(NvBlastAssetGetBonds(asset, logLL));
	for (uint32_t i = 0; i < assetBondCount; ++i, ++assetBond)
	{
		transform(reinterpret_cast<NvcVec3&>(assetBond->centroid), S, R, T);
		NvcVec3& normal = reinterpret_cast<NvcVec3&>(assetBond->normal);
		transform(normal, cofS, R);
		const float l = length(normal);
		assetBond->area *= l;
		multiply(normal, l > 0.f ? sgnDetS / l : 1.f);
	}
}
