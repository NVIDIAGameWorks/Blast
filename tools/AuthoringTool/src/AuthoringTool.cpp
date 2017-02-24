#include "PxPhysicsAPI.h"
#include "PxAllocatorCallback.h"
#include "PxErrorCallback.h"
#include "PsFileBuffer.h"
#include "NvBlast.h"
#include "NvBlastExtAuthoringCollisionBuilder.h"
#include "NvBlastExtSerializationLLInterface.h"
#include "NvBlastExtSerializationInterface.h"
#include "NvBlastExtAuthoringBondGenerator.h"
#include "NvBlastExtAuthoringFractureTool.h"
#include "NvBlastExtAuthoringMesh.h"
#include "SimpleRandomGenerator.h"
#include "FbxFileReader.h"
#include "ObjFileReader.h"
#include "FractureProcessor.h"
#include "FbxFileWriter.h"
#include "ObjFileWriter.h"
#include "BlastDataExporter.h"
#include <string>
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

int main(int argc, const char* const* argv)
{
	// setup cmd line
	TCLAP::CmdLine cmd("Blast SDK: Authoring Tool", ' ', "0.1");

	TCLAP::UnlabeledValueArg<std::string> infileArg("file", "File to load", true, "", "infile");
	cmd.add(infileArg);

	TCLAP::UnlabeledValueArg<std::string> outAssetName("outAssetName", "Output asset name", true, DEFAULT_ASSET_NAME, "output asset name");
	cmd.add(outAssetName);

	TCLAP::ValueArg<std::string> outDirArg("", "outputDir", "Output directory", false, ".", "by default directory of the input file");
	cmd.add(outDirArg);

	// The output modes
	//NOTE: Fun TCLAP quirk here - if you set the default to true and specify this switch on the command line, the value will be false!
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

	TCLAP::SwitchArg protoSer("", "proto", "Serialize Blast data with CapnProto", false);
	cmd.add(protoSer);

	TCLAP::SwitchArg blockSer("", "block", "Serialize Blast data as block of memory", false);
	cmd.add(blockSer);




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
		std::cout << "error: " << e.error() << " for arg " << e.argId() << std::endl;
		return -1;
	}

	// get cmd parse results
	std::string infile = infileArg.getValue();

	std::string outDir;
	

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
		std::cout << "Can't determine extension (and thus, loader) of input file. " << infile << std::endl;
		return -1;
	}

	bool bOutputBPXA = bpxaOutputArg.getValue();
	bool bOutputTK = tkOutputArg.getValue();
	bool bOutputLL = llOutputArg.getValue();

	bool bUE4CoordSystem = ue4OutputArg.getValue();
	bool bOutputFBXAscii = fbxAsciiArg.getValue();

	bool bOutputObjFile = objOutputArg.isSet();
	bool bOutputFbxFile = fbxOutputArg.isSet();

	bool bOutputProtobufSer = protoSer.isSet();
	bool bOutputBlockSer = blockSer.isSet();
	

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
	// Did we specify no serialization type?
	if (!bOutputBlockSer && !bOutputProtobufSer)
	{
		std::cout << "Didn't specify an serialization type on the command line, so defaulting to block serialization type" << std::endl;
		bOutputBlockSer = true;
	}

	std::shared_ptr<IMeshFileReader> fileReader;

	if (extension.compare("FBX")==0)
	{
		fileReader = std::make_shared<FbxFileReader>();
	}
	else if (extension.compare("OBJ")==0)
	{
		fileReader = std::make_shared<ObjFileReader>();
	}
	else
	{
		std::cout << "Unsupported file extension " << extension << std::endl;
		return -1;
	}

	fileReader->setConvertToUE4(bUE4CoordSystem);

	// Load the asset
	std::shared_ptr<Mesh> loadedMesh = fileReader->loadFromFile(infile);

	if (loadedMesh == nullptr)
	{
		std::cout << "Failed to load mesh " << infile << std::endl;
		return -1;
	}

	// Send it to the fracture processor
	FractureProcessor processor;
	FractureSettings settings;
	settings.mode = fracturingMode.getValue();
	settings.cellsCount = cellsCount.getValue();
	settings.clusterCount = clusterCount.getValue();
	settings.slicingX = slicingNumber.getValue().x;
	settings.slicingY = slicingNumber.getValue().y;
	settings.slicingZ = slicingNumber.getValue().z;
	settings.angleVariation = angleVariation.getValue();
	settings.offsetVariation = offsetVariation.getValue();
	settings.clusterRadius = clusterRad.getValue();

	std::shared_ptr<FractureResult> result = processor.fractureMesh(loadedMesh, settings);
	
	// Output the results
	// NOTE: Writing to FBX by default. 
	std::shared_ptr<IMeshFileWriter> fileWriter;
	

	auto assetLL = result->resultPhysicsAsset->getTkAsset().getAssetLL();


	if (bOutputObjFile)
	{
		if (bUE4CoordSystem)
		{
			std::cout << "OBJ output doesn't support UE4 coordinate conversion." << std::endl;
		}
		fileWriter = std::make_shared<ObjFileWriter>();
		bool writeResult = fileWriter->saveToFile(assetLL, result->resultGeometry, assetName, outDir);
		if (!writeResult)
		{
			std::cerr << "Can't write geometry to OBJ file." << std::endl;
			return -1;
		}
	}
	if (bOutputFbxFile)
	{
		fileWriter = std::make_shared<FbxFileWriter>();
		fileWriter->setConvertToUE4(bUE4CoordSystem);
		{
			auto fbxWriter = static_cast<FbxFileWriter *>(fileWriter.get());
			fbxWriter->bOutputFBXAscii = bOutputFBXAscii;
		}

		bool writeResult = fileWriter->saveToFile(assetLL, result->resultGeometry, assetName, outDir);
		if (!writeResult)
		{
			std::cerr << "Can't write geometry to FBX file." << std::endl;
			return -1;
		}
	}

	if (bOutputProtobufSer)
	{
		if (bOutputBPXA)
		{
			std::ostringstream outBlastFilePathStream;
			outBlastFilePathStream << outDir << "/" << assetName << ".pbpxa";
			std::string outBlastFilePath = outBlastFilePathStream.str();

			std::ofstream myFile(outBlastFilePath, std::ios::out | std::ios::binary);

			setPhysXSDK(processor.getPhysics());

			serializeExtPxAssetIntoStream(result->resultPhysicsAsset.get(), myFile);

			myFile.flush();

			std::cout << "Wrote ExtPxAsset to " << outBlastFilePath << std::endl;
		}

		if (bOutputTK)
		{
			std::ostringstream outBlastFilePathStream;
			outBlastFilePathStream << outDir << "/" << assetName << ".ptkasset";
			std::string outBlastFilePath = outBlastFilePathStream.str();

			std::ofstream myFile(outBlastFilePath, std::ios::out | std::ios::binary);

			serializeTkAssetIntoStream(&result->resultPhysicsAsset->getTkAsset(), myFile);

			myFile.flush();

			std::cout << "Wrote TkAsset to " << outBlastFilePath << std::endl;
		}

		if (bOutputLL)
		{
			std::ostringstream outBlastFilePathStream;
			outBlastFilePathStream << outDir << "/" << assetName << ".pllasset";
			std::string outBlastFilePath = outBlastFilePathStream.str();

			std::ofstream myFile(outBlastFilePath, std::ios::out | std::ios::binary);

			serializeAssetIntoStream(result->resultPhysicsAsset->getTkAsset().getAssetLL(), myFile);

			myFile.flush();

			std::cout << "Wrote NvBlastAsset to " << outBlastFilePath << std::endl;
		}
	}

	if (bOutputBlockSer)
	{
		BlastDataExporter blExpr(NvBlastTkFrameworkGet(), processor.getCooking(), NvBlastTkFrameworkGet()->getLogFn());
			if (bOutputLL)
			{
				std::ostringstream outBlastFilePathStream;
				outBlastFilePathStream << outDir << "/" << assetName << ".llasset";
				std::string outBlastFilePath = outBlastFilePathStream.str();
				blExpr.saveBlastLLAsset(outBlastFilePath, result->resultPhysicsAsset->getTkAsset().getAssetLL());
				std::cout << "Wrote NvBlastAsset to " << outBlastFilePath << std::endl;
			}
			if (bOutputTK)
			{
				std::ostringstream outBlastFilePathStream;
				outBlastFilePathStream << outDir << "/" << assetName << ".tkasset";
				std::string outBlastFilePath = outBlastFilePathStream.str();
				blExpr.saveBlastTkAsset(outBlastFilePath, &result->resultPhysicsAsset->getTkAsset());
				std::cout << "Wrote TkAsset to " << outBlastFilePath << std::endl;
			}
			if (bOutputBPXA)
			{
				std::ostringstream outBlastFilePathStream;
				outBlastFilePathStream << outDir << "/" << assetName << ".bpxa";
				std::string outBlastFilePath = outBlastFilePathStream.str();
				blExpr.saveBlastExtAsset(outBlastFilePath, result->resultPhysicsAsset.get());
				std::cout << "Wrote ExtPxAsset to " << outBlastFilePath << std::endl;
			}
	}

	return 0;
}
