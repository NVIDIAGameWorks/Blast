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
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#ifndef PS_INLINE_AOS_H
#define PS_INLINE_AOS_H

#include "NvPreprocessor.h"

#if NV_WINDOWS_FAMILY
    #include "platform/windows/NsWindowsTrigConstants.h"
    #include "platform/windows/NsWindowsInlineAoS.h"
#elif NV_X360
    #include "xbox360/NsXbox360InlineAoS.h"
#elif (NV_LINUX || NV_ANDROID || NV_APPLE || NV_PS4 || (NV_WINRT && NV_NEON))
    #include "platform/unix/NsUnixTrigConstants.h"
    #include "platform/unix/NsUnixInlineAoS.h"
#elif NV_PS3
    #include "ps3/NsPS3InlineAoS.h"
#elif NV_PSP2
    #include "psp2/NsPSP2InlineAoS.h"
#elif NV_XBOXONE
    #include "XboxOne/NsXboxOneTrigConstants.h"
    #include "XboxOne/NsXboxOneInlineAoS.h"
#else
    #error "Platform not supported!"
#endif

#endif

