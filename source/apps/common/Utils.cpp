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
// Copyright (c) 2016-2024 NVIDIA Corporation. All rights reserved.


#include "Utils.h"

#include "Log.h"

#include <string.h>

#if PX_WINDOWS_FAMILY
#include <direct.h>
#define getCwd _getcwd
#else
#include <unistd.h>
#define getCwd getcwd
#endif

///////////////////////////////////////////////////////////////////////////

namespace Nv
{
namespace Blast
{

//////////////////////////////////////////////////////////////////////////////

static void addSlashToPath(std::string& path)
{
    if (path[path.length() - 1] != '/' && path[path.length() - 1] != '\\')
    {
        path.append("/");
    }
}

FileUtils::FileUtils()
{
    mSearchPaths.push_back("");
    char currentPathTemp[FILENAME_MAX];
    if (getCwd(currentPathTemp, sizeof(currentPathTemp)))
    {
        std::string currentPath(currentPathTemp);
        addSlashToPath(currentPath);
        addAbsolutePath(currentPath);
        mCurrentPath = currentPath;
    }
}

std::string FileUtils::getDirectory(const std::string& filePath)
{
    return filePath.substr(0, filePath.find_last_of("/\\") + 1);
}

std::string FileUtils::getFilename(const std::string& filePath, bool bWithExtension)
{
    size_t p0 = filePath.find_last_of("/\\") + 1;
    if (bWithExtension)
    {
        return filePath.substr(p0);
    }
    else
    {
        return filePath.substr(p0, filePath.find_last_of(".") - p0);
    }
}

std::string FileUtils::getFileExtension(const std::string& filePath)
{
    std::string filename = getFilename(filePath);
    size_t p0 = filename.find_last_of(".");
    if (p0 != std::string::npos)
        return filePath.substr(p0);// + 1);
    return "";
}

void FileUtils::addAbsolutePath(const std::string& path)
{
    if (path.empty())
    {
        return;
    }

    std::string newPath = path;
    addSlashToPath(newPath);

    mSearchPaths.push_back(newPath);
}

void FileUtils::addRelativePath(const std::string& relPath)
{
    addAbsolutePath(mCurrentPath + relPath);
}

void FileUtils::clearPaths()
{
    mSearchPaths.clear();
}

FILE* FileUtils::findFile(const std::string& path, bool bVerbose)
{
    FILE* file;
    if (find(path, &file, NULL, bVerbose))
    {
        return file;
    }
    else
    {
        return NULL;
    }
}

std::string FileUtils::findPath(const std::string& path, bool bVerbose)
{
    std::string fullPath;
    if (find(path, NULL, &fullPath, bVerbose))
    {
        return fullPath;
    }
    else
    {
        return path;
    }
}

bool FileUtils::find(const std::string& path, FILE** ppFile, std::string* pFullPath, bool bVerbose)
{
    if (mSearchPaths.empty() || path.empty())
    {
        if (bVerbose)
        {
            lout() << Log::TYPE_ERROR << "Error: Invalid search path configuration.";
        }
        return false;
    }

    std::string fullPath;

    FILE* file = NULL;
    const uint32_t numSearchPaths = (uint32_t)mSearchPaths.size();
    for (uint32_t i = 0; i < numSearchPaths; ++i)
    {
        fullPath = mSearchPaths[i] + path;
        fopen_s(&file, fullPath.c_str(), "rb");
        if (file)
        {
            break;
        }
    }

    if (!file)
    {
        if (bVerbose)
            lout() << Log::TYPE_ERROR << std::endl << "Error: Unable to find file " << path << std::endl;
        return false;
    }

    if (ppFile)
        *ppFile = file;
    else
        fclose(file);

    if (pFullPath)
        *pFullPath = fullPath;

    return true;
}


} // namespace Blast
} // namespace Nv
