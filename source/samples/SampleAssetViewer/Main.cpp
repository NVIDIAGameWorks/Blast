// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2008-2024 NVIDIA Corporation. All rights reserved.


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
