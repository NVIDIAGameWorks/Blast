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
