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
// Copyright (c) 2013-2015 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//

#include "Camera.h"

//#define USE_D3DX9MATH
#ifdef USE_D3DX9MATH
#include <d3dx9math.h> // TODO - remove d3dx reference (quaternion funcs)

#pragma comment(lib, "d3dx9.lib")
#define MAKEVECTOR3 D3DXVECTOR3
#else
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#define FLOAT float
#define D3DXVECTOR3 atcore_float3
#define D3DXMATRIX atcore_float4x4
#define D3DXQUATERNION atcore_float4

#define MAKEVECTOR3 gfsdk_makeFloat3

#define D3DXToRadian( degree ) ((degree) * 0.01745329251994329547)

D3DXQUATERNION * D3DXQuaternionIdentity(
	__inout  D3DXQUATERNION *pOut
	)
{
	*pOut = gfsdk_makeFloat4(0, 0, 0, 1);
	return pOut;
}

D3DXQUATERNION * D3DXQuaternionNormalize(
	__inout  D3DXQUATERNION *pOut,
	__in     const D3DXQUATERNION *pQ
	)
{
	gfsdk_normalize(*pOut);
	return pOut;
}

D3DXQUATERNION * D3DXQuaternionRotationAxis(
	__inout  D3DXQUATERNION *pOut,
	__in     const D3DXVECTOR3 *pV,
	__in     FLOAT Angle
	)
{
	FLOAT s = sin(Angle * 0.5);

	FLOAT x = pV->x * s;
	FLOAT y = pV->y * s;
	FLOAT z = pV->z * s;
	FLOAT w = cos(Angle * 0.5);

	*pOut = gfsdk_makeFloat4(x, y, z, w);
	return pOut;
}

D3DXQUATERNION * D3DXQuaternionMultiply(
	__inout  D3DXQUATERNION *pOut,
	__in     const D3DXQUATERNION *pQ1,
	__in     const D3DXQUATERNION *pQ2
	)
{
	gfsdk_normalize(*pQ1);
	gfsdk_normalize(*pQ2);

	FLOAT px = pQ2->x;
	FLOAT py = pQ2->y;
	FLOAT pz = pQ2->z;
	FLOAT pw = pQ2->w;

	FLOAT qx = pQ1->x;
	FLOAT qy = pQ1->y;
	FLOAT qz = pQ1->z;
	FLOAT qw = pQ1->w;

	FLOAT x = pw * qx + px * qw + py * qz - pz * qy;
	FLOAT y = pw * qy + py * qw + pz * qx - px * qz;
	FLOAT z = pw * qz + pz * qw + px * qy - py * qx;
	FLOAT w = pw * qw - px * qx - py * qy - pz * qz;

	*pOut = gfsdk_makeFloat4(x, y, z, w);
	gfsdk_normalize(*pOut);
	return pOut;
}

D3DXMATRIX * D3DXMatrixLookAtLH(
	__inout  D3DXMATRIX *pOut,
	__in     const D3DXVECTOR3 *pEye,
	__in     const D3DXVECTOR3 *pAt,
	__in     const D3DXVECTOR3 *pUp
	)
{
	/*
	zaxis = normal(At - Eye)
	xaxis = normal(cross(Up, zaxis))
	yaxis = cross(zaxis, xaxis)

	xaxis.x           yaxis.x           zaxis.x          0
	xaxis.y           yaxis.y           zaxis.y          0
	xaxis.z           yaxis.z           zaxis.z          0
	-dot(xaxis, eye)  -dot(yaxis, eye)  -dot(zaxis, eye)  1
	*/

	D3DXVECTOR3 zaxis = *pAt - *pEye;
	gfsdk_normalize(zaxis);
	D3DXVECTOR3 xaxis = gfsdk_cross(*pUp, zaxis);
	gfsdk_normalize(xaxis);
	D3DXVECTOR3 yaxis = gfsdk_cross(zaxis, xaxis);

	gfsdk_makeIdentity(*pOut);
	pOut->_11 = zaxis.x;
	pOut->_21 = zaxis.y;
	pOut->_31 = zaxis.z;
	pOut->_12 = yaxis.x;
	pOut->_22 = yaxis.y;
	pOut->_32 = yaxis.z;
	pOut->_13 = zaxis.x;
	pOut->_23 = zaxis.y;
	pOut->_33 = zaxis.z;
	pOut->_41 = -gfsdk_dot(xaxis, *pEye);
	pOut->_42 = -gfsdk_dot(yaxis, *pEye);
	pOut->_43 = -gfsdk_dot(zaxis, *pEye);
	return pOut;
}

D3DXMATRIX * D3DXMatrixLookAtRH(
	__inout  D3DXMATRIX *pOut,
	__in     const D3DXVECTOR3 *pEye,
	__in     const D3DXVECTOR3 *pAt,
	__in     const D3DXVECTOR3 *pUp
	)
{
	/*
	zaxis = normal(Eye - At)
	xaxis = normal(cross(Up, zaxis))
	yaxis = cross(zaxis, xaxis)

	xaxis.x           yaxis.x           zaxis.x          0
	xaxis.y           yaxis.y           zaxis.y          0
	xaxis.z           yaxis.z           zaxis.z          0
	-dot(xaxis, eye)  -dot(yaxis, eye)  -dot(zaxis, eye)  1
	*/

	D3DXVECTOR3 zaxis = *pEye - *pAt;
	gfsdk_normalize(zaxis);
	D3DXVECTOR3 xaxis = gfsdk_cross(*pUp, zaxis);
	gfsdk_normalize(xaxis);
	D3DXVECTOR3 yaxis = gfsdk_cross(zaxis, xaxis);

	gfsdk_makeIdentity(*pOut);
	pOut->_11 = xaxis.x;
	pOut->_21 = xaxis.y;
	pOut->_31 = xaxis.z;
	pOut->_12 = yaxis.x;
	pOut->_22 = yaxis.y;
	pOut->_32 = yaxis.z;
	pOut->_13 = zaxis.x;
	pOut->_23 = zaxis.y;
	pOut->_33 = zaxis.z;
	pOut->_41 = -gfsdk_dot(xaxis, *pEye);
	pOut->_42 = -gfsdk_dot(yaxis, *pEye);
	pOut->_43 = gfsdk_dot(zaxis, *pEye);
	return pOut;
}

D3DXMATRIX * D3DXMatrixPerspectiveFovLH(
	__inout  D3DXMATRIX *pOut,
	__in     FLOAT fovy,
	__in     FLOAT Aspect,
	__in     FLOAT zn,
	__in     FLOAT zf
	)
{
	/*
	cot(fovY/2)		0						0					0
	0				cot(fovY/2)/aspect		0					0
	0				0						zf/(zf-zn)			1
	0				0						-zn*zf/(zf-zn)		0
	*/

	memset(pOut, 0, sizeof(D3DXMATRIX));
	FLOAT cosHalfFovy = 1 / tan(fovy * 0.5);
	pOut->_11 = cosHalfFovy;
	pOut->_22 = cosHalfFovy / Aspect;
	pOut->_33 = zf / (zf - zn);
	pOut->_34 = 1;
	pOut->_43 = -zn*zf / (zf - zn);
	return pOut;
}

D3DXMATRIX * D3DXMatrixPerspectiveFovRH(
	__inout  D3DXMATRIX *pOut,
	__in     FLOAT fovy,
	__in     FLOAT Aspect,
	__in     FLOAT zn,
	__in     FLOAT zf
	)
{
	/*
	cot(fovY/2)/aspect		0				0					0
	0						cot(fovY/2)		0					0
	0						0				zf/(zn-zf)			-1
	0						0				zn*zf/(zn-zf)		0
	*/

	memset(pOut, 0, sizeof(D3DXMATRIX));
	FLOAT cosHalfFovy = 1 / tan(fovy * 0.5);
	pOut->_11 = cosHalfFovy / Aspect;
	pOut->_22 = cosHalfFovy;
	pOut->_33 = zf / (zn - zf);
	pOut->_34 = -1;
	pOut->_43 = zn*zf / (zn - zf);
	return pOut;
}

D3DXMATRIX * D3DXMatrixOrthoLH(
	__inout  D3DXMATRIX *pOut,
	__in     FLOAT w,
	__in     FLOAT h,
	__in     FLOAT zn,
	__in     FLOAT zf
	)
{
	/*
	2/w  0    0           0
	0    2/h  0           0
	0    0    1/(zf-zn)   0
	0    0    zn/(zn-zf)  1
	*/

	gfsdk_makeIdentity(*pOut);
	pOut->_11 = 2 / w;
	pOut->_22 = 2 / h;
	pOut->_33 = 1 / (zf - zn);
	pOut->_43 = zn / (zn - zf);
	return pOut;
}

D3DXMATRIX * D3DXMatrixOrthoRH(
	__inout  D3DXMATRIX *pOut,
	__in     FLOAT w,
	__in     FLOAT h,
	__in     FLOAT zn,
	__in     FLOAT zf
	)
{
	/*
	2/w  0    0           0
	0    2/h  0           0
	0    0    1/(zn-zf)   0
	0    0    zn/(zn-zf)  1
	*/

	gfsdk_makeIdentity(*pOut);
	pOut->_11 = 2 / w;
	pOut->_22 = 2 / h;
	pOut->_33 = 1 / (zn - zf);
	pOut->_43 = zn / (zn - zf);
	return pOut;
}

D3DXQUATERNION * D3DXQuaternionRotationMatrix(
	__inout  D3DXQUATERNION *pOut,
	__in     const D3DXMATRIX *pM
	)
{
	FLOAT fourXSquaredMinus1 = pM->_11 - pM->_22 - pM->_33;
	FLOAT fourYSquaredMinus1 = pM->_22 - pM->_11 - pM->_33;
	FLOAT fourZSquaredMinus1 = pM->_33 - pM->_11 - pM->_22;
	FLOAT fourWSquaredMinus1 = pM->_11 + pM->_22 + pM->_33;

	int biggestIndex = 0;
	FLOAT fourBiggestSquaredMinus1 = fourWSquaredMinus1;
	if (fourXSquaredMinus1 > fourBiggestSquaredMinus1)
	{
		fourBiggestSquaredMinus1 = fourXSquaredMinus1;
		biggestIndex = 1;
	}
	if (fourYSquaredMinus1 > fourBiggestSquaredMinus1)
	{
		fourBiggestSquaredMinus1 = fourYSquaredMinus1;
		biggestIndex = 2;
	}
	if (fourZSquaredMinus1 > fourBiggestSquaredMinus1)
	{
		fourBiggestSquaredMinus1 = fourZSquaredMinus1;
		biggestIndex = 3;
	}

	FLOAT biggestVal = sqrt(fourBiggestSquaredMinus1 + 1) * 0.5;
	FLOAT mult = 0.25 / biggestVal;

	D3DXQuaternionIdentity(pOut);
	switch (biggestIndex)
	{
	case 0:
		pOut->w = biggestVal;
		pOut->x = (pM->_23 - pM->_32) * mult;
		pOut->y = (pM->_31 - pM->_13) * mult;
		pOut->z = (pM->_12 - pM->_21) * mult;
		break;
	case 1:
		pOut->w = (pM->_23 - pM->_32) * mult;
		pOut->x = biggestVal;
		pOut->y = (pM->_12 + pM->_21) * mult;
		pOut->z = (pM->_31 + pM->_13) * mult;
		break;
	case 2:
		pOut->w = (pM->_31 - pM->_13) * mult;
		pOut->x = (pM->_12 + pM->_21) * mult;
		pOut->y = biggestVal;
		pOut->z = (pM->_23 + pM->_32) * mult;
		break;
	case 3:
		pOut->w = (pM->_12 - pM->_21) * mult;
		pOut->x = (pM->_31 + pM->_13) * mult;
		pOut->y = (pM->_23 + pM->_32) * mult;
		pOut->z = biggestVal;
		break;
	default:
		break;
	}
	return pOut;
}

D3DXMATRIX * D3DXMatrixRotationQuaternion(
	__inout  D3DXMATRIX *pOut,
	__in     const D3DXQUATERNION *pQ
	)
{
	gfsdk_makeIdentity(*pOut);
	FLOAT qxx = (pQ->x * pQ->x);
	FLOAT qyy = (pQ->y * pQ->y);
	FLOAT qzz = (pQ->z * pQ->z);
	FLOAT qxz = (pQ->x * pQ->z);
	FLOAT qxy = (pQ->x * pQ->y);
	FLOAT qyz = (pQ->y * pQ->z);
	FLOAT qwx = (pQ->w * pQ->x);
	FLOAT qwy = (pQ->w * pQ->y);
	FLOAT qwz = (pQ->w * pQ->z);
	pOut->_11 = 1 - 2 * (qyy + qzz);
	pOut->_12 = 2 * (qxy + qwz);
	pOut->_13 = 2 * (qxz - qwy);
	pOut->_21 = 2 * (qxy - qwz);
	pOut->_22 = 1 - 2 * (qxx + qzz);
	pOut->_23 = 2 * (qyz + qwx);
	pOut->_31 = 2 * (qxz + qwy);
	pOut->_32 = 2 * (qyz - qwx);
	pOut->_33 = 1 - 2 * (qxx + qyy);
	return pOut;
}
#endif // USE_D3DX9MATH

Camera::Camera(bool zup, bool lhs)
	:
	_zup(zup),
	_lhs(lhs),
	_isPerspective(true)
{
	
    D3DXQuaternionIdentity((D3DXQUATERNION*)&_orientation);

	gfsdk_makeIdentity(_viewMatrix);
	gfsdk_makeIdentity(_projectionMatrix);

	_fov = (75.0f / 360.0f) * 3.141592653589793;
}

Camera::~Camera()
{
}

void 
Camera::Init(bool zup, bool lhs)
{
	_lhs = lhs;
	_zup = zup;
}

atcore_float3 Camera::GetUp() const
{
	atcore_float3 up;

	if (IsYUp())
		up = gfsdk_makeFloat3(0.0f, 1.0f, 0.0f);
	else
		up = gfsdk_makeFloat3(0.0f, 0.0f, 1.0f);

	return up;
}

void Camera::SetDefaults()
{
	atcore_float3 eye;
	atcore_float3 at = gfsdk_makeFloat3(0.0f, 0.0f, 0.0f);

	if (IsYUp())
	{
		if (_lhs)
			eye = gfsdk_makeFloat3(0.0f, 60.0f, -120.0f);
		else
			eye = gfsdk_makeFloat3(0.0f, 60.0f, 120.0f);
	}
	else // zup
	{
		if (_lhs)
			eye = gfsdk_makeFloat3(0.0f, 120.0f, 60.0f);
		else
			eye = gfsdk_makeFloat3(0.0f, -120.0f, 60.0f);
	}

	atcore_float3 up = GetUp();

	LookAt(eye, at, up);
}

// Build a look at matrix, and calculate the major axis and rotation component.
void Camera::LookAt(const atcore_float3& eye, const atcore_float3& at, const atcore_float3& up)
{
	if (_lhs)
		D3DXMatrixLookAtLH((D3DXMATRIX*)&_viewMatrix, (D3DXVECTOR3*)&eye, (D3DXVECTOR3*)&at, (D3DXVECTOR3*)&up);
	else
		D3DXMatrixLookAtRH((D3DXMATRIX*)&_viewMatrix, (D3DXVECTOR3*)&eye, (D3DXVECTOR3*)&at, (D3DXVECTOR3*)&up);

    _eye = eye;
    _at = at;

    atcore_float3 dir = at - eye;
	_lookDistance = gfsdk_length(dir);
 
    D3DXQuaternionRotationMatrix((D3DXQUATERNION*)&_orientation, (D3DXMATRIX*)&_viewMatrix);
	BuildViewMatrix();
}

void Camera::SetEye(const atcore_float3& eye)
{
	_eye = eye;
	_lookDistance = gfsdk_distance(_eye, _at);
}

void Camera::SetAt(const atcore_float3& at)
{
	_at = at;
	_lookDistance = gfsdk_distance(_eye, _at);
}

void Camera::SetViewMatrix(const atcore_float3& xAxis, const atcore_float3& yAxis, const atcore_float3& zAxis)
{
	_viewMatrix._11 = xAxis.x; _viewMatrix._21 = xAxis.y; _viewMatrix._31 = xAxis.z;
	_viewMatrix._12 = yAxis.x; _viewMatrix._22 = yAxis.y; _viewMatrix._32 = yAxis.z;
	_viewMatrix._13 = zAxis.x; _viewMatrix._23 = zAxis.y; _viewMatrix._33 = zAxis.z;

	D3DXQuaternionRotationMatrix((D3DXQUATERNION*)&_orientation, (D3DXMATRIX*)&_viewMatrix);
	D3DXQuaternionNormalize((D3DXQUATERNION*)&_orientation, (D3DXQUATERNION*)&_orientation);

	atcore_float3 _viewDirection = _lhs ? -1.0f * zAxis : zAxis;

	_eye = _at + _viewDirection * _lookDistance;

	_viewMatrix._41 = - gfsdk_dot(xAxis,_eye);
	_viewMatrix._42 = - gfsdk_dot(yAxis,_eye);
	_viewMatrix._43 = - gfsdk_dot(zAxis,_eye);
}

// Build a perspective matrix
void Camera::Perspective()
{
	_isPerspective = true;

	if (_lhs)
		D3DXMatrixPerspectiveFovLH( (D3DXMATRIX*)&_projectionMatrix, _fov, _aspectRatio, _znear, _zfar);
	else
		D3DXMatrixPerspectiveFovRH( (D3DXMATRIX*)&_projectionMatrix, _fov, _aspectRatio, _znear, _zfar);

}

void Camera::Ortho(float width, float height, float znear, float zfar)
{
	if (_lhs)
		D3DXMatrixOrthoLH( (D3DXMATRIX*)&_projectionMatrix, width, height, znear, zfar);
	else
		D3DXMatrixOrthoRH( (D3DXMATRIX*)&_projectionMatrix, width, height, znear, zfar);

	_znear = znear;
	_zfar = zfar;
	_width = width;
	_height = height;
	_isPerspective = false;
}

// Dolly towards the viewpoint
void Camera::Dolly(float zoom)
{
	if (_isPerspective)
	{
		atcore_float3 offset = _eye - _at;

		_lookDistance = gfsdk_length(offset); 

		gfsdk_normalize(offset);

		float zoomFactor = zoom * _lookDistance;

		_lookDistance += zoomFactor;

		_eye = _lookDistance * offset + _at;
	    
		BuildViewMatrix();
	}
	else
	{
		_width += zoom * 4.0f;
		_height += zoom * 4.0f;

		D3DXMatrixOrthoRH( (D3DXMATRIX*)&_projectionMatrix, _width, _height, _znear, _zfar);

	}
}

// Orbit around the viewpoint
void Camera::Orbit(const atcore_float2& delta)
{

    float heading = D3DXToRadian(delta.x);
    float pitch = D3DXToRadian(delta.y);

	if (_lhs)
	{
		heading *= -1.0f;
		pitch *= -1.0f;
	}
    
    D3DXQUATERNION rot;

	D3DXVECTOR3 yAxis = MAKEVECTOR3(0.0f, 0.0f, 1.0f);
	if (IsYUp()) // change up axis if Y is the up axis (for maya)
	{
		yAxis = MAKEVECTOR3( 0.0f, 1.0f, 0.0f );
	}

	D3DXVECTOR3 xAxis = MAKEVECTOR3(1.0f, 0.0f, 0.0f);
    if (heading != 0.0f)
    {
        D3DXQuaternionRotationAxis(&rot, &yAxis, heading);
        D3DXQuaternionMultiply((D3DXQUATERNION*)&_orientation, &rot, (D3DXQUATERNION*)&_orientation);
    }

    if (pitch != 0.0f)
    {
        D3DXQuaternionRotationAxis(&rot, &xAxis, pitch);
        D3DXQuaternionMultiply((D3DXQUATERNION*)&_orientation, (D3DXQUATERNION*)&_orientation, &rot);
    }
    BuildViewMatrix();

}

// Orbit around the model's center for HAIR-188
void Camera::OrbitLight(const atcore_float2& delta)
{

	float heading = D3DXToRadian(delta.x);
	float pitch = D3DXToRadian(delta.y);

	if (_lhs)
	{
		heading *= -1.0f;
		pitch *= -1.0f;
	}

	D3DXQUATERNION rot;

	D3DXVECTOR3 yAxis = MAKEVECTOR3(0.0f, 0.0f, 1.0f);
	if (IsYUp()) // change up axis if Y is the up axis (for maya)
	{
		yAxis = MAKEVECTOR3( 0.0f, 1.0f, 0.0f );
	}

	D3DXVECTOR3 xAxis = MAKEVECTOR3(1.0f, 0.0f, 0.0f);
	if (heading != 0.0f)
	{
		D3DXQuaternionRotationAxis(&rot, &yAxis, heading);
		D3DXQuaternionMultiply((D3DXQUATERNION*)&_orientation, &rot, (D3DXQUATERNION*)&_orientation);
	}

	if (pitch != 0.0f)
	{
		D3DXQuaternionRotationAxis(&rot, &xAxis, pitch);
		D3DXQuaternionMultiply((D3DXQUATERNION*)&_orientation, &rot, (D3DXQUATERNION*)&_orientation);
	}
	BuildViewMatrix();

}

void Camera::Pan(const atcore_float2& delta)
{
	//Scale the movement by the current view
	//float depth = depthRange.y - depthRange.x;

	atcore_float3 viewDir = GetEye() - GetAt();
	float depth = gfsdk_length(viewDir);
	if (depth <= 0.0f)
		depth = 1.0f;

	atcore_float3 xAxis = GetXAxis();
	atcore_float3 yAxis = GetYAxis();

	if (_isPerspective)
	{
		float fov2 = ((float)-tan(0.5 * _fov) * (depth));
		atcore_float2 newDelta = fov2 * delta;

		_at = _at + (-1.0f * yAxis * newDelta.y) + (xAxis * newDelta.x);
	}
	else
	{
		atcore_float2 newDelta = depth * delta;
		_at = _at + (yAxis * newDelta.y) + (-1.0f * xAxis * newDelta.x);
	}


	BuildViewMatrix();
}

atcore_float3 Camera::GetXAxis() const
{
	return gfsdk_makeFloat3(_viewMatrix._11, _viewMatrix._21, _viewMatrix._31);

}

atcore_float3 Camera::GetYAxis() const
{
	return gfsdk_makeFloat3(_viewMatrix._12, _viewMatrix._22, _viewMatrix._32);
}

atcore_float3 Camera::GetZAxis() const
{
	return gfsdk_makeFloat3(_viewMatrix._13, _viewMatrix._23, _viewMatrix._33);
}

atcore_float3 Camera::GetViewDirection() const
{
	atcore_float3 zAxis = GetZAxis();
	return _lhs ? -1.0f * zAxis : zAxis;
}

// Reconstruct the view matrix from the current orientation and eye/look direction.
void Camera::BuildViewMatrix()
{
	// Reconstruct the view matrix.
	D3DXQuaternionNormalize((D3DXQUATERNION*)&_orientation, (D3DXQUATERNION*)&_orientation);
	D3DXMatrixRotationQuaternion((D3DXMATRIX*)&_viewMatrix, (D3DXQUATERNION*)&_orientation);

	atcore_float3 xAxis = GetXAxis();
	atcore_float3 yAxis = GetYAxis();
	atcore_float3 zAxis = GetZAxis();

	atcore_float3 viewDirection = GetViewDirection();

	_eye = _at + viewDirection * _lookDistance;

	_viewMatrix._41 = - gfsdk_dot(xAxis,_eye);
	_viewMatrix._42 = - gfsdk_dot(yAxis,_eye);
	_viewMatrix._43 = - gfsdk_dot(zAxis,_eye);
}

// Set Z Up or Y Up.
void Camera::ResetUpDir(bool zup)
{
	if (zup == _zup)
		return;

	_zup = zup;

	atcore_float3 eye = GetEye();
	atcore_float3 at = GetAt();
	atcore_float3 up;

	if (IsYUp())
	{
		// Swap eye z and y
		float temp = eye.y;
		eye.y = eye.z;
		eye.z = -temp;
		// Swap at z and y
		temp = at.y;
		at.y = at.z;
		at.z = -temp;

		// Set up dir
		up = gfsdk_makeFloat3(0.0f, 1.0f, 0.0f);
	}
	else
	{
		// Swap eye z and y
		float temp = eye.y;
		eye.y = -eye.z;
		eye.z = temp;
		// Swap at z and y
		temp = at.y;
		at.y = -at.z;
		at.z = temp;

		// Set up dir
		up = gfsdk_makeFloat3(0.0f, 0.0f, 1.0f);
	}
	LookAt(eye, at, up);
	BuildViewMatrix();
}

// Set Z Up or Y Up.
void Camera::ResetLhs(bool lhs)
{
	if (lhs == _lhs)
		return;

	_lhs = lhs;

	atcore_float3 eye = GetEye();
	atcore_float3 at = GetAt();

	if (_zup)
	{
		eye.y *= -1;
		at.y *= -1;
	}
	else
	{
		eye.z *= -1;
		at.z *= -1;
	}

	if (_isPerspective)
		Perspective();
	else
		Ortho(_width, _height, _znear, _zfar);

	atcore_float3 up = GetUp();

	LookAt(eye, at, up);
	BuildViewMatrix();
}

void Camera::FitBounds(const atcore_float3& center, const atcore_float3& extents)
{
	SetAt(center);
	// set center first to get eye
	BuildViewMatrix();
	if (_isPerspective)
	{
		float size = extents.x;
		size = max(size, extents.y);
		size = max(size, extents.z);
		atcore_float3 eye = GetEye();

		atcore_float3 dir = eye - center;
		gfsdk_normalize(dir);
		float distance = size / tanf(_fov/2.f);

		eye = center + distance * dir;

		SetEye(eye);
		BuildViewMatrix();
	}
}

#ifndef NV_ARTISTTOOLS
#include "ProjectParams.h"
#endif // NV_ARTISTTOOLS

bool Camera::LoadParameters(void* ptr)
{
#ifndef NV_ARTISTTOOLS
	nvidia::parameterized::HairProjectParametersNS::Camera_Type* param =
		static_cast<nvidia::parameterized::HairProjectParametersNS::Camera_Type*>(ptr);

	_zup = param->flags == 1;
	_fov = param->fov;
	_aspectRatio = param->aspectRatio;
	_znear = param->znear;
	_zfar = param->zfar;
	_isPerspective = param->isPerspective;
	memcpy(&_eye, &param->eye, sizeof(_eye));
	memcpy(&_at, &param->at, sizeof(_at));
	_lookDistance = param->lookDistance;
	memcpy(&_orientation, &param->orientation, sizeof(_orientation));
	memcpy(&_viewMatrix, &param->viewMatrix, sizeof(_viewMatrix));
	memcpy(&_projectionMatrix, &param->projectionMatrix, sizeof(_projectionMatrix));
#else
	CoreLib::Inst()->Camera_LoadParameters(ptr, this);
#endif // NV_ARTISTTOOLS
	return true;
}

bool Camera::SaveParameters(void *ptr)
{
#ifndef NV_ARTISTTOOLS
	nvidia::parameterized::HairProjectParametersNS::Camera_Type* outParam =
		static_cast<nvidia::parameterized::HairProjectParametersNS::Camera_Type*>(ptr);

	outParam->flags = (_zup ? 1 : 2);
	outParam->fov = _fov;
	outParam->aspectRatio = _aspectRatio;
	outParam->znear = _znear;
	outParam->zfar = _zfar;
	outParam->width = 0;
	outParam->height = 0;
	outParam->isPerspective = _isPerspective;
	memcpy(&outParam->eye, &_eye, sizeof(outParam->eye));
	memcpy(&outParam->at, &_at, sizeof(outParam->at));
	outParam->lookDistance = _lookDistance;
	memcpy(&outParam->orientation, &_orientation, sizeof(outParam->orientation));
	memcpy(&outParam->viewMatrix, &_viewMatrix, sizeof(outParam->viewMatrix));
	memcpy(&outParam->projectionMatrix, &_projectionMatrix, sizeof(outParam->projectionMatrix));
#else
	CoreLib::Inst()->Camera_SaveParameters(ptr, this);
#endif // NV_ARTISTTOOLS
	return true;
}

void Camera::getScreenCoord(float x, float y, float z, int &sx, int &sy)
{
	atcore_float4x4 view		 = (atcore_float4x4&)GetViewMatrix();
	atcore_float4x4 projection = (atcore_float4x4&)GetProjectionMatrix();

	atcore_float4x4 viewProjection = view * projection;

	atcore_float4 vp = gfsdk_transform(viewProjection, gfsdk_makeFloat4(x, y, z, 1.0f));

	float nx = vp.x / vp.w;
	float ny = vp.y / vp.w;

	float w = GetWidth();
	float h = GetHeight();

	sx = w * (0.5f + 0.5f * nx);
	sy = h * (0.5f - 0.5f * ny);

}

