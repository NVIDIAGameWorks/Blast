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
// Copyright (c) 2016-2022 NVIDIA Corporation. All rights reserved.


#ifndef LOG_H
#define LOG_H

#include "Utils.h"
#include "PxVec3.h"

#include <sstream>

namespace Nv
{
namespace Blast
{


//////////////////////////////////////////////////////////////////////////////

void fLogf(const char* format, ...);

class Log : public Singleton<Log>
{
    friend class Singleton<Log>;

public:

    enum MessageType {
        TYPE_INFO = 0,
        TYPE_WARNING,
        TYPE_ERROR,
        TYPE_DEFERRED,

        NUM_TYPES,
        MOST_VERBOSE  = TYPE_INFO,
        LEAST_VERBOSE = TYPE_ERROR
#if defined(_DEBUG)
        , DEFAULT_VERBOSITY = MOST_VERBOSE
#else
        , DEFAULT_VERBOSITY = LEAST_VERBOSE
#endif
    };
    typedef MessageType Verbosity;

    ///////////////////////////////////////////////////////////////////////////

    template<typename T>
    Log& log(const T& value, MessageType messageType);

    void flushDeferredMessages();

    ///////////////////////////////////////////////////////////////////////////

    void setCurrentVerbosity(Verbosity verbosity) { mCurrentVerbosity = verbosity; }
    Verbosity getCurrentVerbosity() const { return mCurrentVerbosity; }

    // Messages types below this level will be ignored
    void setMinVerbosity(Verbosity verbosity)     { mMinVerbosity     = verbosity; }
    Verbosity getMinVerbosity()     const { return mMinVerbosity; }

    ///////////////////////////////////////////////////////////////////////////

protected:
    Log(MessageType verbosity = DEFAULT_VERBOSITY)
        : mCurrentVerbosity(LEAST_VERBOSE),
        mMinVerbosity(verbosity) { }

private:
    Verbosity mCurrentVerbosity;
    Verbosity mMinVerbosity;
    std::vector<std::string> mDeferredMessages;
};

///////////////////////////////////////////////////////////////////////////

PX_INLINE std::ostream& operator<< (std::ostream& stream, const physx::PxVec3& vec)
{
    return stream << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
}

template<typename T>
Log& Log::log(const T& value, Log::MessageType messageType)
{
    if (TYPE_DEFERRED == messageType)
    {
        std::stringstream ss;
        ss << value;
        mDeferredMessages.push_back(ss.str());
    }
    else if(mMinVerbosity <= messageType)
    {
        std::cout << value;
    }
    return *this;
}

PX_INLINE Log& lout() { return Log::instance(); }

template <typename T>
PX_INLINE void fLog(const T& value, Log::MessageType messageType = Log::TYPE_INFO)
{
    lout().log<T>(value, messageType);
}
template <typename T>
PX_INLINE Log& operator<<(Log& logger, const T& value)
{
    return logger.log<T>(value, logger.getCurrentVerbosity());
}
PX_INLINE Log& operator<<(Log& logger, Log::MessageType verbosity)
{
    logger.setCurrentVerbosity(verbosity);
    return logger;
}
typedef std::ostream& (*ostream_manipulator)(std::ostream&);
PX_INLINE Log& operator<<(Log& logger, ostream_manipulator pf)
{
    return operator<< <ostream_manipulator> (logger, pf);
}


} // namespace Blast
} // namespace Nv


#endif
