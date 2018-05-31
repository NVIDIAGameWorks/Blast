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
// Copyright (c) 2016-2018 NVIDIA Corporation. All rights reserved.


#include "Log.h"

#include "PsString.h"

#include <iomanip>
#include <stdarg.h>
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////

namespace Nv
{
namespace Blast
{

void fLogf(const char* format, ...)
{
	char    buf[4096], *p = buf;
	va_list args;
	int     n;

	va_start(args, format);
	//n = _vsnprintf(p, sizeof buf - 3, format, args);
	n = vsprintf_s(p, sizeof(buf)-3, format, args);
	va_end(args);

	p += (n < 0) ? sizeof buf - 3 : n;

	while (p > buf  &&  isspace((unsigned char)p[-1]))
	{
		*--p = '\0';
	}

	*p++ = '\r';
	*p++ = '\n';
	*p   = '\0';

	fLog(buf, Log::TYPE_INFO);
}


//////////////////////////////////////////////////////////////////////////////

void Log::flushDeferredMessages()
{
	if (mDeferredMessages.size() == 0) return;

	std::cout << std::endl;
	for (std::vector<std::string>::iterator it = mDeferredMessages.begin(); it != mDeferredMessages.end(); ++it)
	{
		log(*it, mMinVerbosity);
	}
	mDeferredMessages.clear();
}



} // namespace Blast
} // namespace Nv
