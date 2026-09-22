// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2008-2024 NVIDIA Corporation. All rights reserved.


#ifndef RENDER_UTILS_H
#define RENDER_UTILS_H

#include "DirectXTex.h"
#include <DirectXMath.h>
#include "PxMat44.h"
#include "PxVec3.h"
#include "PxVec4.h"

inline DirectX::XMFLOAT4 getRandomPastelColor()
{
    float r = static_cast<float>(rand()) / RAND_MAX * 0.5f + 0.5f;
    float g = static_cast<float>(rand()) / RAND_MAX * 0.5f + 0.5f;
    float b = static_cast<float>(rand()) / RAND_MAX * 0.5f + 0.5f;
    return DirectX::XMFLOAT4(r, g, b, 1.0f);
}

inline physx::PxMat44 XMMATRIXToPxMat44(const DirectX::XMMATRIX& mat)
{
    physx::PxMat44 m;
    memcpy(const_cast<float*>(m.front()), &mat.r[0], 4 * 4 * sizeof(float));
    return m;
}

inline DirectX::XMMATRIX PxMat44ToXMMATRIX(const physx::PxMat44& mat)
{
    return DirectX::XMMATRIX(mat.front());
}

inline physx::PxVec4 XMVECTORToPxVec4(const DirectX::XMVECTOR& vec)
{
    DirectX::XMFLOAT4 f;
    DirectX::XMStoreFloat4(&f, vec);
    return physx::PxVec4(f.x, f.y, f.z, f.w);
}

inline physx::PxVec3 XMFLOAT3ToPxVec3(const DirectX::XMFLOAT3& vec)
{
    return physx::PxVec3(vec.x, vec.y, vec.z);
}

inline physx::PxVec4 XMFLOAT4ToPxVec4(const DirectX::XMFLOAT4& vec)
{
    return physx::PxVec4(vec.x, vec.y, vec.z, vec.w);
}

inline uint32_t XMFLOAT4ToU32Color(const DirectX::XMFLOAT4& color)
{
    uint32_t c = 0;
    c |= (int)(color.w * 255); c <<= 8;
    c |= (int)(color.z * 255); c <<= 8;
    c |= (int)(color.y * 255); c <<= 8;
    c |= (int)(color.x * 255);
    return c;
}

inline DirectX::XMFLOAT4 XMFLOAT4Lerp(const DirectX::XMFLOAT4 v0, const DirectX::XMFLOAT4 v1, float val)
{
    DirectX::XMFLOAT4 v(
        v0.x * (1 - val) + v1.x * val,
        v0.y * (1 - val) + v1.y * val,
        v0.z * (1 - val) + v1.z * val,
        v0.w * (1 - val) + v1.w * val
        );
    return v;
}

static const physx::PxVec3 forwardVector = physx::PxVec3(0, 0, 1);
static const physx::PxVec3 upVector = physx::PxVec3(0, 1, 0);
static const physx::PxVec3 rightVector = physx::PxVec3(1, 0, 0);

PX_INLINE physx::PxQuat quatLookAt(const physx::PxVec3 direction)
{
    float d = direction.dot(forwardVector);
    if (physx::PxAbs(d + 1.0f) < 1e-5f)
    {
        return physx::PxQuat(physx::PxPi, upVector);
    }
    else if (physx::PxAbs(d - 1.0f) < 1e-5f)
    {
        return physx::PxQuat(physx::PxIdentity);
    }
    else
    {
        float angle = physx::PxAcos(d);
        physx::PxVec3 axis = forwardVector.cross(direction).getNormalized();
        return physx::PxQuat(angle, axis);
    }
}

#endif //RENDER_UTILS_H
