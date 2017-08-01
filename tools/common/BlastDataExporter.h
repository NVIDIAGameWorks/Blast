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
// Copyright (c) 2017 NVIDIA Corporation. All rights reserved.


#ifndef BLAST_DATA_EXPORTER
#define BLAST_DATA_EXPORTER


#include <NvBlastIndexFns.h>
#include <NvBlastExtAuthoringTypes.h>
#include <NvBlastExtPxAsset.h>
#include <vector>
#include <string>

using namespace Nv::Blast;

namespace physx
{
class PxPhysics;
class PxCooking;
}


struct NvBlastBondDesc;
struct NvBlastChunkDesc;

struct NvBlastAsset;
namespace Nv
{
namespace Blast
{
class TkAsset;
class ExtPxAsset;
class ExtSerialization;
}
}


/**
	Tool for Blast asset creation and exporting
*/
class BlastDataExporter
{
public:
	BlastDataExporter(TkFramework* framework, physx::PxPhysics* physics, physx::PxCooking* cooking);
	~BlastDataExporter();

	/**
		Creates ExtPxAsset
	*/
	ExtPxAsset*		createExtBlastAsset(std::vector<NvBlastBondDesc>& bondDescs, const std::vector<NvBlastChunkDesc>& chunkDescs,
		std::vector<ExtPxAssetDesc::ChunkDesc>& physicsChunks);
	/**
		Creates Low Level Blast asset 
	*/
	NvBlastAsset*	createLlBlastAsset(std::vector<NvBlastBondDesc>& bondDescs, const std::vector<NvBlastChunkDesc>& chunkDescs);

	/**
		Creates Blast Toolkit Asset asset 
	*/
	TkAsset*		createTkBlastAsset(const std::vector<NvBlastBondDesc>& bondDescs, const std::vector<NvBlastChunkDesc>& chunkDescs);

	/*
	Saves a Blast object to given path
	*/
	bool			saveBlastObject(const std::string& outputDir, const std::string& objectName, const void* object, uint32_t objectTypeID);

private:
	TkFramework*		mFramework;
	ExtSerialization*	mSerialization;
};




#endif