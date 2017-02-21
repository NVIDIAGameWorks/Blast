#ifndef CORELIB_GLOBAL_H
#define CORELIB_GLOBAL_H

#include <QtCore/qglobal.h>
#include <Nv/NvBlastCommon.h>

#ifdef CORELIB_LIB
# define CORELIB_EXPORT Q_DECL_EXPORT
#else
# define CORELIB_EXPORT Q_DECL_IMPORT
#endif

#pragma warning(disable:4018)
#pragma warning(disable:4099)
#pragma warning(disable:4101)
#pragma warning(disable:4267)
#pragma warning(disable:4273)
#pragma warning(disable:4996)

#endif // CORELIB_GLOBAL_H
