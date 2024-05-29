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
// Copyright (c) 2022-2024 NVIDIA Corporation. All rights reserved.


#include "NvBlastExtExporterObjReader.h"

#pragma warning(push)
#pragma warning(disable:4706)
#pragma warning(disable:4702)
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#pragma warning(pop)


#include <iostream>
#include "NvVec3.h"
#include "NvVec2.h"
#include "NvBlastExtAuthoringMesh.h"

using nvidia::NvVec3;
using nvidia::NvVec2;
using namespace Nv::Blast;

ObjFileReader::ObjFileReader()
{
}

void ObjFileReader::release()
{
    delete this;
}

void ObjFileReader::loadFromFile(const char* filename)
{
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> mats;
    std::string err;
    std::string mtlPath;

    int32_t lastDelimeter = strlen(filename);
    
    while (lastDelimeter > 0 && filename[lastDelimeter] != '/' && filename[lastDelimeter] != '\\')
    {
        lastDelimeter--;
    }
    mtlPath = std::string(filename, filename + lastDelimeter);
    if (mtlPath == "")
    {
        mtlPath = '.';
    }
    mtlPath += '/';
    
    bool ret = tinyobj::LoadObj(shapes, mats, err, filename, mtlPath.c_str());
    
    // can't load?
    if (!ret)
    {
        return;
    }
    if (shapes.size() > 1)
    {
        std::cout << "Can load only one object per mesh" << std::endl;
    }

    if (!mats.empty())
    {
        if (mats.size() == 1 && mats[0].name == "")
        {
            mats[0].name = "Default";
        }
        for (uint32_t i = 0; i < mats.size(); ++i)
        {
                mMaterialNames.push_back(mats[i].name);
        }
    }

    mVertexPositions.clear();
    mVertexNormals.clear();
    mVertexUv.clear();
    mIndices.clear();

    auto& psVec = shapes[0].mesh.positions;
    for (uint32_t i = 0; i < psVec.size() / 3; ++i)
    {
        mVertexPositions.push_back({psVec[i * 3], psVec[i * 3 + 1], psVec[i * 3 + 2]});
    }
    auto& nmVec = shapes[0].mesh.normals;
    for (uint32_t i = 0; i < nmVec.size() / 3; ++i)
    {
        mVertexNormals.push_back({nmVec[i * 3], nmVec[i * 3 + 1], nmVec[i * 3 + 2]});
    }
    auto& txVec = shapes[0].mesh.texcoords;
    for (uint32_t i = 0; i < txVec.size() / 2; ++i)
    {
        mVertexUv.push_back({txVec[i * 2], txVec[i * 2 + 1]});
    }

    mIndices = shapes[0].mesh.indices;
    mPerFaceMatId = shapes[0].mesh.material_ids;
    for (uint32_t i = 0; i < mPerFaceMatId.size(); ++i)
    {
        if (mPerFaceMatId[i] == -1) // TinyOBJ loader sets ID to -1 when .mtl file not found. Set to default 0 material.
        {
            mPerFaceMatId[i] = 0;
        }
    }

}


bool ObjFileReader::isCollisionLoaded() 
{
    return false;
};


uint32_t ObjFileReader::getCollision(uint32_t*& hullsOffset, Nv::Blast::CollisionHull**& hulls)
{
    hullsOffset = nullptr;
    hulls = nullptr;
    return 0;
};

NvcVec3* ObjFileReader::getPositionArray()
{
    return mVertexPositions.data();
};

NvcVec3* ObjFileReader::getNormalsArray()
{
    return mVertexNormals.data();
};

NvcVec2* ObjFileReader::getUvArray()
{
    return mVertexUv.data();
};

uint32_t* ObjFileReader::getIndexArray()
{
    return mIndices.data();
};