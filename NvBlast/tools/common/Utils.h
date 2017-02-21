#ifndef UTILS_H
#define UTILS_H

#include "PsString.h"

#include <string>
#include <iostream>
#include <vector>
#include <map>

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////

namespace Nv
{
namespace Blast
{

///////////////////////////////////////////////////////////////////////////

template<class T>
PX_INLINE bool isNull(const T* p) { return nullptr == p; }

template<class Releasable, class Releaser> class ScopedResource;
template<class Releasable, class Releaser>
PX_INLINE bool isNull(const ScopedResource<Releasable,Releaser>& p) { return !p; }

PX_INLINE bool isNullString(const char* pString)
{
	return (nullptr == pString || pString[0] == '\0' || physx::shdfnd::strcmp(pString, "null") == 0);
}

template<class T>
PX_INLINE bool isValid(const T& p) { return !isNull(p.get()); }

PX_INLINE bool isValidString(const char* pString) { return !isNullString(pString); }

///////////////////////////////////////////////////////////////////////////

// Note: This is not a thread safe singleton class
template <class T>
class Singleton
{
	// The fact that I cannot declare T a friend directly is rather absurd...
	typedef T Type;
	friend typename Singleton<T>::Type;

	//////////////////////////////////////////////////////////////////////////////

public:
	static T& instance()
	{
		static T _instance;
		return _instance;
	}

	//////////////////////////////////////////////////////////////////////////////

private:
	Singleton() { }
	~Singleton() { };
	Singleton(const Singleton&);
	Singleton& operator=(const Singleton&);
};

//////////////////////////////////////////////////////////////////////////////

class FileUtils : public Singleton<FileUtils>
{
	friend class Singleton<FileUtils>;

public:
	void        addAbsolutePath(const std::string&);
	void        addRelativePath(const std::string&);
	void        clearPaths();

	//////////////////////////////////////////////////////////////////////////////

	FILE*       findFile(const std::string&, bool bVerbose = true);
	std::string findPath(const std::string&, bool bVerbose = true);
	bool        find(const std::string&, FILE**, std::string*, bool bVerbose = true);

	//////////////////////////////////////////////////////////////////////////////

	const std::string& getCurrentPath() const
	{
		return mCurrentPath;
	}

	//////////////////////////////////////////////////////////////////////////////

	static std::string getDirectory(const std::string&);
	static std::string getFilename(const std::string&, bool bWithExtension = true);
	static std::string getFileExtension(const std::string&);

	//////////////////////////////////////////////////////////////////////////////

protected:
	FileUtils();

	//////////////////////////////////////////////////////////////////////////////

	std::string              mCurrentPath;
	std::vector<std::string> mSearchPaths;
};


} // namespace Blast
} // namespace Nv


#endif
