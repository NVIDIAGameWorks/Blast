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
// Copyright (c) 2016-2018 NVIDIA Corporation. All rights reserved.


#if NV_VC
#pragma warning(push)
#pragma warning(disable: 4996) // 'fopen' unsafe warning, from NxFileBuffer.h
#endif

#include "PxPhysicsAPI.h"
#include "NvBlastExtApexImportTool.h"
#include "Log.h"
#include <string>
#include <iostream>
#include <fstream>
#include "tclap/CmdLine.h"
#include "ApexDestructibleObjExporter.h"
#include "NvBlastTkFramework.h"
#include "NvBlastGlobals.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlastExtPxManager.h"
#include "BlastDataExporter.h"
#include "windows.h"
#include <NvBlastTkAsset.h>
#include "NvBlastExtLlSerialization.h"
#include "NvBlastExtTkSerialization.h"
#include "NvBlastExtPxSerialization.h"
#include "NvBlastExtExporterJsonCollision.h"
#include <nvparameterized\NvSerializer.h>
#include <PsFileBuffer.h>
#define DEFAULT_INPUT_FILE "../../../tools/ApexImporter/resources/assets/table.apb"
#define DEFAULT_OUTPUT_DIR "C:/TestFracturer/"
#define DEFAULT_ASSET_NAME "table"

using namespace Nv::Blast;
using namespace Nv::Blast::ApexImporter;

void loggingCallback(int type, const char* msg, const char* file, int line)
{
	if (type == NvBlastMessage::Info)
		lout() << Log::TYPE_INFO << msg << " FILE:" << file << " Line: " << line << "\n";
	else
		lout() << Log::TYPE_ERROR << msg << " FILE:" << file << " Line: " << line << "\n";
}

bool isDirectoryExist(std::string path)
{
	DWORD attributes = GetFileAttributesA(path.c_str());
	if ((attributes != INVALID_FILE_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		return true;
	}
	return false;
}

bool mkDirRecursively(std::string path)
{
	if (isDirectoryExist(path))
	{
		return true;
	}
	auto indx = path.find_first_of("\\/");
	while (indx != std::string::npos)
	{
		std::string subfolder = path.substr(0, indx);
		CreateDirectory(subfolder.c_str(), NULL);
		indx = path.find_first_of("\\/", indx + 1);
	}
	return isDirectoryExist(path);
}

// Create an Asset from the APEX destructible asset
void run(const std::string& inFilepath, const std::string& outDir, const std::string& assetName, uint32_t mode, bool llFlag, bool tkFlag, bool extPxFlag,
		 bool obj, bool fbx, bool fbxascii, bool fbxCollision, bool jsonCollision, bool nonSkinned)
{
	std::string inputDir = inFilepath.substr(0, inFilepath.find_last_of("/\\"));

	// load APEX
	ApexImportTool blast;

	lout() << Log::TYPE_INFO << "ApexImportTool initialization" << std::endl;
	if (!blast.isValid())
	{
		lout() << Log::TYPE_ERROR << "Failed to create BlastSDK" << std::endl;
		return;
	}


	// load asset
	lout() << Log::TYPE_INFO << "Loading asset: " << inFilepath << std::endl;
	physx::PxFileBuf* apexAssetStream = PX_NEW(physx::general_PxIOStream::PsFileBuffer)(inFilepath.c_str(), physx::PxFileBuf::OPEN_READ_ONLY);

	NvParameterized::Serializer::DeserializedData data;
	blast.loadAssetFromFile(apexAssetStream, data);
	if (data.size() == 0)
	{
		return;
	}  
	apexAssetStream->release();

	ApexImporterConfig config;
	config.infSearchMode = static_cast<ApexImporterConfig::InterfaceSearchMode>(mode);

	std::vector<uint32_t> chunkReorderInvMap;
	TkFramework* framework = NvBlastTkFrameworkCreate();
	if (framework == nullptr)
	{
		lout() << Log::TYPE_ERROR << "Failed to create TkFramework" << std::endl;
		return;
	}

	std::vector<NvBlastChunkDesc>	chunkDesc;
	std::vector<NvBlastBondDesc>	bondDescs;
	std::vector<uint32_t>			flags;

	std::vector<ExtPxAssetDesc::ChunkDesc> physicsChunks;
	std::vector<ExtPxAssetDesc::SubchunkDesc> physicsSubchunks;

	bool result = blast.importApexAsset(chunkReorderInvMap, data[0], chunkDesc, bondDescs, flags, config);
	if (!result)
	{
		lout() << Log::TYPE_ERROR << "Failed to build Blast asset data" << std::endl;
		return;
	};
	std::vector<std::vector<CollisionHull*>> hulls;
	result = blast.getCollisionGeometry(data[0], static_cast<uint32_t>(chunkDesc.size()), chunkReorderInvMap, flags, physicsChunks, physicsSubchunks, hulls);
	if (!result)
	{
		lout() << Log::TYPE_ERROR << "Failed to build physics data" << std::endl;
		return;
	};


	// save asset
	lout() << Log::TYPE_INFO << "Saving blast asset: " << outDir << std::endl;
	BlastDataExporter blExpr(framework, blast.getPxSdk(), blast.getCooking());
	NvBlastAsset* llAsset = blExpr.createLlBlastAsset(bondDescs, chunkDesc);

	std::cout <<"Chunk count: " << NvBlastAssetGetChunkCount(llAsset, NULL) << std::endl;

	if (llAsset == nullptr && (llFlag || fbx))
	{
		lout() << Log::TYPE_ERROR << "Failed to build low-level asset" << std::endl;
		return;
	}

	if (llFlag)
	{
		blExpr.saveBlastObject(outDir, assetName, llAsset, LlObjectTypeID::Asset);
	}
	if (tkFlag)
	{
		TkAsset* tkAsset = blExpr.createTkBlastAsset(bondDescs, chunkDesc);
		blExpr.saveBlastObject(outDir, assetName, tkAsset, TkObjectTypeID::Asset);
		tkAsset->release();
	}
	if (extPxFlag)
	{
		ExtPxAsset* pxAsset = blExpr.createExtBlastAsset(bondDescs, chunkDesc, physicsChunks);
		blExpr.saveBlastObject(outDir, assetName, pxAsset, ExtPxObjectTypeID::Asset);
		pxAsset->release();
	}	
	
	lout() << Log::TYPE_INFO << "Saving model file: " << outDir << std::endl;
	ApexDestructibleGeometryExporter objSaver(inputDir, outDir);
	if (fbxCollision)
	{
		objSaver.exportToFile(llAsset, data[0], blast, assetName, chunkReorderInvMap, fbx, obj, fbxascii, nonSkinned, hulls);
	}
	else
	{
		objSaver.exportToFile(llAsset, data[0], blast, assetName, chunkReorderInvMap, fbx, obj, fbxascii, nonSkinned);
	}

	if (jsonCollision)
	{
		std::vector<CollisionHull*> flattenedHulls;
		std::vector<uint32_t> hullOffsets;
		for (std::vector<CollisionHull*>& chunkHulls : hulls)
		{
			hullOffsets.push_back(static_cast<uint32_t>(flattenedHulls.size()));
			for (CollisionHull* hull : chunkHulls)
			{
				flattenedHulls.push_back(hull);
			}
		}
		hullOffsets.push_back(static_cast<uint32_t>(flattenedHulls.size()));
		const std::string fullJsonFilename = outDir + ((outDir.back() == '/' || outDir.back() == '\\') ? "" : "/") + assetName + ".json";
		IJsonCollisionExporter* collisionExporter = NvBlastExtExporterCreateJsonCollisionExporter();
		if (collisionExporter != nullptr)
		{
			if (collisionExporter->writeCollision(fullJsonFilename.c_str(), static_cast<uint32_t>(hulls.size()), hullOffsets.data(), flattenedHulls.data()))
			{
				std::cout << "Exported collision geometry: " << fullJsonFilename << std::endl;
			}
			else
			{
				std::cerr << "Can't write collision geometry to json file." << std::endl;
			}
			collisionExporter->release();
		}
	}

	NVBLAST_FREE(llAsset);
}


int main(int argc, const char* const* argv)
{
	try 
	{
		// setup cmd line
		TCLAP::CmdLine cmd("Blast SDK: APEX Importer", ' ', "0.1");

		TCLAP::ValueArg<std::string> infileArg("f", "file", "File to load", true, DEFAULT_INPUT_FILE, "infile");
		cmd.add(infileArg);

		TCLAP::ValueArg<std::string> outDirArg("o", "outputDir", "Output directory", false, DEFAULT_OUTPUT_DIR, "output directory");
		cmd.add(outDirArg);

		TCLAP::ValueArg<std::string> outAssetName("n", "outAssetName", "Output asset name", true, DEFAULT_ASSET_NAME, "output asset name");
		cmd.add(outAssetName);


		TCLAP::ValueArg<uint32_t> interfaceSearchMode("m", "mode", "Interface search mode", false, 0, "0 - EXACT, 1 - FORCED, for detailed description see docs.");
		cmd.add(interfaceSearchMode);

		TCLAP::SwitchArg debugSwitch("d", "debug", "Print debug output", cmd, false);

		TCLAP::SwitchArg pxOutputArg("", "px", "Output ExtPxAsset to the output directory", false);
		cmd.add(pxOutputArg);

		TCLAP::SwitchArg tkOutputArg("", "tk", "Output TkAsset to the output directory", false);
		cmd.add(tkOutputArg);

		TCLAP::SwitchArg llOutputArg("", "ll", "Output LL Blast asset to the output directory", false);
		cmd.add(llOutputArg);

		TCLAP::SwitchArg fbxAsciiArg("", "fbxascii", "Output FBX as an ascii file (defaults to binary output)", false);
		cmd.add(fbxAsciiArg);

		TCLAP::SwitchArg objOutputArg("", "obj", "Output a OBJ mesh to the output directory", false);
		cmd.add(objOutputArg);

		TCLAP::SwitchArg fbxOutputArg("", "fbx", "Output a FBX mesh to the output directory", false);
		cmd.add(fbxOutputArg);

		TCLAP::SwitchArg jsonCollision("", "jsoncollision", "Save collision geometry to a json file", false);
		cmd.add(jsonCollision);

		TCLAP::SwitchArg fbxcollision("", "fbxcollision", "Append collision geometry to FBX file", false);
		cmd.add(fbxcollision);

		TCLAP::SwitchArg nonSkinnedFBX("", "nonskinned", "Output a non-skinned FBX file", false);
		cmd.add(nonSkinnedFBX);

		// parse cmd input
		cmd.parse(argc, argv);

		bool bOutputPX = pxOutputArg.getValue();
		bool bOutputTK = tkOutputArg.getValue();
		bool bOutputLL = llOutputArg.getValue();

		bool bOutputFBXAscii = fbxAsciiArg.isSet();

		bool bOutputObjFile = objOutputArg.isSet();
		bool bOutputFbxFile = fbxOutputArg.isSet();

		bool bFbxCollision = fbxcollision.isSet();
		bool bJsonCollision = jsonCollision.isSet();

		bool bNonSkinned = nonSkinnedFBX.isSet();

		// Did we specify no output formats?
		if (!pxOutputArg.isSet() && !tkOutputArg.isSet() && !llOutputArg.isSet())
		{
			std::cout << "Didn't specify an output format on the command line, so defaulting to outputting an ExtPxAsset" << std::endl;
			bOutputPX = true;
		}
		// Did we specify no geometry output formats?
		if (!bOutputObjFile && !bOutputFbxFile)
		{
			std::cout << "Didn't specify an output geometry format on the command line, so defaulting to outputting .OBJ" << std::endl;
			bOutputObjFile = true;
		}

		// get cmd parse results
		std::string infile = infileArg.getValue();
		std::string outDir;
		std::string assetName = outAssetName.getValue();
		
		/**
			Set output dir, make sure that it exist.
		*/
		if (outDirArg.isSet())
		{
			outDir = outDirArg.getValue();
			std::string temp = outDir + '/';
			if (!isDirectoryExist(outDir.data()))
			{
				std::cout << "Output directory doesn't exist. It will be created." << std::endl;
				if (!mkDirRecursively(temp.data()))
				{
					std::cout << "Directory creation failed!" << std::endl;
					return -1;
				}
			}
		}
		else
		{
			auto idx = infile.find_last_of("/\\");
			if (idx == 0 || idx == std::string::npos)
			{
				outDir = ".";
			}
			else
			{
				outDir = infile.substr(0, idx);
			}
		}

		bool debug = debugSwitch.getValue();
		int mode = interfaceSearchMode.getValue();
		// do stuff
		if (debug)
			lout().setMinVerbosity(Log::MOST_VERBOSE);

		run(infile, outDir, assetName, mode, bOutputLL, bOutputTK, bOutputPX, bOutputObjFile, bOutputFbxFile, bOutputFBXAscii, bFbxCollision, bJsonCollision, bNonSkinned);
	}
	catch (TCLAP::ArgException &e)  // catch any exceptions
	{
		lout() << Log::TYPE_ERROR << "error: " << e.error() << " for arg " << e.argId() << std::endl;
	}

	return 0;
}
