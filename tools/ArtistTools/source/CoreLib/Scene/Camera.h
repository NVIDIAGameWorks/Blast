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

#pragma once

#include "MathUtil.h"

class CORELIB_EXPORT Camera
{
public:

	// YUp default
	Camera(bool zUp = false, bool lhs = false);
	~Camera();

	void Init(bool zup, bool lhs);
	void LookAt(const atcore_float3& eye, const atcore_float3& target, const atcore_float3& up);
	void Perspective();
	void Ortho(float width, float height, float znear, float zfar);
	void Dolly(float zoom);
	void Orbit(const atcore_float2& delta);
	void OrbitLight(const atcore_float2& delta);   // special for HAIR-188
	void Pan(const atcore_float2& delta);

	void getScreenCoord(float x, float y, float z, int &sx, int &sy);

	float GetZNear() { return _znear; }
	float GetZFar() { return _zfar; }

	const atcore_float3& GetAt() const { return _at; }
	const atcore_float3& GetEye() const { return _eye; }

	float GetWidth() const { return _width; }
	float GetHeight() const { return _height; }

	void  SetFOV(float fov) { _fov = fov; }
	void  SetAspetRatio(float aspect) { _aspectRatio = aspect; }
	void  SetZNear(float znear) { _znear = znear; }
	void  SetZFar(float zfar) { _zfar = zfar; }

	void SetSize(int w, int h) { _width = w; _height = h; }

	atcore_float3 GetXAxis() const;
	atcore_float3 GetYAxis() const;
	atcore_float3 GetZAxis() const;

	atcore_float3 GetViewDirection() const;

	void SetViewMatrix(const atcore_float3& xAxis, const atcore_float3& yAxis, const atcore_float3& zAxis);

	float GetLookDistance() const { return _lookDistance; }
	atcore_float3 GetUp() const;

	void SetEye(const atcore_float3& eye);
	void SetAt(const atcore_float3& at);

	atcore_float2 GetZRange() const { return gfsdk_makeFloat2(_znear, _zfar); }

	const atcore_float4x4& GetProjectionMatrix() const { return _projectionMatrix; }
	const atcore_float4x4& GetViewMatrix() const { return _viewMatrix; }

	// Change Up Dir and reset internal eye and at.
	void ResetUpDir(bool zup);
	void ResetLhs(bool lhs);

	bool IsYUp() const { return _zup == false; }
	bool UseLHS() const { return _lhs; }

	bool LoadParameters(void* param);
	bool SaveParameters(void* outParam);
    
    void BuildViewMatrix();

	void SetDefaults();

	void FitBounds(const atcore_float3& center, const atcore_float3& extents);


//private:
	// coordinate axis
	bool	_zup;
	bool	_lhs;

	// Perspective
	bool	_isPerspective;
	float	_fov;
    float	_aspectRatio;

	float	_znear;
    float	_zfar;
	float	_width;
	float	_height;

    float			_lookDistance;

	atcore_float3	_eye;
    atcore_float3	_at;

	atcore_float4		_orientation;
    atcore_float4x4		_viewMatrix;
    atcore_float4x4		_projectionMatrix;
};

