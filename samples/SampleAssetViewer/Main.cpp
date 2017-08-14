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
// Copyright (c) 2008-2017 NVIDIA Corporation. All rights reserved.


#include "Sample.h"
#include "tclap/CmdLine.h"
#include <sstream>

#include <Windows.h>


#define DEFAULT_ASSET_LIST "assets.xml"

struct TCLAPvec3
{
	float x, y, z; 
	TCLAPvec3() :x(0), y(0), z(0){};
	TCLAPvec3& operator=(const std::string &inp)
	{
		std::istringstream stream(inp);
		if (!(stream >> x >> y >> z))
			throw TCLAP::ArgParseException(inp + " is not vec3");
		return *this;
	}
};

struct TCLAPvec4
{
	float x, y, z, w;
	TCLAPvec4() :x(0), y(0), z(0), w(0){};
	TCLAPvec4& operator=(const std::string &inp)
	{
		std::istringstream stream(inp);
		if (!(stream >> x >> y >> z >> w))
			throw TCLAP::ArgParseException(inp + " is not vec4");
		return *this;
	}
};

namespace TCLAP {
	template<>
	struct ArgTraits<TCLAPvec3> {
		typedef StringLike ValueCategory;
	};
	template<>
	struct ArgTraits<TCLAPvec4> {
		typedef StringLike ValueCategory;
	};
}

using namespace std;
using namespace physx;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	PX_UNUSED(hInstance);
	PX_UNUSED(hPrevInstance);
	PX_UNUSED(lpCmdLine);
	PX_UNUSED(nCmdShow);

	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	AllocConsole();
#endif


	LPWSTR* argv;
	int    argc;
	argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	TCLAP::CmdLine cmd("Blast Sample: Asset Viewer", ' ', "0.1");
	TCLAP::ValueArg<std::string> inpXmlArg("x", "xml", "Asset list to load", false, DEFAULT_ASSET_LIST, "inassetlist");
	cmd.add(inpXmlArg);
	TCLAP::ValueArg<std::string> alternatePathArg("t", "path", "Alternate path", false, "", "altPath");
	cmd.add(alternatePathArg);

	TCLAP::ValueArg<std::string> addAssetArg("n", "nAsset", "Additional asset to load", false, "", "additionalAsset");
	cmd.add(addAssetArg);

	TCLAP::ValueArg<TCLAPvec4> rotationArg("r", "rot", "Additional asset rotation", false, TCLAPvec4(), "rotation");
	cmd.add(rotationArg);
	TCLAP::ValueArg<TCLAPvec3> positionArg("p", "pos", "Additional asset position", false, TCLAPvec3(), "position");
	cmd.add(positionArg);


	PxVec3 transform;
	bool addedExternalAsset = false;

	std::vector<string> argsVect;
	if (argc > 1)
	{
		for (size_t i = 0; i < (size_t)argc; ++i)
		{
			argsVect.push_back(string());
			argsVect.back().resize(wcslen(argv[i]), 0);
			wcstombs(&argsVect.back()[0], argv[i], 255);
		}
		cmd.parse(argsVect);
	}
	LocalFree(argv);

	SampleConfig config;
	config.assetsFile = inpXmlArg.getValue();
	config.sampleName = L"Blast Sample: Asset Viewer";

	config.assetsFile = inpXmlArg.getValue();
	if (alternatePathArg.isSet())
	{
		config.additionalResourcesDir.push_back(alternatePathArg.getValue().c_str());
	}
	if (addAssetArg.isSet())
	{
		AssetList::ModelAsset asset;
		TCLAPvec3 p = positionArg.getValue();
		TCLAPvec4 r = rotationArg.getValue();
		asset.transform.p = PxVec3(p.x, p.y, p.z);
		asset.transform.q = PxQuat(r.w, PxVec3(r.x, r.y, r.z).getNormalized());
		asset.name = addAssetArg.getValue();
		asset.id = asset.name;
		asset.file = asset.name;
		config.additionalAssetList.models.push_back(asset);
	}

	int result = runSample(config);

	return result;
}
