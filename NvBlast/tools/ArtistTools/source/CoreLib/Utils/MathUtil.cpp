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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This file contains wrapper functions to make hair lib easy to setup and use
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "MathUtil.h"
#include <string.h>

////////////////////////////////////////////////////////////////////////////////////////
void gfsdk_makeIdentity(atcore_float4x4& transform)
{
	memset(&transform, 0, sizeof(atcore_float4x4));
	transform._11 = 1.0f;
	transform._22 = 1.0f;
	transform._33 = 1.0f;
	transform._44 = 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////
void gfsdk_makeTranslation(atcore_float4x4& m, const atcore_float3& t)
{
	gfsdk_makeIdentity(m);
	m._41 = t.x;
	m._42 = t.y;
	m._43 = t.z;
}

////////////////////////////////////////////////////////////////////////////////////////
void gfsdk_makeScale(atcore_float4x4& m, const atcore_float3& s)
{
	gfsdk_makeIdentity(m);
	m._11 = s.x;
	m._12 = s.y;
	m._13 = s.z;
}

////////////////////////////////////////////////////////////////////////////////
void gfsdk_makeRotation(atcore_float4x4& m, const atcore_float4& q)
{
	gfsdk_makeIdentity(m);

	const float x = q.x;
	const float y = q.y;
	const float z = q.z;
	const float w = q.w;

	const float x2 = x + x;
	const float y2 = y + y;
	const float z2 = z + z;

	const float xx = x2*x;
	const float yy = y2*y;
	const float zz = z2*z;

	const float xy = x2*y;
	const float xz = x2*z;
	const float xw = x2*w;

	const float yz = y2*z;
	const float yw = y2*w;
	const float zw = z2*w;

	m._11 = 1.0f - yy - zz;
	m._12 = xy + zw;
	m._13 = xz - yw;

	m._21 = xy - zw;
	m._22 = 1.0f - xx - zz;
	m._23 = yz + xw;

	m._31 = xz + yw;
	m._32 = yz - xw;
	m._33 = 1.0f - xx - yy;
}

////////////////////////////////////////////////////////////////////////////////
atcore_float4x4 gfsdk_makeTransform(const atcore_float4& q, const atcore_float3& t, const atcore_float3 &s)
{
	atcore_float4x4 m;

	gfsdk_makeRotation(m, q);

	m._11 *= s.x; m._12 *= s.x; m._13 *= s.x;
	m._21 *= s.y; m._22 *= s.y; m._23 *= s.y;
	m._31 *= s.z; m._32 *= s.z; m._33 *= s.z;

	m._41 = t.x; 
	m._42 = t.y;
	m._43 = t.z;

	return m;
}

////////////////////////////////////////////////////////////////////////////////
atcore_float4 gfsdk_getRotation(const atcore_float4x4& sm)
{
	atcore_float4x4 m = sm;

	gfsdk_orthonormalize(m);

	atcore_float4 q;

	float x,y,z,w;

	float tr = m._11 + m._22 + m._33, h;
	if(tr >= 0)
	{
		h = sqrt(tr +1);
		w = float(0.5) * h;
		h = float(0.5) / h;

		x = (m._23 - m._32) * h;
		y = (m._31 - m._13) * h;
		z = (m._12 - m._21) * h;
	}
	else
	{
		float max = m._11;
		int i = 0; 
		if (m._22 > m._11)
		{
			i = 1; 
			max = m._22;
		}
		if (m._33 > max)
			i = 2; 
		switch (i)
		{
		case 0:
			h = sqrt((m._11 - (m._22 + m._33)) + 1);
			x = float(0.5) * h;
			h = float(0.5) / h;

			y = (m._21 + m._12) * h; 
			z = (m._13 + m._31) * h;
			w = (m._23 - m._32) * h;
			break;
		case 1:
			h = sqrt((m._22 - (m._33 + m._11)) + 1);
			y = float(0.5) * h;
			h = float(0.5) / h;

			z = (m._32 + m._23) * h;
			x = (m._21 + m._12) * h;
			w = (m._31 - m._13) * h;
			break;
		case 2:
			h = sqrt((m._33 - (m._11 + m._22)) + 1);
			z = float(0.5) * h;
			h = float(0.5) / h;

			x = (m._13 + m._31) * h;
			y = (m._32 + m._23) * h;
			w = (m._12 - m._21) * h;
			break;
		default: // Make compiler happy
			x = y = z = w = 0;
			break;
		}
	}	

	return gfsdk_makeFloat4(x,y,z,w);
}

////////////////////////////////////////////////////////////////////////////////////////
atcore_float4x4 gfsdk_lerp(atcore_float4x4& start, atcore_float4x4& end, float t)
{
	atcore_float4 sq = gfsdk_getRotation(start);
	atcore_float4 eq = gfsdk_getRotation(end);
	atcore_float3 st = gfsdk_getTranslation(start);
	atcore_float3 et = gfsdk_getTranslation(end);

	atcore_float3 ss = gfsdk_getScale(start);
	atcore_float3 es = gfsdk_getScale(end);
	atcore_float3 s = gfsdk_lerp(ss, es, t);

	atcore_dualquaternion sdq = gfsdk_makeDQ(sq, st);
	atcore_dualquaternion edq = gfsdk_makeDQ(eq, et);

	atcore_dualquaternion dq = gfsdk_lerp(sdq, edq, t);

	atcore_float4 gr = getRotation(dq);
	atcore_float3 gt = getTranslation(dq);

	return gfsdk_makeTransform(gr, gt, s);
}

////////////////////////////////////////////////////////////////////////////////
atcore_float4x4 operator*(const atcore_float4x4& in1, const atcore_float4x4& in2)
{
#define MATRIX_SUM(OUT, IN1, IN2, ROW, COL) OUT._##ROW##COL = IN1._##ROW##1 * IN2._1##COL + IN1._##ROW##2 * IN2._2##COL + IN1._##ROW##3 * IN2._3##COL + IN1._##ROW##4 * IN2._4##COL;

	atcore_float4x4 out;

	MATRIX_SUM(out, in1, in2, 1, 1); 
	MATRIX_SUM(out, in1, in2, 1, 2);
	MATRIX_SUM(out, in1, in2, 1, 3);
	MATRIX_SUM(out, in1, in2, 1, 4);

	MATRIX_SUM(out, in1, in2, 2, 1); 
	MATRIX_SUM(out, in1, in2, 2, 2);
	MATRIX_SUM(out, in1, in2, 2, 3);
	MATRIX_SUM(out, in1, in2, 2, 4);

	MATRIX_SUM(out, in1, in2, 3, 1); 
	MATRIX_SUM(out, in1, in2, 3, 2);
	MATRIX_SUM(out, in1, in2, 3, 3);
	MATRIX_SUM(out, in1, in2, 3, 4);

	MATRIX_SUM(out, in1, in2, 4, 1); 
	MATRIX_SUM(out, in1, in2, 4, 2);
	MATRIX_SUM(out, in1, in2, 4, 3);
	MATRIX_SUM(out, in1, in2, 4, 4);

#undef MATRIX_SUM

	return out;
}

////////////////////////////////////////////////////////////////////////////////
float 
gfsdk_getDeterminant(const atcore_float4x4& m)
{
	const float* matrix = (const float*)&m;
	
	atcore_float3 p0 = gfsdk_makeFloat3(matrix[0*4+0], matrix[0*4+1], matrix[0*4+2]);
	atcore_float3 p1 = gfsdk_makeFloat3(matrix[1*4+0], matrix[1*4+1], matrix[1*4+2]);
	atcore_float3 p2 = gfsdk_makeFloat3(matrix[2*4+0], matrix[2*4+1], matrix[2*4+2]);

	atcore_float3 tempv = gfsdk_cross(p1,p2);

	return gfsdk_dot(p0, tempv);
}

////////////////////////////////////////////////////////////////////////////////
atcore_float4x4
gfsdk_getSubMatrix(int ki,int kj, const atcore_float4x4& m)
{
	atcore_float4x4 out;
	gfsdk_makeIdentity(out);

	float* pDst = (float*)&out;
	const float* matrix = (const float*)&m;

	int row, col;
	int dstCol = 0, dstRow = 0;

	for ( col = 0; col < 4; col++ )
	{
		if ( col == kj )
		{
			continue;
		}
		for ( dstRow = 0, row = 0; row < 4; row++ )
		{
			if ( row == ki )
			{
				continue;
			}
			pDst[dstCol*4+dstRow] = matrix[col*4+row];
			dstRow++;
		}
		dstCol++;
	}

	return out;
}

////////////////////////////////////////////////////////////////////////////////
atcore_float4x4 gfsdk_inverse(const atcore_float4x4& m)
{
	atcore_float4x4 im;

	float* inverse_matrix = (float*)&im;

	float det = gfsdk_getDeterminant(m);
	det = 1.0f / det;
	for (int i = 0; i < 4; i++ )
	{
		for (int j = 0; j < 4; j++ )
		{
			int sign = 1 - ( ( i + j ) % 2 ) * 2;

			atcore_float4x4 subMat = gfsdk_getSubMatrix(i, j, m);
			float subDeterminant = gfsdk_getDeterminant(subMat);
	
			inverse_matrix[i*4+j] = ( subDeterminant * sign ) * det;
		}
	}

	return im;
}

////////////////////////////////////////////////////////////////////////////////
atcore_dualquaternion gfsdk_makeDQ(const atcore_float4x4& m)
{
	atcore_float4 q = gfsdk_getRotation(m);
	atcore_float3 t = gfsdk_getTranslation(m);

	return gfsdk_makeDQ(q, t);
}
