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
