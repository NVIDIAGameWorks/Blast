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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.


#ifndef NV_C_TYPES_H
#define NV_C_TYPES_H

#include "NvPreprocessor.h"
#ifdef _MSC_VER
#ifndef _INTPTR
#define _INTPTR 0
#endif
#endif
#include <stdint.h>

/**  C type for 2-float vectors */
typedef struct
{
	float x, y;
} NvcVec2;

/**  C type for 3-float vectors */
typedef struct
{
	float x, y, z;
} NvcVec3;

/**  C type for 4-float vectors */
typedef struct
{
	float x, y, z, w;
} NvcVec4;

/**  C type for quaternions */
typedef struct
{
	float x, y, z, w;
} NvcQuat;

/**  C type for transforms */
typedef struct
{
	NvcQuat q;
	NvcVec3 p;
} NvcTransform;

/**  C type for 3x3 matrices */
typedef struct
{
	NvcVec3 column0, column1, column2, column3;
} NvcMat34;

/**  C type for 3x3 matrices */
typedef struct
{
	NvcVec3 column0, column1, column2;
} NvcMat33;

/**  C type for 4x4 matrices */
typedef struct
{
	NvcVec4 column0, column1, column2, column3;
} NvcMat44;

/** C type for 3d bounding box */
typedef struct
{
	NvcVec3 minimum;
	NvcVec3 maximum;
} NvcBounds3;

/** C type for a plane */
typedef struct
{
	NvcVec3 n;
	float d;
} NvcPlane;

/**  C type for 2-integer vectors */
typedef struct
{
	int32_t x, y;
} NvcVec2i;

/**  C type for 3-integer vectors */
typedef struct
{
	int32_t x, y, z;
} NvcVec3i;

/**  C type for 4-integer vectors */
typedef struct
{
	int32_t x, y, z, w;
} NvcVec4i;

/** @} */

#endif // NV_C_TYPES_H
