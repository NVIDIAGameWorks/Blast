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


#ifndef ASSETGENERATOR_H
#define ASSETGENERATOR_H


#include "NvBlast.h"

#include <vector>
#include <cmath>

class GeneratorAsset
{
public:
	struct Vec3
	{
		float x, y, z;

		Vec3() {}
		Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
		Vec3 operator * (float v) const { return Vec3(x * v, y * v, z * v); }
		Vec3 operator * (const Vec3& v) const { return Vec3(x * v.x, y * v.y, z * v.z); }
		Vec3 operator + (const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
		Vec3 operator - (const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
		Vec3 getNormalized() const
		{
			return (*this)*(1.0f / sqrt(x*x + y*y + z*z));
		}
	};

	struct BlastChunkCube
	{
		BlastChunkCube(Vec3 position_, Vec3 extents_)
		{
			position = position_;
			extents = extents_;
		}

		Vec3 position;
		Vec3 extents;
	};

	std::vector<NvBlastChunkDesc> solverChunks;
	std::vector<NvBlastBondDesc> solverBonds;
	std::vector<BlastChunkCube> chunks;
	Vec3 extents;
};


class CubeAssetGenerator
{
public:
	struct DepthInfo
	{
		DepthInfo(GeneratorAsset::Vec3 slices = GeneratorAsset::Vec3(1, 1, 1), NvBlastChunkDesc::Flags flag_ = NvBlastChunkDesc::Flags::NoFlags) 
			: slicesPerAxis(slices), flag(flag_) {}

		GeneratorAsset::Vec3 slicesPerAxis;
		NvBlastChunkDesc::Flags flag;
	};

	enum BondFlags
	{
		NO_BONDS            = 0,
		X_BONDS             = 1 << 0,
		Y_BONDS             = 1 << 1,
		Z_BONDS             = 1 << 2,
		X_PLUS_WORLD_BONDS  = 1 << 3,
		X_MINUS_WORLD_BONDS = 1 << 4,
		Y_PLUS_WORLD_BONDS  = 1 << 5,
		Y_MINUS_WORLD_BONDS = 1 << 6,
		Z_PLUS_WORLD_BONDS  = 1 << 7,
		Z_MINUS_WORLD_BONDS = 1 << 8,
		ALL_INTERNAL_BONDS  = X_BONDS | Y_BONDS | Z_BONDS
	};


	struct Settings
	{
		Settings() : bondFlags(BondFlags::ALL_INTERNAL_BONDS) {}

		std::vector<DepthInfo> depths;
		GeneratorAsset::Vec3 extents;
		BondFlags bondFlags;
	};

	static void generate(GeneratorAsset& asset, const Settings& settings);
private:
	static void fillBondDesc(std::vector<NvBlastBondDesc>& bondDescs, uint32_t id0, uint32_t id1, GeneratorAsset::Vec3 pos0, GeneratorAsset::Vec3 pos1, GeneratorAsset::Vec3 size, float area);
};


inline CubeAssetGenerator::BondFlags operator | (CubeAssetGenerator::BondFlags a, CubeAssetGenerator::BondFlags b)
{
	return static_cast<CubeAssetGenerator::BondFlags>(static_cast<int>(a) | static_cast<int>(b));
}

#endif // #ifndef ASSETGENERATOR_H
