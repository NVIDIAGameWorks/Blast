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
// Copyright (c) 2008-2023 NVIDIA Corporation. All rights reserved.


#include "PrimitiveRenderMesh.h"
#include "Renderer.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                          Base Mesh internal class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PrimitiveRenderMesh::PrimitiveRenderMesh(const float v[], UINT numVertices)
{
    std::vector<D3D11_INPUT_ELEMENT_DESC> layout;
    layout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 });
    layout.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 });

    initialize(v, numVertices, sizeof(float) * 6, layout, nullptr, 0);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                  Box Mesh
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const float boxVertices[] =
{
        -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f,
         1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 
         1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 
         1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 
        -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 
        -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 

        -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f,
        -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f,

        -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f,
        -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f,
        -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f,
        -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f,
        -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f,
        -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f,

         1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,
         1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f,
         1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,
         1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,
         1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,
         1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,

        -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f,
         1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f,
         1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f,
         1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f,
        -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f,
        -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f,

        -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f,
         1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f,
         1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,
         1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f
};

BoxRenderMesh::BoxRenderMesh() : PrimitiveRenderMesh(boxVertices, sizeof(boxVertices) / (6 * sizeof(boxVertices[0]))) {}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                  Plane Mesh
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const float planeSize = 1.0f; // we use scaling instead
const float planeTilesCount = 1000.0f;

const float planeVertices[] =
{
    0,  planeSize,  planeSize, 1.0f, 0.0f, 0.0f,  planeTilesCount,  planeTilesCount,
    0,  planeSize, -planeSize, 1.0f, 0.0f, 0.0f,  planeTilesCount, -planeTilesCount,
    0, -planeSize, -planeSize, 1.0f, 0.0f, 0.0f, -planeTilesCount, -planeTilesCount,
    0, -planeSize, -planeSize, 1.0f, 0.0f, 0.0f, -planeTilesCount, -planeTilesCount,
    0, -planeSize,  planeSize, 1.0f, 0.0f, 0.0f, -planeTilesCount,  planeTilesCount,
    0,  planeSize,  planeSize, 1.0f, 0.0f, 0.0f,  planeTilesCount,  planeTilesCount
};

PlaneRenderMesh::PlaneRenderMesh()
{
    std::vector<D3D11_INPUT_ELEMENT_DESC> layout;
    layout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 });
    layout.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 });
    layout.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 });

    initialize(planeVertices, sizeof(planeVertices) / (8 * sizeof(planeVertices[0])), sizeof(float) * 8, layout, nullptr, 0);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                  Sphere Mesh
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


const uint32_t g_numSlices = 8;  // along lines of longitude
const uint32_t g_numStacks = 16; // along lines of latitude

const uint32_t g_numSphereVertices = (g_numSlices * 2 + 1)*(g_numStacks + 1);
const uint32_t g_numSphereIndices = g_numSlices * 2 * g_numStacks * 6;

const uint32_t g_numConeVertices = (g_numSlices * 2 + 1) * 2;
const uint32_t g_numConeIndices = g_numSlices * 2 * 6;

PxVec3 g_spherePositions[g_numSphereVertices];
uint16_t g_sphereIndices[g_numSphereIndices];

void generateSphereMesh(uint16_t slices, uint16_t stacks, PxVec3* positions, uint16_t* indices)
{
    const PxF32 thetaStep = PxPi / stacks;
    const PxF32 phiStep = PxTwoPi / (slices * 2);

    PxF32 theta = 0.0f;

    // generate vertices
    for (uint16_t y = 0; y <= stacks; ++y)
    {
        PxF32 phi = 0.0f;

        PxF32 cosTheta = PxCos(theta);
        PxF32 sinTheta = PxSin(theta);

        for (uint16_t x = 0; x <= slices * 2; ++x)
        {
            PxF32 cosPhi = PxCos(phi);
            PxF32 sinPhi = PxSin(phi);

            PxVec3 p(cosPhi*sinTheta, cosTheta, sinPhi*sinTheta);

            // write vertex
            *(positions++) = p;

            phi += phiStep;
        }

        theta += thetaStep;
    }

    const uint16_t numRingQuads = 2 * slices;
    const uint16_t numRingVerts = 2 * slices + 1;

    // add faces
    for (uint16_t y = 0; y < stacks; ++y)
    {
        for (uint16_t i = 0; i < numRingQuads; ++i)
        {
            // add a quad
            *(indices++) = (y + 0)*numRingVerts + i;
            *(indices++) = (y + 1)*numRingVerts + i;
            *(indices++) = (y + 1)*numRingVerts + i + 1;

            *(indices++) = (y + 1)*numRingVerts + i + 1;
            *(indices++) = (y + 0)*numRingVerts + i + 1;
            *(indices++) = (y + 0)*numRingVerts + i;
        }
    }
}


struct SphereVertex
{
    PxVec3 position;
    PxVec3 normal;
};

SphereRenderMesh::SphereRenderMesh()
{
    generateSphereMesh(g_numSlices, g_numStacks, g_spherePositions, g_sphereIndices);

    std::vector<SphereVertex> vertices;
    for (uint32_t i = 0; i < g_numSphereVertices; i++)
    {
        vertices.push_back({ g_spherePositions[i], g_spherePositions[i] });
    }

    std::vector<D3D11_INPUT_ELEMENT_DESC> layout;
    layout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 });
    layout.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 });

    initialize(vertices.data(), (uint32_t)vertices.size(), sizeof(SphereVertex), layout, g_sphereIndices, g_numSphereIndices);
}


SphereRenderMesh::~SphereRenderMesh()
{
}
