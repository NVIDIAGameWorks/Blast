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

#ifndef PS_WINDOWS_TRIG_CONSTANTS_H
#define PS_WINDOWS_TRIG_CONSTANTS_H

//#include "NvMath.h"

#define NV_GLOBALCONST extern const __declspec(selectany)

__declspec(align(16)) struct NV_VECTORF32 
{
    float f[4];
};


//#define NV_PI               3.141592654f
//#define NV_2PI              6.283185307f
//#define NV_1DIVPI           0.318309886f
//#define NV_1DIV2PI          0.159154943f
//#define NV_PIDIV2           1.570796327f
//#define NV_PIDIV4           0.785398163f

NV_GLOBALCONST NV_VECTORF32 g_NVSinCoefficients0    = {{1.0f, -0.166666667f, 8.333333333e-3f, -1.984126984e-4f}};
NV_GLOBALCONST NV_VECTORF32 g_NVSinCoefficients1    = {{2.755731922e-6f, -2.505210839e-8f, 1.605904384e-10f, -7.647163732e-13f}};
NV_GLOBALCONST NV_VECTORF32 g_NVSinCoefficients2    = {{2.811457254e-15f, -8.220635247e-18f, 1.957294106e-20f, -3.868170171e-23f}};
NV_GLOBALCONST NV_VECTORF32 g_NVCosCoefficients0    = {{1.0f, -0.5f, 4.166666667e-2f, -1.388888889e-3f}};
NV_GLOBALCONST NV_VECTORF32 g_NVCosCoefficients1    = {{2.480158730e-5f, -2.755731922e-7f, 2.087675699e-9f, -1.147074560e-11f}};
NV_GLOBALCONST NV_VECTORF32 g_NVCosCoefficients2    = {{4.779477332e-14f, -1.561920697e-16f, 4.110317623e-19f, -8.896791392e-22f}};
NV_GLOBALCONST NV_VECTORF32 g_NVTanCoefficients0    = {{1.0f, 0.333333333f, 0.133333333f, 5.396825397e-2f}};
NV_GLOBALCONST NV_VECTORF32 g_NVTanCoefficients1    = {{2.186948854e-2f, 8.863235530e-3f, 3.592128167e-3f, 1.455834485e-3f}};
NV_GLOBALCONST NV_VECTORF32 g_NVTanCoefficients2    = {{5.900274264e-4f, 2.391290764e-4f, 9.691537707e-5f, 3.927832950e-5f}};
NV_GLOBALCONST NV_VECTORF32 g_NVASinCoefficients0   = {{-0.05806367563904f, -0.41861972469416f, 0.22480114791621f, 2.17337241360606f}};
NV_GLOBALCONST NV_VECTORF32 g_NVASinCoefficients1   = {{0.61657275907170f, 4.29696498283455f, -1.18942822255452f, -6.53784832094831f}};
NV_GLOBALCONST NV_VECTORF32 g_NVASinCoefficients2   = {{-1.36926553863413f, -4.48179294237210f, 1.41810672941833f, 5.48179257935713f}};
NV_GLOBALCONST NV_VECTORF32 g_NVATanCoefficients0   = {{1.0f, 0.333333334f, 0.2f, 0.142857143f}};
NV_GLOBALCONST NV_VECTORF32 g_NVATanCoefficients1   = {{1.111111111e-1f, 9.090909091e-2f, 7.692307692e-2f, 6.666666667e-2f}};
NV_GLOBALCONST NV_VECTORF32 g_NVATanCoefficients2   = {{5.882352941e-2f, 5.263157895e-2f, 4.761904762e-2f, 4.347826087e-2f}};
NV_GLOBALCONST NV_VECTORF32 g_NVSinEstCoefficients  = {{1.0f, -1.66521856991541e-1f, 8.199913018755e-3f, -1.61475937228e-4f}};
NV_GLOBALCONST NV_VECTORF32 g_NVCosEstCoefficients  = {{1.0f, -4.95348008918096e-1f, 3.878259962881e-2f, -9.24587976263e-4f}};
NV_GLOBALCONST NV_VECTORF32 g_NVTanEstCoefficients  = {{2.484f, -1.954923183e-1f, 2.467401101f, NvInvPi}};
NV_GLOBALCONST NV_VECTORF32 g_NVATanEstCoefficients = {{7.689891418951e-1f, 1.104742493348f, 8.661844266006e-1f, NvPiDivTwo}};
NV_GLOBALCONST NV_VECTORF32 g_NVASinEstCoefficients = {{-1.36178272886711f, 2.37949493464538f, -8.08228565650486e-1f, 2.78440142746736e-1f}};
NV_GLOBALCONST NV_VECTORF32 g_NVASinEstConstants    = {{1.00000011921f, NvPiDivTwo, 0.0f, 0.0f}};
NV_GLOBALCONST NV_VECTORF32 g_NVPiConstants0        = {{NvPi, NvTwoPi, NvInvPi, NvInvTwoPi}};
NV_GLOBALCONST NV_VECTORF32 g_NVReciprocalTwoPi     = {{NvInvTwoPi, NvInvTwoPi, NvInvTwoPi, NvInvTwoPi}};
NV_GLOBALCONST NV_VECTORF32 g_NVTwoPi               = {{NvTwoPi, NvTwoPi, NvTwoPi, NvTwoPi}};

#endif
