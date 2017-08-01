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
// Copyright (c) 2008-2017 NVIDIA Corporation. All rights reserved.


#ifndef SAMPLE_H
#define SAMPLE_H

#include "PxTransform.h"
#include <string>
#include <vector>


struct AssetList
{
	struct BoxAsset
	{
		BoxAsset() : staticHeight(-std::numeric_limits<float>().infinity()), 
			jointAllBonds(false), extents(20, 20, 20), bondFlags(7)
		{}

		struct Level
		{
			Level() :x(0), y(0), z(0), isSupport(0) {};

			int				x, y, z;
			bool			isSupport;
		};

		std::string			id;
		std::string			name;
		physx::PxVec3		extents;
		float				staticHeight;
		bool				jointAllBonds;
		std::vector<Level>	levels;
		uint32_t			bondFlags;
	};

	struct ModelAsset
	{
		ModelAsset() : isSkinned(false), transform(physx::PxIdentity) 
		{}

		std::string			id;
		std::string			file;
		std::string			name;
		std::string			fullpath;
		physx::PxTransform	transform;
		bool				isSkinned;
	};

	struct CompositeAsset
	{
		CompositeAsset() : transform(physx::PxIdentity)
		{}

		struct AssetRef
		{
			std::string			id;
			physx::PxTransform	transform;
		};

		struct Joint
		{
			int32_t				assetIndices[2];
			uint32_t			chunkIndices[2];
			physx::PxVec3		attachPositions[2];
		};

		std::string				id;
		std::string				name;
		physx::PxTransform		transform;
		std::vector<AssetRef>	assetRefs;
		std::vector<Joint>		joints;
	};

	std::vector<ModelAsset>		models;
	std::vector<CompositeAsset>	composites;
	std::vector<BoxAsset>		boxes;
};

struct SampleConfig
{
	std::wstring			sampleName;
	std::string				assetsFile;
	std::vector<std::string> additionalResourcesDir;
	AssetList				additionalAssetList;
};

int runSample(const SampleConfig& config);

#endif //SAMPLE_H