/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

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
		NO_BONDS = 0,
		X_BONDS = 1,
		Y_BONDS = 2,
		Z_BONDS = 4,
		ALL_BONDS = X_BONDS | Y_BONDS | Z_BONDS
	};


	struct Settings
	{
		Settings() : bondFlags(BondFlags::ALL_BONDS) {}

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
