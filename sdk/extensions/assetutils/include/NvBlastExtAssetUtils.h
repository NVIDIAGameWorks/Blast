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


#ifndef NVBLASTEXTASSETUTILS_H
#define NVBLASTEXTASSETUTILS_H


#include "NvBlastTypes.h"
#include "NvCTypes.h"
#include <stdint.h>


/**
Reauthor the provided asset to bond the specified support chunks to the world.

\param[in] asset					Pointer to the original asset. Won't be modified.
\param[in] worldBoundChunks			Array of support chunk indices which are to be bound to the world.
\param[in] worldBoundChunksCount	Size of worldBoundChunks array.
\param[in] bondDirections			Array of normals for each bond (size worldBoundChunksCount)
\param[in] bondUserData				Array of user data values for the new bonds, of size worldBoundChunksCount.  May be NULL.  If NULL, bond user data will be set to zero.

\return a new asset with added bonds if successful, NULL otherwise.
*/
NVBLAST_API NvBlastAsset* NvBlastExtAssetUtilsAddWorldBonds
(
	const NvBlastAsset* asset,
	const uint32_t* worldBoundChunks,
	uint32_t worldBoundChunkCount,
	const NvcVec3* bondDirections,
	const uint32_t* bondUserData
);


/**
Bond descriptor used to merge assets.

In addition to the NvBlastBondDesc fields, adds "component" indices to indicate
to which component asset the chunk indices in NvBlastBondDesc refer.  Used in the
function NvBlastExtAssetUtilsMergeAssets.
*/
struct NvBlastExtAssetUtilsBondDesc : public NvBlastBondDesc
{
	uint32_t	componentIndices[2];	//!< The asset component for the corresponding chunkIndices[2] value.
};


/**
Creates an asset descriptor which will build an asset that merges several assets.  Each asset (or component)
is given a transform, applied to the geometric information in the chunk and bond descriptors.

New bond descriptors may be given to bond support chunks from different components.

An NvBlastAsset may appear more than once in the components array.

NOTE: This function allocates memory using the allocator in NvBlastGlobals, to create the new chunk and bond
descriptor arrays referenced in the returned NvBlastAssetDesc.  The user must free this memory after use with
NVBLAST_FREE appied to the pointers in the returned NvBlastAssetDesc.

\param[in]	components		An array of assets to merge, of size componentCount.
\param[in]	rotations		An array of rotations to apply to the geometric data in the chunks and bonds,
							stored quaternion format. The quaternions MUST be normalized.  If NULL, no rotations are applied.
							If not NULL, the array must be of size componentCount.
\param[in]	translations	An array of transforms to apply to the geometric data in the chunks and bonds.
							If NULL, no translations are applied.  If not NULL, the array must be of size componentCount.
\param[in]	componentCount	The size of the components and relativeTransforms arrays.
\param[in]	newBondDescs	Descriptors of type NvBlastExtAssetUtilsBondDesc for new bonds between components, of size newBondCount.  If NULL, newBondCount must be 0.
\param[in]	newBondCount	The size of the newBondDescs array.

\return an asset descriptor that will build an asset which merges the components, using NvBlastCreateAsset.
*/
NVBLAST_API NvBlastAssetDesc NvBlastExtAssetUtilsMergeAssets
(
	const NvBlastAsset** components,
	const NvcQuat* rotations,
	const NvcVec3* translations,
	uint32_t componentCount,
	const NvBlastExtAssetUtilsBondDesc* newBondDescs,
	uint32_t newBondCount
);


/**
Transforms asset in place using scale, rotation, transform. 
Chunk centroids, chunk bond centroids and bond normals are being transformed.
Chunk volume and bond area are changed accordingly.

\param[in, out]	asset		Pointer to the asset to be transformed (modified).
\param[in]		scale		Pointer to scale to be applied. Can be nullptr.
\param[in]		rotation	Pointer to rotation to be applied. Can be nullptr.
\param[in]		translation	Pointer to translation to be applied. Can be nullptr.
*/
NVBLAST_API void NvBlastExtAssetTransformInPlace
(
	NvBlastAsset* asset,
	const NvcVec3* scale,
	const NvcQuat* rotation,
	const NvcVec3* translation
);

#endif // ifndef NVBLASTEXTASSETUTILS_H
