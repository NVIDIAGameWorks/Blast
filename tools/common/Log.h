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
// Copyright (c) 2016-2020 NVIDIA Corporation. All rights reserved.


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
