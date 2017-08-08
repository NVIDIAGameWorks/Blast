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


#include "PxPhysicsAPI.h"
#include "PsFileBuffer.h"
#include "NvBlast.h"
#include "NvBlastAssert.h"
#include "NvBlastGlobals.h"
#include "NvBlastExtExporter.h"
#include "NvBlastPxCallbacks.h"
#include "NvBlastTkAsset.h"
#include "NvBlastExtLlSerialization.h"
#include "NvBlastExtTkSerialization.h"
#include "NvBlastExtPxSerialization.h"
#include "NvBlastExtAuthoring.h"
#include "NvBlastExtAuthoringMesh.h"
#include "NvBlastExtAuthoringBondGenerator.h"
#include "NvBlastExtAuthoringCollisionBuilder.h"
#include "NvBlastExtAuthoringFractureTool.h"
#include "BlastDataExporter.h"
#include "SimpleRandomGenerator.h"
#include "NvBlastExtAuthoringMeshCleaner.h"

#include <string>
#include <memory>
#include <iostream>
#include <vector>
#include <cctype>
#include <fstream>
#include <iosfwd>
#include <Windows.h>
#include "tclap/CmdLine.h"

using physx::PxVec3;
using physx::PxVec2;

#define DEFAULT_ASSET_NAME "AuthoringTest"

using namespace Nv::Blast;

physx::PxFoundation*	gFoundation = nullptr;
physx::PxPhysics*		gPhysics = nullptr;
physx::PxCooking*		gCooking = nullptr;

struct TCLAPint3
{
	int32_t x, y, z;
	TCLAPint3(int32_t x, int32_t y, int32_t z) :x(x), y(y), z(z){};
	TCLAPint3() :x(0), y(0), z(0){};
	TCLAPint3& operator=(const std::string &inp)
	{
		std::istringstream stream(inp);
		if (!(stream >> x >> y >> z))
			throw TCLAP::ArgParseException(inp + " is not int3");
		return *this;
	}
};

namespace TCLAP {
	template<>
	struct ArgTraits<TCLAPint3> {
		typedef StringLike ValueCategory;
	};
}

bool isDirectoryExist(const std::string& path)
{
	DWORD attributes = GetFileAttributesA(path.c_str());
	if ((attributes != INVALID_FILE_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		return true;
	}
	return false;
}

bool isFileExist(const std::string& path)
{
	DWORD attributes = GetFileAttributesA(path.c_str());
	if ((attributes != INVALID_FILE_ATTRIBUTES) && !(attributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		return true;
	}
	return false;
}

bool mkDirRecursively(const std::string& path)
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

bool initPhysX()
{
	gFoundation = PxCreateFoundation(PX_FOUNDATION_VERSION, NvBlastGetPxAllocatorCallback(), NvBlastGetPxErrorCallback());
	if (!gFoundation)
	{
		std::cerr << "Can't init PhysX foundation" << std::endl;
		return false;
	}
	physx::PxTolerancesScale scale;
	gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, scale, true);
	if (!gPhysics)
	{
		std::cerr << "Can't create Physics" << std::endl;
		return false;
	}
	physx::PxCookingParams cookingParams(scale);
	cookingParams.buildGPUData = true;
	gCooking = PxCreateCooking(PX_PHYSICS_VERSION, gPhysics->getFoundation(), cookingParams);
	if (!gCooking)
	{
		std::cerr << "Can't create Cooking" << std::endl;
		return false;
	}
	return true;
}

int main(int argc, const char* const* argv)
{
	// set blast global error callback
	// overriding default one in order to exit tool in profile/release configuration too (and write to stderr)
	class CustomErrorCallback : public ErrorCallback
	{
		virtual void reportError(ErrorCode::Enum code, const char* msg, const char* file, int line) override
		{
			std::stringstream str;
			bool critical = false;
			switch (code)
			{
			case ErrorCode::eNO_ERROR:			str << "[Info]";				critical = false; break;
			case ErrorCode::eDEBUG_INFO:		str << "[Debug Info]";			critical = false; break;
			case ErrorCode::eDEBUG_WARNING:		str << "[Debug Warning]";		critical = false; break;
			case ErrorCode::eINVALID_PARAMETER:	str << "[Invalid Parameter]";	critical = true;  break;
			case ErrorCode::eINVALID_OPERATION:	str << "[Invalid Operation]";	critical = true;  break;
			case ErrorCode::eOUT_OF_MEMORY:		str << "[Out of] Memory";		critical = true;  break;
			case ErrorCode::eINTERNAL_ERROR:	str << "[Internal Error]";		critical = true;  break;
			case ErrorCode::eABORT:				str << "[Abort]";				critical = true;  break;
			case ErrorCode::ePERF_WARNING:		str << "[Perf Warning]";		critical = false; break;
			default:							NVBLAST_ASSERT(false);
			}
#if NV_DEBUG || NV_CHECKED
			str << file << "(" << line << "): ";
#else 
			NV_UNUSED(file);
			NV_UNUSED(line);
#endif				
			str << " " << msg << "\n";
			std::cerr << str.str();

			if (critical)
			{
				std::cerr << "Authoring failed. Exiting.\n";
				exit(-1);
			}
		}
	};
	CustomErrorCallback errorCallback;
	NvBlastGlobalSetErrorCallback(&errorCallback);

	// setup cmd line
	TCLAP::CmdLine cmd("Blast SDK: Authoring Tool", ' ', "0.1");

	TCLAP::UnlabeledValueArg<std::string> infileArg("file", "File to load", true, "", "infile");
	cmd.add(infileArg);

	TCLAP::UnlabeledValueArg<std::string> outAssetName("outAssetName", "Output asset name", true, DEFAULT_ASSET_NAME, "output asset name");
	cmd.add(outAssetName);

	TCLAP::ValueArg<std::string> outDirArg("", "outputDir", "Output directory", false, ".", "by default directory of the input file");
	cmd.add(outDirArg);

	TCLAP::SwitchArg cleanArg("", "clean", "Try clean mesh before fracturing", false);
	cmd.add(cleanArg);

	// The output modes
	//NOTE: Fun TCLAP quirk here - if you set the default to true and specify this switch on the command line, the value will be false!
	TCLAP::SwitchArg bpxaOutputArg("", "bpxa", "Output ExtPxAsset to the output directory (ext: bpxa)", false);
	cmd.add(bpxaOutputArg);

	TCLAP::SwitchArg tkOutputArg("", "tk", "Output TkAsset to the output directory (ext: tkasset)", false);
	cmd.add(tkOutputArg);

	TCLAP::SwitchArg llOutputArg("", "ll", "Output LL Blast asset to the output directory (ext: llasset)", false);
	cmd.add(llOutputArg);

	TCLAP::SwitchArg fbxAsciiArg("", "fbxascii", "Output FBX as an ascii file (defaults to binary output)", false);
	cmd.add(fbxAsciiArg);

 	TCLAP::SwitchArg objOutputArg("", "obj", "Output a OBJ mesh to the output directory", false);
 	cmd.add(objOutputArg);

	TCLAP::SwitchArg fbxOutputArg("", "fbx", "Output a FBX mesh to the output directory", false);
	cmd.add(fbxOutputArg);

	TCLAP::SwitchArg protoSer("", "proto", "Serialize Blast data with CapnProto", false);
	cmd.add(protoSer);

	TCLAP::SwitchArg blockSer("", "block", "Serialize Blast data as block of memory", false);
	cmd.add(blockSer);

	TCLAP::SwitchArg fbxCollision("", "fbxcollision", "Add collision geometry to FBX file", false);
	cmd.add(fbxCollision);

	TCLAP::SwitchArg nonSkinnedFBX("", "nonskinned", "Output a non-skinned FBX file", false);
	cmd.add(nonSkinnedFBX);



	TCLAP::ValueArg<unsigned char> fracturingMode("", "mode", "Fracturing mode", false, 'v', "v - voronoi, c - clustered voronoi, s - slicing.");
	cmd.add(fracturingMode);
	TCLAP::ValueArg<uint32_t> cellsCount("", "cells", "Voronoi cells count", false, 5, "by default 5");
	cmd.add(cellsCount);
	TCLAP::ValueArg<uint32_t> clusterCount("", "clusters", "Uniform Voronoi cluster count", false, 5, "by default 5");
	cmd.add(clusterCount);
	TCLAP::ValueArg<float> clusterRad("", "radius", "Clustered Voronoi cluster radius", false, 1.0f, "by default 1.0");
	cmd.add(clusterRad);

	TCLAP::ValueArg<TCLAPint3> slicingNumber("", "slices", "Number of slices per direction", false, TCLAPint3(1, 1, 1), "by default 1 1 1");
	cmd.add(slicingNumber);

	TCLAP::ValueArg<float> angleVariation("", "avar", "Slicing angle variation", false, 0.0, "by default 0.0");
	cmd.add(angleVariation);

	TCLAP::ValueArg<float> offsetVariation("", "ovar", "Slicing offset variation", false, 0.0, "by default 0.0");
	cmd.add(offsetVariation);

	try
	{
		// parse cmd input
		cmd.parse(argc, argv);
	}
	catch (TCLAP::ArgException &e)  // catch any exceptions
	{
		std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
		return -1;
	}

	// get cmd parse results
	std::string infile = infileArg.getValue();
	if (!isFileExist(infile))
	{
		std::cerr << "[Error] Can't fine input file: " << infile << std::endl;
		return -1;
	}

	std::string outDir;
	
	std::cout << "Input file: " << infile << std::endl;

	if (outDirArg.isSet())
	{
		outDir = outDirArg.getValue();
		std::string temp = outDir + '/';
		if (!isDirectoryExist(outDir.data()))
		{
			std::cout << "Output directory doesn't exist. It will be created." << std::endl;
			if (!mkDirRecursively(temp.data()))
			{
				std::cerr << "Directory creation failed!" << std::endl;
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
	std::string assetName = outAssetName.getValue();

	// Determine whether to use the obj or fbx loader

	auto idx = infile.rfind('.');
	std::string extension;
	

	if (idx != std::string::npos)
	{
		extension = infile.substr(idx + 1);
		std::transform(extension.begin(), extension.end(), extension.begin(), std::toupper);
	}
	else
	{
		std::cerr << "Can't determine extension (and thus, loader) of input file. " << infile << std::endl;
		return -1;
	}

	bool bOutputBPXA = bpxaOutputArg.getValue();
	bool bOutputTK = tkOutputArg.getValue();
	bool bOutputLL = llOutputArg.getValue();

	bool bOutputFBXAscii = fbxAsciiArg.getValue();

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
		std::cout << "Didn't specify an output geometry format on the command line, so defaulting to outputting .FBX" << std::endl;
		bOutputFbxFile = true;
	}

	std::shared_ptr<IMeshFileReader> fileReader;

	if (extension.compare("FBX")==0)
	{
		fileReader = std::shared_ptr<IMeshFileReader>(NvBlastExtExporterCreateFbxFileReader(), [](IMeshFileReader* p) {p->release(); });
	}
	else if (extension.compare("OBJ")==0)
	{
		fileReader = std::shared_ptr<IMeshFileReader>(NvBlastExtExporterCreateObjFileReader(), [](IMeshFileReader* p) {p->release(); });
	}
	else
	{
		std::cout << "Unsupported file extension " << extension << std::endl;
		return -1;
	}
	
	// Load the asset
	fileReader->loadFromFile(infile.c_str());

	uint32_t vcount = fileReader->getVerticesCount();
	//uint32_t ncount = (uint32_t)fileReader->getNormalsArray().size();
	//uint32_t uvcount = (uint32_t)fileReader->getUvArray().size();

	if (!initPhysX())
	{
		std::cout << "Failed to initialize PhysX" << std::endl;
		return -1;
	}
	Nv::Blast::FractureTool* fTool = NvBlastExtAuthoringCreateFractureTool();

	PxVec3* pos = fileReader->getPositionArray();
	PxVec3* norm = fileReader->getNormalsArray();
	PxVec2* uv = fileReader->getUvArray();

	Nv::Blast::Mesh* mesh = NvBlastExtAuthoringCreateMesh(pos, norm, uv, vcount, fileReader->getIndexArray(), fileReader->getIdicesCount());

	if (cleanArg.isSet())
	{
		MeshCleaner* clr = NvBlastExtAuthoringCreateMeshCleaner();
		Nv::Blast::Mesh* nmesh;
		nmesh = clr->cleanMesh(mesh);
		clr->release();
		mesh->release();
		mesh = nmesh;
	}
	mesh->setMaterialId(fileReader->getMaterialIds());
	mesh->setSmoothingGroup(fileReader->getSmoothingGroups());

	fTool->setSourceMesh(mesh);
	

	SimpleRandomGenerator rng;
	Nv::Blast::VoronoiSitesGenerator* voronoiSitesGenerator = NvBlastExtAuthoringCreateVoronoiSitesGenerator(mesh, &rng);
	if (voronoiSitesGenerator == nullptr)
	{
		std::cerr << "Failed to create Voronoi sites generator" << std::endl;
		return -1;
	}

	// Send it to the fracture processor
	switch (fracturingMode.getValue())
	{
		case 'v':
		{
			voronoiSitesGenerator->uniformlyGenerateSitesInMesh(cellsCount.getValue());
			const physx::PxVec3* sites = nullptr;
			uint32_t sitesCount = voronoiSitesGenerator->getVoronoiSites(sites);
			if (fTool->voronoiFracturing(0, sitesCount, sites, false) != 0)
			{
				std::cerr << "Failed to fracture with Voronoi" << std::endl;
				return -1;
			}
			break;
		}
		case 'c':
		{
			voronoiSitesGenerator->clusteredSitesGeneration(cellsCount.getValue(), clusterCount.getValue(), clusterRad.getValue());
			const physx::PxVec3* sites = nullptr;
			uint32_t sitesCount = voronoiSitesGenerator->getVoronoiSites(sites);
			if (fTool->voronoiFracturing(0, sitesCount, sites, false) != 0)
			{
				std::cerr << "Failed to fracture with Clustered Voronoi" << std::endl;
				return -1;
			}
			break;
		}
		case 's':
		{
			SlicingConfiguration slConfig;
			slConfig.x_slices = slicingNumber.getValue().x;
			slConfig.y_slices = slicingNumber.getValue().y;
			slConfig.z_slices = slicingNumber.getValue().z;
			slConfig.angle_variations = angleVariation.getValue();
			slConfig.offset_variations = offsetVariation.getValue();
			if (fTool->slicing(0, slConfig, false, &rng) != 0)
			{
				std::cerr << "Failed to fracture with Slicing" << std::endl;
				return -1;
			}
			break;
		}
		default:
			std::cerr << "Unknown mode" << std::endl;
			return -1;
	}
	voronoiSitesGenerator->release();
	mesh->release();

	Nv::Blast::BlastBondGenerator* bondGenerator = NvBlastExtAuthoringCreateBondGenerator(gCooking, &gPhysics->getPhysicsInsertionCallback());
	Nv::Blast::ConvexMeshBuilder* collisionBuilder = NvBlastExtAuthoringCreateConvexMeshBuilder(gCooking, &gPhysics->getPhysicsInsertionCallback());
	Nv::Blast::AuthoringResult* result = NvBlastExtAuthoringProcessFracture(*fTool, *bondGenerator, *collisionBuilder);
	NvBlastTkFrameworkCreate();

	collisionBuilder->release();
	bondGenerator->release();
	fTool->release();

	// Output the results
	// NOTE: Writing to FBX by default. 

	std::vector<char*> matNames;
	for (int32_t i = 0; i < fileReader->getMaterialCount(); ++i)
	{
		matNames.push_back(fileReader->getMaterialName(i));
	}
	result->materialNames = matNames.data();
	result->materialCount = static_cast<uint32_t>(matNames.size());
	

	if (!fbxCollision.isSet())
	{
		result->releaseCollisionHulls();
	}

	if (bOutputObjFile)
	{
		std::shared_ptr<IMeshFileWriter> fileWriter(NvBlastExtExporterCreateObjFileWriter(), [](IMeshFileWriter* p) {p->release(); });
		fileWriter->appendMesh(*result, assetName.c_str());
		if (!fileWriter->saveToFile(assetName.c_str(), outDir.c_str()))
		{
			std::cerr << "Can't write geometry to OBJ file." << std::endl;
			return -1;
		}
	}
	if (bOutputFbxFile)
	{
		std::shared_ptr<IMeshFileWriter> fileWriter(NvBlastExtExporterCreateFbxFileWriter(bOutputFBXAscii), [](IMeshFileWriter* p) {p->release(); });
		fileWriter->appendMesh(*result, assetName.c_str(), nonSkinnedFBX.isSet());
		if (!fileWriter->saveToFile(assetName.c_str(), outDir.c_str()))
		{
			std::cerr << "Can't write geometry to FBX file." << std::endl;
			return -1;
		}
	}
	
	auto saveBlastData = [&](BlastDataExporter& blExpr)
	{
		if (bOutputLL)
		{
			blExpr.saveBlastObject(outDir, assetName, result->asset, LlObjectTypeID::Asset);
		}
		if (bOutputTK || bOutputBPXA)
		{
			Nv::Blast::TkAssetDesc descriptor;
			descriptor.bondCount = result->bondCount;
			descriptor.bondDescs = result->bondDescs;
			descriptor.bondFlags = nullptr;
			descriptor.chunkCount = result->chunkCount;
			descriptor.chunkDescs = result->chunkDescs;
			Nv::Blast::ExtPxAsset* physicsAsset = Nv::Blast::ExtPxAsset::create(descriptor, result->physicsChunks, result->physicsSubchunks, *NvBlastTkFrameworkGet());
			if (bOutputTK)
			{
				blExpr.saveBlastObject(outDir, assetName, &physicsAsset->getTkAsset(), TkObjectTypeID::Asset);
			}
			if (bOutputBPXA)
			{
				blExpr.saveBlastObject(outDir, assetName, physicsAsset, ExtPxObjectTypeID::Asset);
			}
			physicsAsset->release();
		}
	};
	
	BlastDataExporter blExpr(NvBlastTkFrameworkGet(), gPhysics, gCooking);
	saveBlastData(blExpr);

	result->release();

	return 0;
}
