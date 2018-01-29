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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.


#include "NvBlastExtSerialization.h"
#include "NvBlastExtLlSerialization.h"
#include "NvBlastExtTkSerialization.h"
#include "NvBlastExtPxSerialization.h"
#include "NvBlastTk.h"
#include <string>
#include <iostream>
#include "tclap/CmdLine.h"
#include <stdint.h>
#include <iterator>
#include <fstream>
#include <functional>

#include "PxIO.h"
#include "PxPhysics.h"
#include "PxFileBuf.h"
#include "cooking/PxCooking.h"

#include "NvBlastExtSerializationInternal.h"
#include "NvBlastPxCallbacks.h"
#include "PxFoundation.h"
#include "PxFoundationVersion.h"
#include "PxPhysicsVersion.h"


using namespace Nv::Blast;
using namespace physx;
using namespace physx::general_PxIOStream2;


static ExtSerializationInternal*	sSer = nullptr;
static TkFramework*					sFwk = nullptr;
static PxFoundation*				sFnd = nullptr;
static PxPhysics*					sPhys = nullptr;
static PxCooking*					sCook = nullptr;


static void str_to_upper(std::string& str)
{
	for (auto & c : str) c = (char)::toupper(c);
}


static void decompose_filename(std::string& name, std::string& ext, const std::string& filename)
{
	const size_t pos = filename.find_last_of('.');
	if (pos != std::string::npos)
	{
		name = filename.substr(0, pos);
		ext = filename.substr(pos + 1);
	}
	else
	{
		name = filename;
		ext = "";
	}
}


static uint32_t encodingID_from_str(const std::string& str)
{
	const char* s = str.c_str();
	return str.length() >= 4 ? NVBLAST_FOURCC(s[0], s[1], s[2], s[3]) : 0;
}


static int init_framework()
{
	// create serialization manager and load all serializers
	sSer = static_cast<ExtSerializationInternal*>(NvBlastExtSerializationCreate());
	if (sSer == nullptr)
	{
		std::cerr << "FAIL: Could not create serialization manager" << std::endl;
		return -1;
	}

	sFwk = NvBlastTkFrameworkCreate();
	if (sFwk != nullptr)
	{
		NvBlastExtTkSerializerLoadSet(*sFwk, *sSer);
	}

	sFnd = PxCreateFoundation(PX_FOUNDATION_VERSION, NvBlastGetPxAllocatorCallback(), NvBlastGetPxErrorCallback());
	if (sFnd != nullptr)
	{
		PxTolerancesScale tol;
		PxCookingParams cookingParams(tol);
		cookingParams.buildGPUData = true;
		sPhys = PxCreatePhysics(PX_PHYSICS_VERSION, *sFnd, tol, true);
		if (sPhys != nullptr)
		{
			sCook = PxCreateCooking(PX_PHYSICS_VERSION, sPhys->getFoundation(), cookingParams);
			if (sPhys != nullptr && sCook != nullptr)
			{
				NvBlastExtPxSerializerLoadSet(*sFwk, *sPhys, *sCook, *sSer);
				return 0;
			}
		}
	}

	return -1;
}


static void term_framework()
{
	// release serialization
	if (sSer != nullptr)
	{
		sSer->release();
	}

	// release framework, physics, foundation, and cooking
	if (sCook != nullptr)
	{
		sCook->release();
	}
	if (sPhys != nullptr)
	{
		sPhys->release();
	}
	if (sFnd != nullptr)
	{
		sFnd->release();
	}
	if (sFwk != nullptr)
	{
		sFwk->release();
	}
}


static bool parse_inputs(uint32_t& objectTypeID, uint32_t& inEncodingID, std::string& outfile, uint32_t& outEncodingID, TCLAP::CmdLine& cmd, std::vector<std::string>& args, const std::string& infile)
{
	TCLAP::ValueArg<std::string> typeArg("t", "type", "Input file type: llasset, tkasset, bpxa, pllasset, ptkasset, or pbpxa.  The default will be based upon the input filename's extension.", false, "", "type");
	cmd.add(typeArg);
	TCLAP::ValueArg<std::string> outfileArg("o", "outfile", "Output filename.  The extension will be .blast if none is given in the filename.  If no outfile is given, infile with .blast extension will be used.", false, "", "outfile");
	cmd.add(outfileArg);
	TCLAP::ValueArg<std::string> encodingArg("e", "encoding", "Output encoding: cpnb or raw (default is cpnb).", false, "", "encoding");
	cmd.add(encodingArg);

	cmd.parse(args);

	std::string name;
	std::string ext;
	decompose_filename(name, ext, infile);

	std::string type = typeArg.isSet() ? typeArg.getValue() : ext;
	str_to_upper(type);
	const char* types[] = { "LLASSET", "PLLASSET", "TKASSET", "PTKASSET", "BPXA", "PBPXA" };
	size_t typeIndex = 0;
	for (; typeIndex < sizeof(types) / sizeof(types[0]) && type != types[typeIndex]; ++typeIndex) {}

	inEncodingID = ExtSerialization::EncodingID::CapnProtoBinary;
	switch (typeIndex)
	{
	case 0:
		inEncodingID = ExtSerialization::EncodingID::RawBinary;
	case 1:
		objectTypeID = LlObjectTypeID::Asset;
		break;
	case 2:
		inEncodingID = ExtSerialization::EncodingID::RawBinary;
	case 3:
		objectTypeID = TkObjectTypeID::Asset;
		break;
	case 4:
		inEncodingID = ExtSerialization::EncodingID::RawBinary;
	case 5:
		objectTypeID = ExtPxObjectTypeID::Asset;
		break;
	default:
		return false;
	}

	if (outfileArg.isSet())
	{
		outfile = outfileArg.getValue();
		if (std::string::npos == outfile.find_first_of('.'))
		{
			outfile += ".blast";
		}
	}
	else
	{
		outfile = name + ".blast";
	}

	std::string encoding = encodingArg.isSet() ? encodingArg.getValue() : "CPNB";
	str_to_upper(encoding);
	if (encoding.length() < 4)
	{
		encoding.insert(encoding.end(), 4 - encoding.length(), ' ');
	}
	outEncodingID = encodingID_from_str(encoding);

	return true;
}


int customMain(std::vector<std::string>& args)
{
	try 
	{
		// setup cmd line
		TCLAP::CmdLine cmd("Blast Legacy File Converter", ' ', "1.0");

		// parse cmd input
		std::string infile;
		if (args.size() > 1)
		{
			infile = args[1];
			args.erase(args.begin() + 1);
		}
		uint32_t objectTypeID;
		uint32_t inEncodingID;
		std::string outfile;
		uint32_t outEncodingID;
		if (!parse_inputs(objectTypeID, inEncodingID, outfile, outEncodingID, cmd, args, infile))
		{
			std::cerr << "FAIL: Could not parse inputs" << std::endl;
			return -1;
		}

		if (init_framework())
		{
			return -1;
		}

		sSer->setSerializationEncoding(outEncodingID);

		// find serializer
		ExtSerializer* serializer = sSer->findSerializer(objectTypeID, inEncodingID);
		if (serializer == nullptr)
		{
			std::cerr << "FAIL: No serializer for the file object type and encoding found" << std::endl;
			return -1;
		}

		// read input file
		std::ifstream infileStream(infile.c_str(), std::ios::binary);
		if (!infileStream.is_open())
		{
			std::cerr << "FAIL: Can't open input file: " << infile << std::endl;
			return -1;
		}
		const std::vector<char> inBuffer((std::istreambuf_iterator<char>(infileStream)), std::istreambuf_iterator<char>());
		infileStream.close();

		// deserialize
		void* object = serializer->deserializeFromBuffer(inBuffer.data(), inBuffer.size());
		if (object == nullptr)
		{
			std::cerr << "FAIL: serializer could not deserialize data from input file: " << infile << std::endl;
			return -1;
		}

		// serialize using manager, so that the appropriate header information is written
		void* outBuffer;
		const uint64_t bytesWritten = sSer->serializeIntoBuffer(outBuffer, object, objectTypeID);
		if (bytesWritten == 0)
		{
			std::cerr << "FAIL: could not serialize object in requested output format" << std::endl;
			return -1;
		}

		// release object
		NVBLAST_FREE(object);

		// write file
		std::ofstream outfileStream(outfile.c_str(), std::ios::binary);
		if (!outfileStream.is_open())
		{
			std::cerr << "FAIL: Can't open output file: " << outfile << std::endl;
			return -1;
		}
		outfileStream.write(reinterpret_cast<char*>(outBuffer), bytesWritten);
		outfileStream.close();

		// release buffer
		NVBLAST_FREE(outBuffer);

		// release framework
		term_framework();
	}
	catch (TCLAP::ArgException &e)  // catch any exceptions
	{
		std::cout << "error: " << e.error() << " for arg " << e.argId() << std::endl;
	}

	return 0;
}


int main(int argc, const char* const* argv)
{
	std::vector<std::string> args;
	for (int i = 0; i < argc; i++)
		args.push_back(argv[i]);
	return customMain(args);
}
