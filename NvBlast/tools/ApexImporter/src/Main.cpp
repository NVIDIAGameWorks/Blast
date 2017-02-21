
#if NV_VC
#pragma warning(push)
#pragma warning(disable: 4996) // 'fopen' unsafe warning, from NxFileBuffer.h
#endif

#include "PxPhysicsAPI.h"
#include "Apex.h"
#include <ModuleDestructible.h>
#include <DestructibleAsset.h>
#include "NullRenderer.h"
#include "NvBlastExtApexImportTool.h"
#include "Log.h"
#include <string>
#include <iostream>
#include "tclap/CmdLine.h"
#include "ApexDestructibleObjExporter.h"
#include "NvBlastTkFramework.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlastExtPxManager.h"
#include <BlastDataExporter.h>
#include "windows.h"
#include <NvBlastTkAsset.h>
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
void run(const std::string& inFilepath, const std::string& outDir, const std::string& assetName, uint32_t mode, bool llFlag, bool tkFlag, bool extPxFlag, bool obj, bool fbx, bool fbxascii, bool ue4)
{
	std::string inputDir = inFilepath.substr(0, inFilepath.find_last_of("/\\"));

	// load APEX
	ApexImportTool blast(loggingCallback);

	lout() << Log::TYPE_INFO << "ApexImportTool initialization" << std::endl;
	blast.initialize();
	if (!blast.isValid())
	{
		lout() << Log::TYPE_ERROR << "Failed to create BlastSDK" << std::endl;
		return;
	}

	// load asset
	lout() << Log::TYPE_INFO << "Loading asset: " << inFilepath << std::endl;
	physx::PxFileBuf* apexAssetStream = nvidia::apex::GetApexSDK()->createStream(inFilepath.c_str(), physx::PxFileBuf::OPEN_READ_ONLY);
	nvidia::apex::DestructibleAsset* apexAsset = blast.loadAssetFromFile(apexAssetStream);
	if (!apexAsset)
	{
		return;
	}  
	apexAssetStream->release();

	ApexImporterConfig config;
	config.infSearchMode = static_cast<ApexImporterConfig::InterfaceSearchMode>(mode);

	std::vector<uint32_t> chunkReorderInvMap;
	TkFrameworkDesc frameworkDesc =
	{
		nvidia::apex::GetApexSDK()->getErrorCallback(),
		nvidia::apex::GetApexSDK()->getAllocator()
	};
	TkFramework* framework = NvBlastTkFrameworkCreate(frameworkDesc);
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

	bool result = blast.importApexAsset(chunkReorderInvMap, apexAsset, chunkDesc, bondDescs, flags, config);
	if (!result)
	{
		lout() << Log::TYPE_ERROR << "Failed to build Blast asset data" << std::endl;
		return;
	};

	result = blast.getCollisionGeometry(apexAsset, static_cast<uint32_t>(chunkDesc.size()), chunkReorderInvMap, flags, physicsChunks, physicsSubchunks);
	if (!result)
	{
		lout() << Log::TYPE_ERROR << "Failed to build physics data" << std::endl;
		return;
	};


	// save asset
	lout() << Log::TYPE_INFO << "Saving blast asset: " << outDir << std::endl;
	BlastDataExporter blExpr(framework, nvidia::apex::GetApexSDK()->getCookingInterface(), loggingCallback);
	NvBlastAsset* llAsset = blExpr.createLlBlastAsset(bondDescs, chunkDesc);

	std::cout <<"Chunk count: " << NvBlastAssetGetChunkCount(llAsset, NULL) << std::endl;

	if (llAsset == nullptr && (llFlag || fbx))
	{
		lout() << Log::TYPE_ERROR << "Failed to build low-level asset" << std::endl;
		return;
	}

	if (llFlag)
	{
		std::ostringstream outBlastFilePathStream;
		outBlastFilePathStream << outDir << "/" << assetName << ".llasset";
		std::string outBlastFilePath = outBlastFilePathStream.str();
		blExpr.saveBlastLLAsset(outBlastFilePath, llAsset);
		std::cout << "Wrote NvBlastAsset to " << outBlastFilePath << std::endl;
	}
	if (tkFlag)
	{
		TkAsset* tkAsset = blExpr.createTkBlastAsset(bondDescs, chunkDesc);
		std::ostringstream outBlastFilePathStream;
		outBlastFilePathStream << outDir << "/" << assetName << ".tkasset";
		std::string outBlastFilePath = outBlastFilePathStream.str();
		blExpr.saveBlastTkAsset(outBlastFilePath, tkAsset);
		std::cout << "Wrote TkAsset to " << outBlastFilePath << std::endl;
		tkAsset->release();
	}
	if (extPxFlag)
	{
		ExtPxAsset* pxAsset = blExpr.createExtBlastAsset(bondDescs, chunkDesc, physicsChunks);
		std::ostringstream outBlastFilePathStream;
		outBlastFilePathStream << outDir << "/" << assetName << ".bpxa";
		std::string outBlastFilePath = outBlastFilePathStream.str();
		blExpr.saveBlastExtAsset(outBlastFilePath, pxAsset);
		std::cout << "Wrote ExtPxAsset to " << outBlastFilePath << std::endl;
		pxAsset->release();
	}	
	
	lout() << Log::TYPE_INFO << "Saving model file: " << outDir << std::endl;
	ApexDestructibleGeometryExporter objSaver(inputDir, outDir);
	objSaver.exportToFile(llAsset, *apexAsset, assetName, chunkReorderInvMap, fbx, obj, fbxascii, ue4);
	_aligned_free(llAsset);
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

		TCLAP::SwitchArg bpxaOutputArg("", "bpxa", "Output ExtPxAsset to the output directory (ext: bpxa)", false);
		cmd.add(bpxaOutputArg);

		TCLAP::SwitchArg tkOutputArg("", "tk", "Output TkAsset to the output directory (ext: tkasset)", false);
		cmd.add(tkOutputArg);

		TCLAP::SwitchArg llOutputArg("", "ll", "Output LL Blast asset to the output directory (ext: llasset)", false);
		cmd.add(llOutputArg);

		TCLAP::SwitchArg ue4OutputArg("", "ue4", "Output FBX with UE4 coordinate system", false);
		cmd.add(ue4OutputArg);

		TCLAP::SwitchArg fbxAsciiArg("", "fbxascii", "Output FBX as an ascii file (defaults to binary output)", false);
		cmd.add(fbxAsciiArg);

		TCLAP::SwitchArg objOutputArg("", "obj", "Output a OBJ mesh to the output directory", false);
		cmd.add(objOutputArg);

		TCLAP::SwitchArg fbxOutputArg("", "fbx", "Output a FBX mesh to the output directory", false);
		cmd.add(fbxOutputArg);


		// parse cmd input
		cmd.parse(argc, argv);

		bool bOutputBPXA = bpxaOutputArg.getValue();
		bool bOutputTK = tkOutputArg.getValue();
		bool bOutputLL = llOutputArg.getValue();

		bool bUE4CoordSystem = ue4OutputArg.isSet();
		bool bOutputFBXAscii = fbxAsciiArg.isSet();

		bool bOutputObjFile = objOutputArg.isSet();
		bool bOutputFbxFile = fbxOutputArg.isSet();


		// Did we specify no output formats?
		if (!bpxaOutputArg.isSet() && !tkOutputArg.isSet() && !llOutputArg.isSet())
		{
			std::cout << "Didn't specify an output format on the command line, so defaulting to outputting BPXA" << std::endl;
			bOutputBPXA = true;
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

		run(infile, outDir, assetName, mode, bOutputLL, bOutputTK, bOutputBPXA, bOutputObjFile, bOutputFbxFile, bOutputFBXAscii, bUE4CoordSystem);
	}
	catch (TCLAP::ArgException &e)  // catch any exceptions
	{
		lout() << Log::TYPE_ERROR << "error: " << e.error() << " for arg " << e.argId() << std::endl;
	}

	return 0;
}
