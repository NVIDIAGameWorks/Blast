/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

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