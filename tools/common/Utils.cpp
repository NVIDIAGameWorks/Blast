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
