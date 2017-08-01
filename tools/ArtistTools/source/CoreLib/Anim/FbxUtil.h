// This code contains NVIDIA Confidential Information and is disclosed 
// under the Mutual Non-Disclosure Agreement. 
// 
// Notice 
// ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES 
// NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO 
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT, 
// MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 
// 
// NVIDIA Corporation assumes no responsibility for the consequences of use of such 
// information or for any infringement of patents or other rights of third parties that may 
// result from its use. No license is granted by implication or otherwise under any patent 
// or patent rights of NVIDIA Corporation. No third party distribution is allowed unless 
// expressly authorized by NVIDIA.  Details are subject to change without notice. 
// This code supersedes and replaces all information previously supplied. 
// NVIDIA Corporation products are not authorized for use as critical 
// components in life support devices or systems without express written approval of 
// NVIDIA Corporation. 
// 
// Copyright (c) 2013 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//
#pragma once

#include "AnimUtil.h"


////////////////////////////////////////////////////////////////////////////////////////
// Helper for fbx file load
class CORELIB_EXPORT FbxUtil
{
public:

	/// initialize fbx loader.  
	static bool Initialize(const nvidia::Char* fbxFileName, nvidia::Float& fbxSceneUnit, float toScneUnit = 0.0f, bool bConvertUnit = true);
	static bool Release(void);

	static bool InitializeAnimationCache(AnimationCache& cache);

	/// Get global frame range and fps information from fbx.
	static bool GetGlobalSettings(nvidia::Float32* pStarfFrame = 0, nvidia::Float32* pEndFrame = 0, nvidia::Float32 *pFps = 0, int* upAxis = 0, char* rootBoneName = 0);

	/// get skinning data from the mesh node
	static bool InitializeSkinData( const nvidia::Char* meshName, SkinData& pSkinningDataToUpdate);

	/// create mesh descriptor from fbx mesh node
	static bool CreateMeshDescriptor(const nvidia::Char* meshName, MeshDesc &meshDesc);

	/// get all the renderable meshes from the fbx scene
	static bool GetMeshInfo(int* numMeshes, char** meshNames, char** parents, char** skinned);

	/// get mesh material info from fbx scene
	static bool GetMeshMaterials(const nvidia::Char* meshName, int *numMaterials, MeshMaterial** materials);

	/// get hair directly from fbx
//	static bool CreateHairFromFbx(const char* guideName, const char* growthMeshName, NvHair::AssetDescriptor &hairAsset);
};

