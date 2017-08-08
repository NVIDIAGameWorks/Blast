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


#include "ResourceManager.h"
#include "PxAssert.h"
#include "PsString.h"
#include "Utils.h"

#include <windows.h>


using namespace physx;

#define PATH_MAX_LEN 512

// Add By Lixu Begin
ResourceManager* pResourceManager = nullptr;
ResourceManager* ResourceManager::ins()
{
	return pResourceManager;
}
// Add By Lixu End

ResourceManager::ResourceManager()
{
	// search for root folder by default
	addSearchDir(".");

// Add By Lixu Begin
	pResourceManager = this;
// Add By Lixu End
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

void ResourceManager::releaseTexture(const char* name)
{
	std::pair<ResourceType, std::string> key(eTEXTURE, name);
	auto val = m_loadedResources.find(key);
	if (val != m_loadedResources.end())
	{
		Resource* pResource = val->second;
		delete pResource;
		pResource = nullptr;
		m_loadedResources.erase(key);
	}
}

const Resource* ResourceManager::requestResource(ResourceType type, const char* name)
{
	// search in loaded
	std::pair<ResourceType, std::string> key(type, name);
	auto val = m_loadedResources.find(key);
	if (val != m_loadedResources.end())
	{
		return val->second;
	}

	Resource* resource = nullptr;
	if (type == eSHADER_FILE)
	{
		char path[PATH_MAX_LEN];
		const char* exts[] = { "hlsl" };
		if (findFile(name, std::vector<const char*>(exts, exts + sizeof(exts) / sizeof(exts[0])), path))
		{
			resource = new ShaderFileResource(path);
		}
		else
		{
			PX_ALWAYS_ASSERT_MESSAGE(name);
		}
	}
	else if (type == eTEXTURE)
	{
		char path[PATH_MAX_LEN];
// Add By Lixu Begin
		const char* exts[] = { "dds", "tga", "jpg", "png", "bmp" };
// Add By Lixu End
		if (findFile(name, std::vector<const char*>(exts, exts + sizeof(exts) / sizeof(exts[0])), path))
		{
			TextureResource* textureResource(new TextureResource());
			WCHAR wPath[MAX_PATH];
			MultiByteToWideChar(CP_ACP, 0, path, -1, wPath, MAX_PATH);
			wPath[MAX_PATH - 1] = 0;

			const char* ext = strext(path);
			if (::stricmp(ext, "dds") == 0)
			{
				V(DirectX::LoadFromDDSFile(wPath, DirectX::DDS_FLAGS_NONE, &textureResource->metaData,
					textureResource->image));
			}
			else if (::stricmp(ext, "tga") == 0)
			{
				V(DirectX::LoadFromTGAFile(wPath, &textureResource->metaData,
					textureResource->image));
			}
// Add By Lixu Begin
			else if (::stricmp(ext, "jpg") == 0)
			{
				V(DirectX::LoadFromWICFile(wPath, DirectX::TEX_FILTER_DEFAULT | DirectX::WIC_FLAGS_ALL_FRAMES, &textureResource->metaData,
					textureResource->image));
			}
			else if (::stricmp(ext, "png") == 0)
			{
				V(DirectX::LoadFromWICFile(wPath, DirectX::TEX_FILTER_DEFAULT | DirectX::WIC_FLAGS_ALL_FRAMES, &textureResource->metaData,
					textureResource->image));
			}
			else if (::stricmp(ext, "bmp") == 0)
			{
				V(DirectX::LoadFromWICFile(wPath, DirectX::TEX_FILTER_DEFAULT | DirectX::WIC_FLAGS_ALL_FRAMES, &textureResource->metaData,
					textureResource->image));
			}
// Add By Lixu End
			else
			{
				PX_ALWAYS_ASSERT_MESSAGE("Unsupported texture extension");
			}
			resource = textureResource;
		}
	}

	if (resource)
	{
		m_loadedResources.emplace(key, resource);
		return resource;
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
	
	std::string::size_type pos = 0;
	pos = fileNameOnly.find("\\", pos);
	while ((pos != std::string::npos))
	{
		fileNameOnly.replace(pos, 1, "/");
		pos = fileNameOnly.find("\\", (pos + 1));
	}

	size_t ind = fileNameOnly.find_last_of('/');
	if (ind > 0 && (ind != std::string::npos))
		fileNameOnly = fileNameOnly.substr(ind + 1);

// Add By Lixu Begin
	std::string fileDir = ".";
	size_t fl = fileName.length();
	if (ind >= 0 && ind < fl)
		fileDir = fileName.substr(0, ind);
	if (findFileInDir(fileNameOnly.c_str(), fileDir.c_str(), true, foundPath))
		return true;
// Add By Lixu End

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

