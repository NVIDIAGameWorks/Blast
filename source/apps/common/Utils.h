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
// Copyright (c) 2016-2023 NVIDIA Corporation. All rights reserved.


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
    return (nullptr == pString || pString[0] == '\0' || nvidia::shdfnd::strcmp(pString, "null") == 0);
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
