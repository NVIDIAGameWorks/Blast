/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

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
			jointAllBonds(false), extents(20, 20, 20)
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
	};

	struct ModelAsset
	{
		ModelAsset() : isSkinned(false), transform(physx::PxIdentity) 
		{}

		std::string			id;
		std::string			file;
		std::string			name;
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