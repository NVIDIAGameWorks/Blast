/*
 * Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */


#include "NvBlastAssert.h"

#include <stdio.h>
#include <stdlib.h>

#if NV_WINDOWS_FAMILY
#include <crtdbg.h>
#endif

extern "C"
{

void NvBlastAssertHandler(const char* expr, const char* file, int line, bool& ignore)
{
	NV_UNUSED(ignore); // is used only in debug windows config
	char buffer[1024];
#if NV_WINDOWS_FAMILY
	sprintf_s(buffer, 1024, "%s(%d) : Assertion failed: %s\n", file, line, expr);
#else
	sprintf(buffer, "%s(%d) : Assertion failed: %s\n", file, line, expr);
#endif
	puts(buffer);
#if NV_WINDOWS_FAMILY && NV_DEBUG
	// _CrtDbgReport returns -1 on error, 1 on 'retry', 0 otherwise including 'ignore'.
	// Hitting 'abort' will terminate the process immediately.
	int result = _CrtDbgReport(_CRT_ASSERT, file, line, NULL, "%s", buffer);
	int mode = _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_REPORT_MODE);
	ignore = _CRTDBG_MODE_WNDW == mode && result == 0;
	if (ignore)
		return;
	__debugbreak();
#elif (NV_WINDOWS_FAMILY && NV_CHECKED) || NV_CLANG
	__debugbreak();
#else
	abort();
#endif
}

} // extern "C"
