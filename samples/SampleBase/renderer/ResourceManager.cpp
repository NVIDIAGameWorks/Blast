/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "ResourceManager.h"
#include "PxAssert.h"
#include "PsString.h"
#include "Utils.h"

#include <windows.h>


using namespace physx;

#define PATH_MAX_LEN 512


ResourceManager::ResourceManager()
{
	// search for root folder by default
	addSearchDir(".");
}

const ShaderFileResource* ResourceManager::requestShaderFile(const char* name)
{
	const Resource* resource = requestResource(eSHADER_FILE, name);
	return resource != nullptr ? static_cast<const ShaderFileResource*>(resource) : nullptr;
}

const TextureResource* ResourceManager::requestTexture(const char* name)
{
	const Resource* resource = requestResource(eTEXTURE, name);
	return resource != nullptr ? static_cast<const TextureResource*>(resource) : nullptr;
}

const Resource* ResourceManager::requestResource(ResourceType type, const char* name)
{
	// search in loaded
	std::pair<ResourceType, std::string> key(type, name);
	auto val = m_loadedResources.find(key);
	if (val != m_loadedResources.end())
	{
		return val->second.get();
	}

	std::shared_ptr<Resource> resource;
	if (type == eSHADER_FILE)
	{
		char path[PATH_MAX_LEN];
		const char* exts[] = { "hlsl" };
		if (findFile(name, std::vector<const char*>(exts, exts + sizeof(exts) / sizeof(exts[0])), path))
		{
			resource = std::shared_ptr<Resource>(new ShaderFileResource(path));
		}
		else
		{
			PX_ALWAYS_ASSERT_MESSAGE(name);
		}
	}
	else if (type == eTEXTURE)
	{
		char path[PATH_MAX_LEN];
		const char* exts[] = { "dds", "tga" };
		if (findFile(name, std::vector<const char*>(exts, exts + sizeof(exts) / sizeof(exts[0])), path))
		{
			std::shared_ptr<TextureResource> textureResource(new TextureResource());
			WCHAR wPath[MAX_PATH];
			MultiByteToWideChar(CP_ACP, 0, path, -1, wPath, MAX_PATH);
			wPath[MAX_PATH - 1] = 0;

			const char* ext = strext(path);
			if (::strcmp(ext, "dds") == 0)
			{
				V(DirectX::LoadFromDDSFile(wPath, DirectX::DDS_FLAGS_NONE, &textureResource->metaData,
					textureResource->image));
			}
			else if (::strcmp(ext, "tga") == 0)
			{
				V(DirectX::LoadFromTGAFile(wPath, &textureResource->metaData,
					textureResource->image));
			}
			else
			{
				PX_ALWAYS_ASSERT_MESSAGE("Unsupported texture extension");
			}
			resource = textureResource;
		}
	}

	if (resource.get())
	{
		m_loadedResources.emplace(key, resource);
		return resource.get();
	}
	else
	{
		PX_ALWAYS_ASSERT_MESSAGE(name);
		return nullptr;
	}
}

bool dirExists(const char* dir)
{
	DWORD ftyp = GetFileAttributesA(dir);
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  // something is wrong with path!

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;   // this is a directory!

	return false;    // this is not a directory!
}

bool ResourceManager::addSearchDir(const char* dir, bool recursive)
{
	if (dirExists(dir))
	{
		m_searchDirs.push_back(SearchDir(dir, recursive));
		return true;
	}
	return false;
}


ResourceManager::~ResourceManager()
{
}


bool ResourceManager::findFileInDir(std::string fileNameFull, const char* path, bool recursive, char* foundPath)
{
	WIN32_FIND_DATAA ffd;
	char tmp[PATH_MAX_LEN];
	shdfnd::snprintf(tmp, sizeof(tmp), "%s\\*", path);
	HANDLE hFind = FindFirstFileA(tmp, &ffd);

	if(INVALID_HANDLE_VALUE == hFind)
	{
		return NULL;
	}

	do
	{
		if (0 == shdfnd::strcmp(".", ffd.cFileName) || 0 == shdfnd::strcmp("..", ffd.cFileName))
			continue;

		if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			shdfnd::snprintf(tmp, sizeof(tmp), "%s\\%s", path, ffd.cFileName);
			if(findFileInDir(fileNameFull, tmp, recursive, foundPath))
				return true;
		}
		else if (shdfnd::stricmp(ffd.cFileName, fileNameFull.c_str()) == 0)
		{
			shdfnd::snprintf(foundPath, PATH_MAX_LEN, "%s\\%s", path, ffd.cFileName);
			return true;
		}
	} while(FindNextFileA(hFind, &ffd) != 0);
	// release handle
	FindClose(hFind);
	return false;
}

bool ResourceManager::findFile(std::string fileName, const std::vector<const char*>& exts, char* foundPath)
{
	std::string fileNameOnly = fileName;
	size_t ind = fileNameOnly.find_last_of('/');
	if (ind > 0)
		fileNameOnly = fileNameOnly.substr(ind + 1);

	for(size_t i = 0; i < m_searchDirs.size(); i++)
	{
		const SearchDir& searchDir = m_searchDirs[i];

		for(size_t j = 0; j < exts.size(); j++)
		{
			const char* ext = exts[j];
			const uint32_t fileMaxLen = 128;
			char fileNameFull[fileMaxLen] = { 0 };

			physx::shdfnd::snprintf(fileNameFull, fileMaxLen, "%s.%s", fileNameOnly.c_str(), ext);
			if(findFileInDir(fileNameFull, searchDir.path.c_str(), searchDir.recursive, foundPath))
				return true;
		}

		if (findFileInDir(fileNameOnly.c_str(), searchDir.path.c_str(), searchDir.recursive, foundPath))
			return true;
	}
	return false;
}

bool ResourceManager::findFile(std::string fileName, std::string& foundPath)
{
	std::vector<const char*> exts;
	char path[PATH_MAX_LEN];
	if (findFile(fileName, exts, path))
	{
		foundPath = path;
		return true;
	}
	else
	{
		return false;
	}
}

