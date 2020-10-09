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
#include "NvBlastExtPxManager.h"
#include "NvBlastExtPxCollisionBuilder.h"
#include "NvBlastExtAuthoring.h"
#include "NvBlastExtAuthoringMesh.h"
#include "NvBlastExtAuthoringBondGenerator.h"
#include "NvBlastExtAuthoringCutout.h"
#include "NvBlastExtAuthoringFractureTool.h"
#include "BlastDataExporter.h"
#include "SimpleRandomGenerator.h"
#include "NvBlastExtAuthoringMeshCleaner.h"
#include "NvBlastExtExporterJsonCollision.h"

#include <string>
#include <memory>
#include <iostream>
#include <vector>
#include <cctype>
#include <fstream>
#include <iosfwd>
#include <Windows.h>
#include "tclap/CmdLine.h"


//enable/disable memory leak detection
//#define _CRTDBG_MAP_ALLOC 
#ifdef _CRTDBG_MAP_ALLOC
#include <stdlib.h> 
#include <crtdbg.h> 
#endif

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

struct TCLAPfloat3
{
	float x, y, z;
	TCLAPfloat3(float x, float y, float z) :x(x), y(y), z(z) {};
	TCLAPfloat3() :x(0), y(0), z(0) {};
	TCLAPfloat3& operator=(const std::string &inp)
	{
		std::istringstream stream(inp);
		if (!(stream >> x >> y >> z))
			throw TCLAP::ArgParseException(inp + " is not float3");
		return *this;
	}

	operator NvcVec3()
	{
		return {x, y, z};
	}
	operator physx::PxVec3()
	{
		return { x, y, z };
	}
};

namespace TCLAP {
	template<>
	struct ArgTraits<TCLAPint3> {
		typedef StringLike ValueCategory;
	};

	template<>
	struct ArgTraits<TCLAPfloat3> {
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

unsigned char *LoadBitmapFile(const char *filename, BITMAPINFOHEADER *bitmapInfoHeader)
{
	FILE *filePtr; //our file pointer
	BITMAPFILEHEADER bitmapFileHeader; //our bitmap file header
	unsigned char *bitmapImage;  //store image data
	//int imageIdx = 0;  //image index counter
	unsigned char tempRGB;  //our swap variable

							//open filename in read binary mode
	filePtr = fopen(filename, "rb");
	if (filePtr == NULL)
		return NULL;

	//read the bitmap file header
	fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, filePtr);

	//verify that this is a bmp file by check bitmap id
	if (bitmapFileHeader.bfType != 0x4D42)
	{
		fclose(filePtr);
		return NULL;
	}

	//read the bitmap info header
	fread(bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, filePtr); // small edit. forgot to add the closing bracket at sizeof

																   //move file point to the begging of bitmap data
	fseek(filePtr, bitmapFileHeader.bfOffBits, SEEK_SET);

	//Only incompressed 24 byte RGB is supported
	if (bitmapInfoHeader->biCompression != BI_RGB || bitmapInfoHeader->biBitCount != 24)
	{
		return nullptr;
	}
	else
	{
		bitmapInfoHeader->biSizeImage = 3 * bitmapInfoHeader->biHeight * bitmapInfoHeader->biHeight;
	}

	//allocate enough memory for the bitmap image data
	bitmapImage = (unsigned char*)malloc(bitmapInfoHeader->biSizeImage);

	//verify memory allocation
	if (!bitmapImage)
	{
		free(bitmapImage);
		fclose(filePtr);
		return NULL;
	}

	//read in the bitmap image data
	fread(bitmapImage, sizeof(uint8_t), bitmapInfoHeader->biSizeImage, filePtr);

	//make sure bitmap image data was read
	if (bitmapImage == NULL)
	{
		fclose(filePtr);
		return NULL;
	}

	//swap the r and b values to get RGB (bitmap is BGR)
	if (bitmapInfoHeader->biBitCount > 1)
	{
		for (uint32_t imageIdx = 0; imageIdx < bitmapInfoHeader->biSizeImage; imageIdx += 3) // fixed semicolon
		{
			tempRGB = bitmapImage[imageIdx];
			bitmapImage[imageIdx] = bitmapImage[imageIdx + 2];
			bitmapImage[imageIdx + 2] = tempRGB;
		}
	}

	//close file and return bitmap iamge data
	fclose(filePtr);
	return bitmapImage;
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
	TCLAP::CmdLine cmd("Blast SDK: Authoring Tool", ' ', "1.1");

	TCLAP::UnlabeledValueArg<std::string> infileArg("file", "File to load", true, "", "infile");
	cmd.add(infileArg);

	TCLAP::UnlabeledValueArg<std::string> outAssetName("outAssetName", "Output asset name", true, DEFAULT_ASSET_NAME, "output asset name");
	cmd.add(outAssetName);

	TCLAP::ValueArg<std::string> outDirArg("", "outputDir", "Output directory", false, ".", "by default directory of the input file");
	cmd.add(outDirArg);

	TCLAP::SwitchArg cleanArg("", "clean", "Try cleaning mesh before fracturing", false);
	cmd.add(cleanArg);

	// The output modes
	//NOTE: Fun TCLAP quirk here - if you set the default to true and specify this switch on the command line, the value will be false!
	TCLAP::SwitchArg pxOutputArg("", "px", "Output ExtPxAsset to the .blast file in the output directory.", false);
	cmd.add(pxOutputArg);

	TCLAP::SwitchArg tkOutputArg("", "tk", "Output TkAsset to the .blast file in the output directory.", false);
	cmd.add(tkOutputArg);

	TCLAP::SwitchArg llOutputArg("", "ll", "Output LL Blast (NvBlastAsset) to the .blast file in the output directory.", false);
	cmd.add(llOutputArg);

	TCLAP::SwitchArg fbxAsciiArg("", "fbxascii", "Output FBX as an ascii file (defaults to binary output)", false);
	cmd.add(fbxAsciiArg);

 	TCLAP::SwitchArg objOutputArg("", "obj", "Output a OBJ mesh to the output directory", false);
 	cmd.add(objOutputArg);

	TCLAP::SwitchArg fbxOutputArg("", "fbx", "Output a FBX mesh to the output directory", false);
	cmd.add(fbxOutputArg);

	TCLAP::SwitchArg fbxCollision("", "fbxcollision", "Add collision geometry to FBX file", false);
	cmd.add(fbxCollision);

	TCLAP::SwitchArg jsonCollision("", "jsoncollision", "Save collision geometry to a json file", false);
	cmd.add(jsonCollision);

	TCLAP::SwitchArg nonSkinnedFBX("", "nonskinned", "Output a non-skinned FBX file", false);
	cmd.add(nonSkinnedFBX);

	TCLAP::ValueArg<int32_t> interiorMatId("", "interiorMat", "Use to setup interior material id, by default new material for internal surface will be created.", false, -1, "by default -1");
	cmd.add(interiorMatId);

	TCLAP::ValueArg<unsigned char> fracturingMode("", "mode", "Fracturing mode", false, 'v',
		"v - voronoi, c - clustered voronoi, s - slicing, p - plane cut, u - cutout, i - chunks from islands.");
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

	TCLAP::ValueArg<TCLAPfloat3> point("", "point", "Plane surface point", false, TCLAPfloat3(0, 0, 0), "by default 0 0 0");
	cmd.add(point);

	TCLAP::ValueArg<TCLAPfloat3> normal("", "normal", "Plane surface normal", false, TCLAPfloat3(1, 0, 0), "by default 1 0 0");
    cmd.add(normal);

	TCLAP::ValueArg<std::string> cutoutBitmapPath("", "cutoutBitmap", "Path to *.bmp file with cutout bitmap", false, ".", "by defualt empty");
	cmd.add(cutoutBitmapPath);

	TCLAP::ValueArg<uint32_t> aggregateMaxCount("", "agg", "Maximum number of collision hulls per chunk (aggregate)", false, 1, "by default 1");
	cmd.add(aggregateMaxCount);

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
		std::cerr << "[Error] Can't find input file: " << infile << std::endl;
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
	std::cout << "Output directory: " << outDir << std::endl;
	std::string assetName = outAssetName.getValue();

	// Determine whether to use the obj or fbx loader

	auto idx = infile.rfind('.');
	std::string extension;
	

	if (idx != std::string::npos)
	{
		extension = infile.substr(idx + 1);
		std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) -> unsigned char { return (unsigned char)std::toupper(c); });
	}
	else
	{
		std::cerr << "Can't determine extension (and thus, loader) of input file. " << infile << std::endl;
		return -1;
	}

	bool bOutputPX = pxOutputArg.getValue();
	bool bOutputTK = tkOutputArg.getValue();
	bool bOutputLL = llOutputArg.getValue();

	bool bOutputFBXAscii = fbxAsciiArg.getValue();

	bool bOutputObjFile = objOutputArg.isSet();
	bool bOutputFbxFile = fbxOutputArg.isSet();

	// Did we specify no output formats?
	if (!bOutputPX && !bOutputTK && !bOutputLL)
	{
		std::cout << "Didn't specify an output format on the command line. Use default: LL Blast asset (NvBlastAsset)." << std::endl;
		bOutputLL = true;
	}
	else if	((int)bOutputPX + (int)bOutputTK + (int)bOutputLL > 1)
	{
		std::cerr << "More than one of the --ll, --tk, and --px options are set. Choose one. " << std::endl;
		return -1;
	}

	// Did we specify no geometry output formats?
	if (!bOutputObjFile && !bOutputFbxFile)
	{
		std::cout << "Didn't specify an output geometry format on the command line. Use default: .FBX" << std::endl;
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

#ifdef _CRTDBG_MAP_ALLOC
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);

	_CrtMemState _ms;
	_CrtMemCheckpoint(&_ms);
#endif
	
	if (!initPhysX())
	{
		std::cerr << "Failed to initialize PhysX" << std::endl;
		return -1;
	}
	Nv::Blast::FractureTool* fTool = NvBlastExtAuthoringCreateFractureTool();

	NvcVec3* pos = fileReader->getPositionArray();
	NvcVec3* norm = fileReader->getNormalsArray();
	NvcVec2* uv = fileReader->getUvArray();

	Nv::Blast::Mesh* mesh = NvBlastExtAuthoringCreateMesh(pos, norm, uv, vcount, fileReader->getIndexArray(), fileReader->getIndicesCount());

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
	rng.seed(0);
	Nv::Blast::VoronoiSitesGenerator* voronoiSitesGenerator = NvBlastExtAuthoringCreateVoronoiSitesGenerator(mesh, &rng);
	if (voronoiSitesGenerator == nullptr)
	{
		std::cerr << "Failed to create Voronoi sites generator" << std::endl;
		return -1;
	}

	// Send it to the fracture processor

	switch (fracturingMode.getValue())
	{
		case 'i':
		{
			std::cout << "Generate chunks from islands..." << std::endl;
			fTool->islandDetectionAndRemoving(0, true);
			break;
		}
		case 'v':
		{
			std::cout << "Fracturing with Voronoi..." << std::endl;
			voronoiSitesGenerator->uniformlyGenerateSitesInMesh(cellsCount.getValue());
			const NvcVec3* sites = nullptr;
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
			std::cout << "Fracturing with Clustered Voronoi..." << std::endl;
			voronoiSitesGenerator->clusteredSitesGeneration(cellsCount.getValue(), clusterCount.getValue(), clusterRad.getValue());
			const NvcVec3* sites = nullptr;
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
			std::cout << "Fracturing with Slicing..." << std::endl;
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
		case 'p':
		{
			std::cout << "Plane cut fracturing..." << std::endl;
			NoiseConfiguration noise;
			if (fTool->cut(0, normal.getValue(), point.getValue(), noise, false, &rng) != 0)
			{
				std::cerr << "Failed to fracture with Cutout (in half-space, plane cut)" << std::endl;
				return -1;
			}
			break;
		}
		case 'u':
		{
			std::cout << "Cutout fracturing..." << std::endl;
			CutoutConfiguration cutoutConfig;
			physx::PxVec3 axis = normal.getValue();
			if (axis.isZero())
			{
				axis = PxVec3(0.f, 0.f, 1.f);
			}
			axis.normalize();
			float d = axis.dot(physx::PxVec3(0.f, 0.f, 1.f));
		    physx::PxQuat q;
			if (d < (1e-6f - 1.0f))
			{
				q = physx::PxQuat(physx::PxPi, PxVec3(1.f, 0.f, 0.f));
			}
			else if (d < 1.f)
			{
				float s = physx::PxSqrt((1 + d) * 2);
				float invs = 1 / s;
				auto c = axis.cross(PxVec3(0.f, 0.f, 1.f));
				q = {c.x * invs, c.y * invs, c.z * invs, s * 0.5f};
				q.normalize();
			}
		    cutoutConfig.transform.q = reinterpret_cast<NvcQuat&>(q);
			cutoutConfig.transform.p = point.getValue();
			if (cutoutBitmapPath.isSet())
			{
				BITMAPINFOHEADER header;
				uint8_t* bitmap = LoadBitmapFile(cutoutBitmapPath.getValue().c_str(), &header);
				if (bitmap != nullptr)
				{
					cutoutConfig.cutoutSet = NvBlastExtAuthoringCreateCutoutSet();
					NvBlastExtAuthoringBuildCutoutSet(*cutoutConfig.cutoutSet, bitmap, header.biWidth, header.biHeight, 0.001f, 1.f, false, true);
				}
			}
			if (fTool->cutout(0, cutoutConfig, false, &rng) != 0)
			{
				std::cerr << "Failed to fracture with Cutout" << std::endl;
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
	
	Nv::Blast::ExtPxCollisionBuilder* collisionBuilder = ExtPxManager::createCollisionBuilder(*gPhysics, *gCooking);
	Nv::Blast::BlastBondGenerator* bondGenerator   = NvBlastExtAuthoringCreateBondGenerator(collisionBuilder);
	
	Nv::Blast::ConvexDecompositionParams collisionParameter;
	collisionParameter.maximumNumberOfHulls = aggregateMaxCount.getValue() > 0 ? aggregateMaxCount.getValue() : 1;
	collisionParameter.voxelGridResolution = 0;
	Nv::Blast::AuthoringResult* result = NvBlastExtAuthoringProcessFracture(*fTool, *bondGenerator, *collisionBuilder, collisionParameter);
	auto tk = NvBlastTkFrameworkCreate();

	bondGenerator->release();
	fTool->release();

	// Output the results
	// NOTE: Writing to FBX by default. 

	std::vector<const char*> matNames;
	for (int32_t i = 0; i < fileReader->getMaterialCount(); ++i)
	{
		matNames.push_back(fileReader->getMaterialName(i));
	}
	result->materialNames = matNames.data();
	result->materialCount = static_cast<uint32_t>(matNames.size());
	
	const std::string assetNameFull = outDir + "\\" + assetName;

	if (jsonCollision.isSet())
	{
		const std::string fullJsonFilename = assetNameFull + ".json";
		IJsonCollisionExporter* collisionExporter = NvBlastExtExporterCreateJsonCollisionExporter();
		if (collisionExporter != nullptr)
		{
			if (collisionExporter->writeCollision(fullJsonFilename.c_str(), result->chunkCount, result->collisionHullOffset, result->collisionHull))
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

	if (!fbxCollision.isSet())
	{
		NvBlastExtAuthoringReleaseAuthoringResultCollision(*collisionBuilder, result);
	}

	if (bOutputObjFile)
	{
		std::shared_ptr<IMeshFileWriter> fileWriter(NvBlastExtExporterCreateObjFileWriter(), [](IMeshFileWriter* p) {p->release(); });
		if (interiorMatId.isSet() && interiorMatId.getValue() >= 0)
		if (!fbxCollision.isSet())
		{
			fileWriter->setInteriorIndex(interiorMatId.getValue());
		}
		fileWriter->appendMesh(*result, assetName.c_str());
		if (!fileWriter->saveToFile(assetName.c_str(), outDir.c_str()))
		{
			std::cerr << "Can't write geometry to OBJ file." << std::endl;
			return -1;
		}
		std::cout << "Exported render mesh geometry: " << assetNameFull << ".obj" << std::endl;
	}
	if (bOutputFbxFile)
	{
		std::shared_ptr<IMeshFileWriter> fileWriter(NvBlastExtExporterCreateFbxFileWriter(bOutputFBXAscii), [](IMeshFileWriter* p) {p->release(); });
		if (interiorMatId.isSet() && interiorMatId.getValue() >= 0)
		{
			fileWriter->setInteriorIndex(interiorMatId.getValue());
		}
		fileWriter->appendMesh(*result, assetName.c_str(), nonSkinnedFBX.isSet());
		if (!fileWriter->saveToFile(assetName.c_str(), outDir.c_str()))
		{
			std::cerr << "Can't write geometry to FBX file." << std::endl;
			return -1;
		}
		if (fbxCollision.isSet())
		{
			std::cout << "Exported render mesh and collision geometry: " << assetNameFull << ".fbx" << std::endl;
		}
		else
		{
			std::cout << "Exported render mesh geometry: " << assetNameFull << ".fbx" << std::endl;
		}
	}
	
	auto saveBlastData = [&](BlastDataExporter& blExpr)
	{
		if (bOutputLL)
		{
			blExpr.saveBlastObject(outDir, assetName, result->asset, LlObjectTypeID::Asset);
			std::cout << "Exported NvBlastAsset: " << assetNameFull << ".blast" << std::endl;
		}
		else
		{
			Nv::Blast::TkAssetDesc descriptor;
			descriptor.bondCount = result->bondCount;
			descriptor.bondDescs = result->bondDescs;
			descriptor.bondFlags = nullptr;
			descriptor.chunkCount = result->chunkCount;
			descriptor.chunkDescs = result->chunkDescs;

			std::vector<ExtPxChunk> physicsChunks(result->chunkCount);
			std::vector<ExtPxSubchunk> physicsSubchunks(result->chunkCount);
			collisionBuilder->buildPhysicsChunks(result->chunkCount, result->collisionHullOffset, result->collisionHull, physicsChunks.data(), physicsSubchunks.data());
			Nv::Blast::ExtPxAsset* physicsAsset = Nv::Blast::ExtPxAsset::create(descriptor, physicsChunks.data(), physicsSubchunks.data(), *NvBlastTkFrameworkGet());
			if (bOutputTK)
			{
				blExpr.saveBlastObject(outDir, assetName, &physicsAsset->getTkAsset(), TkObjectTypeID::Asset);
				std::cout << "Exported TkAsset: " << assetNameFull << ".blast" << std::endl;
			}
			else if (bOutputPX)
			{
				blExpr.saveBlastObject(outDir, assetName, physicsAsset, ExtPxObjectTypeID::Asset);
				std::cout << "Exported ExtPxAsset: " << assetNameFull << ".blast" << std::endl;
			}
			physicsAsset->release();
		}
	};
	
	BlastDataExporter blExpr(NvBlastTkFrameworkGet(), gPhysics, gCooking);
	saveBlastData(blExpr);

	NvBlastExtAuthoringReleaseAuthoringResult(*collisionBuilder, result);
	collisionBuilder->release();

	if (tk)
	{
		tk->release();
	}
	if (gCooking)
	{
		gCooking->release();
	}
	if (gPhysics)
	{
		gPhysics->release();
	}
	if (gFoundation)
	{
		gFoundation->release();
	}	

	std::cout << "Success!" << std::endl;

	return 0;
}
