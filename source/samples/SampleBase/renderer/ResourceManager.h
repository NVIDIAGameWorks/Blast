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
// Copyright (c) 2008-2022 NVIDIA Corporation. All rights reserved.


#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <vector>
#include <string>
#include <map>
#include <memory>
#include "DirectXTex.h"


struct Resource
{
private:
    Resource& operator = (const Resource&);
};


struct ShaderFileResource : public Resource
{
    ShaderFileResource(const std::string& p) : path(p) {}
    std::string path;
};


struct TextureResource : public Resource
{
    DirectX::TexMetadata metaData;
    DirectX::ScratchImage image;
};


/**
ResourceManager used to look for files in provided dirs (see addSearchDir). Also it loads resources and caches them.
*/
class ResourceManager
{
public:
    //////// ctor ////////

    ResourceManager();
    ~ResourceManager();

    //////// public API ////////

    bool addSearchDir(const char* dir, bool recursive = true);

    const ShaderFileResource* requestShaderFile(const char* name);

    const TextureResource* requestTexture(const char* name);

    bool findFile(std::string fileName, std::string& foundPath);

    bool findFile(std::string fileName, const std::vector<const char*>& exts, char* foundPath);


private:
    //////// internal methods ////////

    enum ResourceType
    {
        eSHADER_FILE,
        eTEXTURE
    };

    const Resource* requestResource(ResourceType type, const char* name);

    bool findFileInDir(std::string fileNameFull, const char* path, bool recursive, char* foundPath);

    struct SearchDir
    {
        SearchDir(std::string path_, bool recursive_) : path(path_), recursive(recursive_) {}

        std::string path;
        bool recursive;
    };


    //////// internal data ////////

    std::vector<SearchDir> m_searchDirs;
    std::map<std::pair<ResourceType, std::string>, std::shared_ptr<Resource>> m_loadedResources;
};
#endif