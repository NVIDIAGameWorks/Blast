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
// Copyright (c) 2008-2020 NVIDIA Corporation. All rights reserved.


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