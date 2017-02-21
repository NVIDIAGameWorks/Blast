// This code contains NVIDIA Confidential Information and is disclosed 
// under the Mutual Non-Disclosure Agreement. 
// 
// Notice 
// ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES 
// NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO 
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT, 
// MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 
// 
// NVIDIA Corporation assumes no responsibility for the consequences of use of such 
// information or for any infringement of patents or other rights of third parties that may 
// result from its use. No license is granted by implication or otherwise under any patent 
// or patent rights of NVIDIA Corporation. No third party distribution is allowed unless 
// expressly authorized by NVIDIA.  Details are subject to change without notice. 
// This code supersedes and replaces all information previously supplied. 
// NVIDIA Corporation products are not authorized for use as critical 
// components in life support devices or systems without express written approval of 
// NVIDIA Corporation. 
// 
// Copyright (c) 2013 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//
#pragma once

#include "corelib_global.h"
#include "CoreLib.h"
#include "math.h"
#include "float.h"

////////////////////////////////////////////////////////////////////////////////////////
inline float gfsdk_lerp(float v1, float v2, float t)
{
	return (1.0f - t) * v1 + t * v2;
}

inline float gfsdk_min(float x, float y) {
	return (x < y) ? x : y;
}

inline float gfsdk_max(float x, float y) {
	return (x > y) ? x : y;
}

////////////////////////////////////////////////////////////////////////////////////////
inline atcore_float2 gfsdk_makeFloat2(float x, float y)
{
	atcore_float2 v;
	v.x = x;
	v.y = y;
	return v;
}

////////////////////////////////////////////////////////////////////////////////////////
inline atcore_float2 operator*(float s, const atcore_float2 &p) 
{
	return gfsdk_makeFloat2(s * p.x, s * p.y);
}

////////////////////////////////////////////////////////////////////////////////////////
inline atcore_float2 operator*(const atcore_float2 &p, float s) 
{
	return gfsdk_makeFloat2(s * p.x, s * p.y);
}


////////////////////////////////////////////////////////////////////////////////////////
inline atcore_float3 gfsdk_makeFloat3(float x, float y, float z)
{
	atcore_float3 v;
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}

////////////////////////////////////////////////////////////////////////////////////////
inline atcore_float3 operator+(const atcore_float3 &p0, const atcore_float3 &p1) 
{
	return gfsdk_makeFloat3(p0.x + p1.x, p0.y + p1.y, p0.z + p1.z);
}

////////////////////////////////////////////////////////////////////////////////////////
inline atcore_float3& operator+=(atcore_float3 &v, const atcore_float3 & v1)
{
	v.x += v1.x; v.y += v1.y; v.z += v1.z;
	return v;
}

////////////////////////////////////////////////////////////////////////////////////////
inline atcore_float3 operator-(const atcore_float3 &p0, const atcore_float3 &p1) 
{
	return gfsdk_makeFloat3(p0.x - p1.x, p0.y - p1.y, p0.z - p1.z);
}

////////////////////////////////////////////////////////////////////////////////////////
inline atcore_float3& operator-=(atcore_float3 &v, const atcore_float3 & v1)
{
	v.x -= v1.x; v.y -= v1.y; v.z -= v1.z;
	return v;
}

////////////////////////////////////////////////////////////////////////////////////////
inline atcore_float3 operator*(float s, const atcore_float3 &p) 
{
	return gfsdk_makeFloat3(s * p.x, s * p.y, s * p.z);
}

////////////////////////////////////////////////////////////////////////////////////////
inline atcore_float3 operator*(const atcore_float3 &p, float s) 
{
	return gfsdk_makeFloat3(s * p.x, s * p.y, s * p.z);
}

////////////////////////////////////////////////////////////////////////////////////////
inline float gfsdk_dot(const atcore_float3& v0, const atcore_float3 &v1) {
	return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
}

////////////////////////////////////////////////////////////////////////////////////////
inline float gfsdk_lengthSquared(const atcore_float3& v) {
	return gfsdk_dot(v,v);
}

////////////////////////////////////////////////////////////////////////////////////////
inline float gfsdk_length(const atcore_float3 &v) {
	return sqrt(gfsdk_lengthSquared(v));
}

////////////////////////////////////////////////////////////////////////////////////////
inline float gfsdk_distance(const atcore_float3 &v1, const atcore_float3 &v2) {
	return gfsdk_length(v2-v1);
}

////////////////////////////////////////////////////////////////////////////////////////
inline const atcore_float3& gfsdk_normalize(atcore_float3 &v) {
	float l = gfsdk_length(v);
	if (l != 0) {	v.x /= l;	v.y /= l;	v.z /= l; }
	return v;
}

////////////////////////////////////////////////////////////////////////////////////////
inline atcore_float3 gfsdk_cross(const atcore_float3& v1, const atcore_float3& v2)
{
	return gfsdk_makeFloat3(
		v1.y * v2.z - v1.z * v2.y, 
		v1.z * v2.x - v1.x * v2.z,
		v1.x * v2.y - v1.y * v2.x);
}

////////////////////////////////////////////////////////////////////////////////////////
inline atcore_float3 gfsdk_lerp(const atcore_float3& v1, const atcore_float3& v2, float t)
{
	return gfsdk_makeFloat3(gfsdk_lerp(v1.x, v2.x, t), gfsdk_lerp(v1.y, v2.y, t), gfsdk_lerp(v1.z, v2.z, t));
}

////////////////////////////////////////////////////////////////////////////////////////
inline atcore_float3 gfsdk_min(const atcore_float3& v1, const atcore_float3 &v2)
{
	return gfsdk_makeFloat3(
		gfsdk_min(v1.x, v2.x),
		gfsdk_min(v1.y, v2.y),
		gfsdk_min(v1.z, v2.z)
		);
}

////////////////////////////////////////////////////////////////////////////////////////
inline atcore_float3 gfsdk_max(const atcore_float3& v1, const atcore_float3 &v2)
{
	return gfsdk_makeFloat3(
		gfsdk_max(v1.x, v2.x),
		gfsdk_max(v1.y, v2.y),
		gfsdk_max(v1.z, v2.z)
		);
}

////////////////////////////////////////////////////////////////////////////////////////
inline float gfsdk_min(const atcore_float3 &v)
{
	return gfsdk_min(gfsdk_min(v.x, v.y), v.z);
}

////////////////////////////////////////////////////////////////////////////////////////
inline float gfsdk_max(const atcore_float3 &v)
{
	return gfsdk_max(gfsdk_max(v.x, v.y), v.z);
}

////////////////////////////////////////////////////////////////////////////////
// float4
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
inline atcore_float4 gfsdk_makeFloat4(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 0.0f)
{
	atcore_float4 v;
	v.x = x;
	v.y = y;
	v.z = z;
	v.w = w;
	return v;
}

////////////////////////////////////////////////////////////////////////////////////////
inline atcore_float4 gfsdk_makeFloat4(const atcore_float3& v, float w)
{
	return gfsdk_makeFloat4(v.x, v.y, v.z, w);
}

////////////////////////////////////////////////////////////////////////////////////////
inline atcore_float4& operator+=(atcore_float4 &v, const atcore_float4 & v1)
{
	v.x += v1.x; v.y += v1.y; v.z += v1.z; v.w += v1.w;
	return v;
}

////////////////////////////////////////////////////////////////////////////////////////
inline atcore_float4 operator*(float s, const atcore_float4 &p) 
{
	return gfsdk_makeFloat4(s * p.x, s * p.y, s * p.z, s * p.w);
}

////////////////////////////////////////////////////////////////////////////////////////
inline atcore_float4 operator*(const atcore_float4 &p, float s) 
{
	return gfsdk_makeFloat4(s * p.x, s * p.y, s * p.z, s * p.w);
}

////////////////////////////////////////////////////////////////////////////////////////
inline float gfsdk_dot(const atcore_float4& v0, const atcore_float4 &v1) {
	return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z + v0.w * v1.w;
}

////////////////////////////////////////////////////////////////////////////////////////
inline float gfsdk_lengthSquared(const atcore_float4& v) {
	return gfsdk_dot(v,v);
}

////////////////////////////////////////////////////////////////////////////////////////
inline float gfsdk_length(const atcore_float4 &v) {
	return sqrt(gfsdk_lengthSquared(v));
}

////////////////////////////////////////////////////////////////////////////////////////
inline const atcore_float4 gfsdk_normalize(const atcore_float4 &v) {
	atcore_float4 nv = v;

	float l = gfsdk_length(nv);
	if (l > FLT_EPSILON)
	{
		const float s = 1.0f / l;

		nv.x *= s; 
		nv.y *= s; 
		nv.z *= s; 
		nv.w *= s;
	}

	return nv;
}

////////////////////////////////////////////////////////////////////////////////////////
inline atcore_float4 gfsdk_lerp(const atcore_float4& v1, const atcore_float4& v2, float t)
{
	return gfsdk_makeFloat4(
		gfsdk_lerp(v1.x, v2.x, t), 
		gfsdk_lerp(v1.y, v2.y, t), 
		gfsdk_lerp(v1.z, v2.z, t),
		gfsdk_lerp(v1.w, v2.w, t)
		);
}

////////////////////////////////////////////////////////////////////////////////
// quaternion
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
inline atcore_float4 gfsdk_quaternionConjugate(const atcore_float4& in)
{
	return gfsdk_makeFloat4(-in.x, -in.y, -in.z, in.w);
}

////////////////////////////////////////////////////////////////////////////////
inline atcore_float4 gfsdk_quaternionMultiply(const atcore_float4& q0, const atcore_float4& q1)
{
	atcore_float4 q;

	const float tx = q0.w * q1.x + q0.x * q1.w + q0.y * q1.z - q0.z * q1.y;
	const float ty = q0.w * q1.y + q0.y * q1.w + q0.z * q1.x - q0.x * q1.z;
	const float tz = q0.w * q1.z + q0.z * q1.w + q0.x * q1.y - q0.y * q1.x;

	q.w = q0.w * q1.w - q0.x * q1.x - q0.y * q1.y - q0.z  * q1.z;
	q.x = tx;
	q.y = ty;
	q.z = tz;

	return q;
}

////////////////////////////////////////////////////////////////////////////////
// 4x4 matrix
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
CORELIB_EXPORT void gfsdk_makeIdentity(atcore_float4x4& transform);
CORELIB_EXPORT void gfsdk_makeTranslation(atcore_float4x4& m, const atcore_float3& t);
CORELIB_EXPORT void gfsdk_makeScale(atcore_float4x4& m, const atcore_float3& s);
CORELIB_EXPORT void gfsdk_makeRotation(atcore_float4x4& m, const atcore_float4& q);

////////////////////////////////////////////////////////////////////////////////////////
inline atcore_float4x4& operator*=(atcore_float4x4& m, float s)
{
	float* data = (float*)&m;
	for (int i = 0; i < 16; i++)
		data[i] *= s;

	return m;
}

////////////////////////////////////////////////////////////////////////////////////////
inline void gfsdk_setPosition(atcore_float4x4&m, const atcore_float3& v)
{
	m._41 = v.x;
	m._42 = v.y;
	m._43 = v.z;
}

////////////////////////////////////////////////////////////////////////////////////////
inline atcore_float4x4 gfsdk_transpose(const atcore_float4x4& m)
{
	atcore_float4x4 om;

	om._11 = m._11; om._12 = m._21;	om._13 = m._31;	om._14 = m._41;
	om._21 = m._12; om._22 = m._22;	om._23 = m._32;	om._24 = m._42;
	om._31 = m._13; om._32 = m._23;	om._33 = m._33;	om._34 = m._43;
	om._41 = m._14; om._42 = m._24;	om._43 = m._34;	om._44 = m._44;

	return om;
}

////////////////////////////////////////////////////////////////////////////////////////
inline atcore_float4x4& operator+=(atcore_float4x4& m1, const atcore_float4x4& m2)
{
	float* data1 = (float*)&m1;
	float* data2 = (float*)&m2;

	for (int i = 0; i < 16; i++)
		data1[i] += data2[i];

	return m1;
}

////////////////////////////////////////////////////////////////////////////////
inline atcore_float4 gfsdk_transform(const atcore_float4x4 &m, atcore_float4 op)
{
	atcore_float4 p;
	
	p.x = op.x * m._11 + op.y * m._21 + op.z * m._31 + op.w * m._41;
	p.y = op.x * m._12 + op.y * m._22 + op.z * m._32 + op.w * m._42;
	p.z = op.x * m._13 + op.y * m._23 + op.z * m._33 + op.w * m._43;
	p.w = op.x * m._14 + op.y * m._24 + op.z * m._34 + op.w * m._44;

	return p;
}

////////////////////////////////////////////////////////////////////////////////
inline atcore_float3 gfsdk_transformCoord(const atcore_float4x4 &m, atcore_float3 op)
{
	atcore_float3 p;
	
	p.x = op.x * m._11 + op.y * m._21 + op.z * m._31 + m._41;
	p.y = op.x * m._12 + op.y * m._22 + op.z * m._32 + m._42;
	p.z = op.x * m._13 + op.y * m._23 + op.z * m._33 + m._43;

	return p;
}

////////////////////////////////////////////////////////////////////////////////
inline atcore_float3 gfsdk_transformVector(const atcore_float4x4 &m, atcore_float3 op)
{
	atcore_float3 p;
	
	p.x = op.x * m._11 + op.y * m._21 + op.z * m._31;
	p.y = op.x * m._12 + op.y * m._22 + op.z * m._32;
	p.z = op.x * m._13 + op.y * m._23 + op.z * m._33;

	return p;
}

////////////////////////////////////////////////////////////////////////////////
inline atcore_float3 gfsdk_getScale(const atcore_float4x4& m)
{
	atcore_float3 ax = gfsdk_makeFloat3(m._11, m._12, m._13);
	atcore_float3 ay = gfsdk_makeFloat3(m._21, m._22, m._23);
	atcore_float3 az = gfsdk_makeFloat3(m._31, m._32, m._33);

	return gfsdk_makeFloat3(gfsdk_length(ax), gfsdk_length(ay), gfsdk_length(az));
}

////////////////////////////////////////////////////////////////////////////////
inline atcore_float3 gfsdk_getTranslation(const atcore_float4x4& m)
{
	return gfsdk_makeFloat3(m._41, m._42, m._43);
}

////////////////////////////////////////////////////////////////////////////////
inline void gfsdk_setTranslation(atcore_float4x4& m, const atcore_float3 &v)
{
	m._41 = v.x;
	m._42 = v.y;
	m._43 = v.z;
}

////////////////////////////////////////////////////////////////////////////////
inline const atcore_float4x4& gfsdk_orthonormalize(atcore_float4x4& m)
{
	atcore_float3 ax = gfsdk_makeFloat3(m._11, m._12, m._13);
	atcore_float3 ay = gfsdk_makeFloat3(m._21, m._22, m._23);
	atcore_float3 az = gfsdk_makeFloat3(m._31, m._32, m._33);

	gfsdk_normalize(ax);
	gfsdk_normalize(ay);
	gfsdk_normalize(az);

	m._11 = ax.x; m._12 = ax.y; m._13 = ax.z;
	m._21 = ay.x; m._22 = ay.y; m._23 = ay.z;
	m._31 = az.x; m._32 = az.y; m._33 = az.z;

	return m;
}

////////////////////////////////////////////////////////////////////////////////
CORELIB_EXPORT atcore_float4x4 gfsdk_lerp(atcore_float4x4& start, atcore_float4x4& end, float t);

////////////////////////////////////////////////////////////////////////////////
CORELIB_EXPORT atcore_float4 gfsdk_getRotation(const atcore_float4x4& m);

////////////////////////////////////////////////////////////////////////////////
CORELIB_EXPORT atcore_float4x4 gfsdk_makeTransform(const atcore_float4& q, const atcore_float3& t, const atcore_float3 &s);

////////////////////////////////////////////////////////////////////////////////
CORELIB_EXPORT atcore_float4x4 operator*(const atcore_float4x4& in1, const atcore_float4x4& in2);

////////////////////////////////////////////////////////////////////////////////
CORELIB_EXPORT atcore_float4x4 gfsdk_inverse(const atcore_float4x4& m);

////////////////////////////////////////////////////////////////////////////////
// dual quaternion 
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
inline atcore_dualquaternion gfsdk_makeDQ(const atcore_float4& q, const atcore_float3& t)
{
	atcore_dualquaternion dq;

	dq.q0 = gfsdk_normalize(q);
	dq.q1 = gfsdk_quaternionMultiply(gfsdk_makeFloat4(t, 0), dq.q0) * 0.5f;

	return dq;
}

////////////////////////////////////////////////////////////////////////////////
atcore_dualquaternion gfsdk_makeDQ(const atcore_float4x4& m);

////////////////////////////////////////////////////////////////////////////////
inline const atcore_dualquaternion& gfsdk_normalize(atcore_dualquaternion & dq)
{
	float mag = gfsdk_dot( dq.q0, dq.q0);
	float deLen = 1.0f / sqrt(mag+FLT_EPSILON);
	
	dq.q0 = dq.q0 * deLen;
	dq.q1 = dq.q1 * deLen;

	return dq;
}

////////////////////////////////////////////////////////////////////////////////
inline atcore_dualquaternion operator*(float s, const atcore_dualquaternion & dq)
{
	return atcore_dualquaternion(s * dq.q0, s * dq.q1);
}

////////////////////////////////////////////////////////////////////////////////
inline atcore_dualquaternion operator*(const atcore_dualquaternion & dq, float s)
{
	return atcore_dualquaternion(s * dq.q0, s * dq.q1);
}

////////////////////////////////////////////////////////////////////////////////
inline atcore_dualquaternion& operator+=(atcore_dualquaternion &dq, const atcore_dualquaternion & dq2)
{
	// hemispherization
	float sign = (gfsdk_dot(dq.q0, dq2.q0) < -FLT_EPSILON) ? -1.0f: 1.0f;

	dq.q0 += sign * dq2.q0;
	dq.q1 += sign * dq2.q1;

	return dq;
}

////////////////////////////////////////////////////////////////////////////////
inline atcore_dualquaternion gfsdk_lerp(const atcore_dualquaternion &dq1, const atcore_dualquaternion & dq2, float t)
{
	atcore_dualquaternion dq = dq1 * (1.0f - t);

	float sign = (gfsdk_dot(dq1.q0, dq2.q0) < -FLT_EPSILON) ? -1.0f: 1.0f;

	dq += (t * sign) * dq2;
	gfsdk_normalize(dq);

	return dq;
}

////////////////////////////////////////////////////////////////////////////////
inline atcore_float3 gfsdk_transformCoord(const atcore_dualquaternion& dq, const atcore_float3 &vecIn) 
{
	atcore_float3 d0 = gfsdk_makeFloat3(dq.q0.x, dq.q0.y, dq.q0.z);
	atcore_float3 de = gfsdk_makeFloat3(dq.q1.x, dq.q1.y, dq.q1.z);
	float a0 = dq.q0.w;
	float ae = dq.q1.w;

	atcore_float3 temp = gfsdk_cross( d0, vecIn ) + a0 * vecIn;
	atcore_float3 temp2 = 2.0f * (a0 * de - ae * d0 + gfsdk_cross(d0, de));

	return vecIn + temp2 + 2.0f * gfsdk_cross( d0, temp);
}

////////////////////////////////////////////////////////////////////////////////
inline atcore_float3 gfsdk_transformVector(const atcore_dualquaternion& dq, const atcore_float3 &vecIn) 
{
	atcore_float3 d0 = gfsdk_makeFloat3(dq.q0.x, dq.q0.y, dq.q0.z);
	atcore_float3 de = gfsdk_makeFloat3(dq.q1.x, dq.q1.y, dq.q1.z);
	float a0 = dq.q0.w;
	float ae = dq.q1.w;

	atcore_float3 temp = gfsdk_cross( d0, vecIn ) + a0 * vecIn;
	return vecIn + 2.0f * gfsdk_cross( d0, temp);
}

////////////////////////////////////////////////////////////////////////////////
inline atcore_float4 getRotation(const atcore_dualquaternion& dq)
{
	return dq.q0;
}

////////////////////////////////////////////////////////////////////////////////
inline atcore_float3 getTranslation(const atcore_dualquaternion& dq)
{
	atcore_float4 dual = 2.0f * dq.q1;
	atcore_float4 t = gfsdk_quaternionMultiply(dual, gfsdk_quaternionConjugate( dq.q0 ));

	return gfsdk_makeFloat3(t.x, t.y, t.z);
}

