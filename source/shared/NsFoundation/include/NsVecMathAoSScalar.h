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

#ifndef NV_PHYSICS_COMMON_VECMATH_INLINE_SCALAR
#define NV_PHYSICS_COMMON_VECMATH_INLINE_SCALAR

#if COMPILE_VECTOR_INTRINSICS
#error Scalar version should not be included when using vector intrinsics.
#endif

//Remove this define when all platforms use simd solver.
#define NV_SUPPORT_SIMD


struct VecU8V;
struct VecI16V;
struct VecU16V;
struct VecI32V;
struct VecU32V;
struct Vec4V;
typedef Vec4V QuatV;

NV_ALIGN_PREFIX(16)
struct FloatV
{
    float x;
    float pad[3];
    FloatV(){}
    FloatV(const float _x)
        : x(_x)
    {
    }
}
NV_ALIGN_SUFFIX(16);

NV_ALIGN_PREFIX(16)
struct Vec4V
{
    float x, y, z, w;
    Vec4V(){}
    Vec4V(const float _x, const float _y, const float _z, const float _w)
        : x(_x),
          y(_y),
          z(_z),
          w(_w)
    {
    }
}
NV_ALIGN_SUFFIX(16);

NV_ALIGN_PREFIX(16)
struct Vec3V
{
    float x,y,z;
    float pad;
    Vec3V(){}
    Vec3V(const float _x, const float _y, const float _z)
        : x(_x),
          y(_y),
          z(_z),
          pad(0.0f)
    {
    }
}
NV_ALIGN_SUFFIX(16);

NV_ALIGN_PREFIX(16)
struct BoolV
{
    uint32_t ux, uy, uz, uw;
    BoolV(){}
    BoolV(const uint32_t _x, const uint32_t _y, const uint32_t _z, const uint32_t _w)
        : ux(_x),
          uy(_y),
          uz(_z),
          uw(_w)
    {
    }
}
NV_ALIGN_SUFFIX(16);

struct Mat33V
{
    Mat33V(){}
    Mat33V(const Vec3V& c0, const Vec3V& c1, const Vec3V& c2)
        : col0(c0),
          col1(c1),
          col2(c2)
    {
    }
    Vec3V col0;
    Vec3V col1;
    Vec3V col2;
};

struct Mat34V
{
    Mat34V(){}
    Mat34V(const Vec3V& c0, const Vec3V& c1, const Vec3V& c2, const Vec3V& c3)
        : col0(c0),
          col1(c1),
          col2(c2),
          col3(c3)
    {
    }
    Vec3V col0;
    Vec3V col1;
    Vec3V col2;
    Vec3V col3;
};

struct Mat43V
{
    Mat43V(){}
    Mat43V(const Vec4V& c0, const Vec4V& c1, const Vec4V& c2)
        : col0(c0),
          col1(c1),
          col2(c2)
    {
    }
    Vec4V col0;
    Vec4V col1;
    Vec4V col2;
};

struct Mat44V
{
    Mat44V(){}
    Mat44V(const Vec4V& c0, const Vec4V& c1, const Vec4V& c2, const Vec4V& c3)
        : col0(c0),
          col1(c1),
          col2(c2),
          col3(c3)
    {
    }
    Vec4V col0;
    Vec4V col1;
    Vec4V col2;
    Vec4V col3;
};

NV_ALIGN_PREFIX(16)
struct VecU32V
{
    uint32_t u32[4];
    NV_FORCE_INLINE VecU32V() {}
    NV_FORCE_INLINE VecU32V(uint32_t a, uint32_t b, uint32_t c, uint32_t d) { u32[0] = a; u32[1] = b; u32[2] = c; u32[3] = d; }
}
NV_ALIGN_SUFFIX(16);

NV_ALIGN_PREFIX(16)
struct VecI32V
{
    int32_t i32[4];
    NV_FORCE_INLINE VecI32V() {}
    NV_FORCE_INLINE VecI32V(int32_t a, int32_t b, int32_t c, int32_t d) { i32[0] = a; i32[1] = b; i32[2] = c; i32[3] = d; }
}
NV_ALIGN_SUFFIX(16);

NV_ALIGN_PREFIX(16)
struct VecI16V
{
    int16_t i16[8];
    NV_FORCE_INLINE VecI16V() {}
    NV_FORCE_INLINE VecI16V(int16_t a, int16_t b, int16_t c, int16_t d, int16_t e, int16_t f, int16_t g, int16_t h)
        { i16[0] = a; i16[1] = b; i16[2] = c; i16[3] = d; i16[4] = e; i16[5] = f; i16[6] = g; i16[7] = h; }
}
NV_ALIGN_SUFFIX(16);

NV_ALIGN_PREFIX(16)
struct VecU16V
{
    union { uint16_t u16[8]; int16_t i16[8]; };
    NV_FORCE_INLINE VecU16V() {}
    NV_FORCE_INLINE VecU16V(uint16_t a, uint16_t b, uint16_t c, uint16_t d, uint16_t e, uint16_t f, uint16_t g, uint16_t h)
        { u16[0] = a; u16[1] = b; u16[2] = c; u16[3] = d; u16[4] = e; u16[5] = f; u16[6] = g; u16[7] = h; }
}
NV_ALIGN_SUFFIX(16);

NV_ALIGN_PREFIX(16)
struct VecU8V
{
    uint8_t u8[8];
    NV_FORCE_INLINE VecU8V() {}
    NV_FORCE_INLINE VecU8V(uint8_t a, uint8_t b, uint8_t c, uint8_t d) { u8[0] = a; u8[1] = b; u8[2] = c; u8[3] = d; }
}
NV_ALIGN_SUFFIX(16);


#define FloatVArg   FloatV&
#define Vec3VArg    Vec3V&
#define Vec4VArg    Vec4V&
#define BoolVArg    BoolV&
#define VecU32VArg  VecU32V&
#define VecI32VArg  VecI32V&
#define VecU16VArg  VecU16V&
#define VecI16VArg  VecI16V&
#define VecU8VArg   VecU8V&
#define QuatVArg    QuatV&

#define VecCrossV Vec3V

typedef VecI32V VecShiftV;
#define VecShiftVArg VecShiftV&

#endif //NV_PHYSICS_COMMON_VECMATH_INLINE_SCALAR



