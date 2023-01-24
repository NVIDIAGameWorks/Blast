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
// Copyright (c) 2008-2014 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

#ifndef NV_NSFOUNDATION_NSINTRINSICS_H
#define NV_NSFOUNDATION_NSINTRINSICS_H

#include "NvPreprocessor.h"

#if(NV_WINDOWS_FAMILY || NV_WINRT)
#include "platform/windows/NsWindowsIntrinsics.h"
#elif NV_X360
#include "xbox360/NsXbox360Intrinsics.h"
#elif(NV_LINUX || NV_ANDROID || NV_APPLE_FAMILY || NV_PS4)
#include "platform/unix/NsUnixIntrinsics.h"
#elif NV_PS3
#include "ps3/NsPS3Intrinsics.h"
#elif NV_PSP2
#include "psp2/NsPSP2Intrinsics.h"
#elif NV_WIIU
#include "wiiu/NsWiiUIntrinsics.h"
#elif NV_XBOXONE
#include "XboxOne/NsXboxOneIntrinsics.h"
#else
#error "Platform not supported!"
#endif

#endif // #ifndef NV_NSFOUNDATION_NSINTRINSICS_H
