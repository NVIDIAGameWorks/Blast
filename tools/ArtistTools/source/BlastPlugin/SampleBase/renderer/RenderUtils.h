/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef RENDER_UTILS_H
#define RENDER_UTILS_H

#include "DirectXTex.h"
#include <DirectXMath.h>
#include "PxMat44.h"
#include "PxVec3.h"
#include "PxVec4.h"

static DirectX::XMFLOAT4 getRandomPastelColor()
{
	float r = ((double)rand() / (RAND_MAX)) * 0.5f + 0.5f;
	float g = ((double)rand() / (RAND_MAX)) * 0.5f + 0.5f;
	float b = ((double)rand() / (RAND_MAX)) * 0.5f + 0.5f;
	return DirectX::XMFLOAT4(r, g, b, 1.0f);
}

static physx::PxMat44 XMMATRIXToPxMat44(const DirectX::XMMATRIX& mat)
{
	physx::PxMat44 m;
	memcpy(const_cast<float*>(m.front()), &mat.r[0], 4 * 4 * sizeof(float));
	return m;
}

static DirectX::XMMATRIX PxMat44ToXMMATRIX(const physx::PxMat44& mat)
{
	return DirectX::XMMATRIX(mat.front());
}

static physx::PxVec4 XMVECTORToPxVec4(const DirectX::XMVECTOR& vec)
{
	DirectX::XMFLOAT4 f;
	DirectX::XMStoreFloat4(&f, vec);
	return physx::PxVec4(f.x, f.y, f.z, f.w);
}

static physx::PxVec3 XMFLOAT3ToPxVec3(const DirectX::XMFLOAT3& vec)
{
	return physx::PxVec3(vec.x, vec.y, vec.z);
}

static physx::PxVec4 XMFLOAT4ToPxVec4(const DirectX::XMFLOAT4& vec)
{
	return physx::PxVec4(vec.x, vec.y, vec.z, vec.w);
}

static uint32_t XMFLOAT4ToU32Color(const DirectX::XMFLOAT4& color)
{
	uint32_t c = 0;
	c |= (int)(color.w * 255); c <<= 8;
	c |= (int)(color.z * 255); c <<= 8;
	c |= (int)(color.y * 255); c <<= 8;
	c |= (int)(color.x * 255);
	return c;
}

static DirectX::XMFLOAT4 XMFLOAT4Lerp(const DirectX::XMFLOAT4 v0, const DirectX::XMFLOAT4 v1, float val)
{
	DirectX::XMFLOAT4 v(
		v0.x * (1 - val) + v1.x * val,
		v0.y * (1 - val) + v1.y * val,
		v0.z * (1 - val) + v1.z * val,
		v0.w * (1 - val) + v1.w * val
		);
	return v;
}

#endif //RENDER_UTILS_H