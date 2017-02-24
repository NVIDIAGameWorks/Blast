#pragma once

/**
Function pointer type for allocation - has same signature as stdlib malloc.
*/
typedef void*	(*NvBlastExtAlloc)(size_t size);

extern NvBlastExtAlloc gAlloc;
extern NvBlastLog gLog;

