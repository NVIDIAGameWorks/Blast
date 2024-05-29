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

//! @file
//!
//! @brief Defines an API to export blast meshes and data in common formats

#ifndef NVBLASTEXTEXPORTER_H
#define NVBLASTEXTEXPORTER_H

#include "NvBlastTypes.h"
#include "NvCTypes.h"

struct NvBlastAsset;

namespace Nv
{
namespace Blast
{
struct AuthoringResult;
struct CollisionHull;

struct Material
{
    const char* name;
    const char* diffuse_tex;
};

struct ExporterMeshData
{
    NvBlastAsset* asset; //Blast asset

    uint32_t positionsCount; //Number of positions

    uint32_t normalsCount; //Number of normals

    uint32_t uvsCount; //Number of textures uv

    NvcVec3* positions; //Array of positions

    NvcVec3* normals;  // Array of normals

    NvcVec2* uvs;  // Array of textures uv

    uint32_t meshCount; //Number of meshes (chunks)

    uint32_t submeshCount; //Number of submeshes

    Material* submeshMats; 


    /**
        Indices offsets for posIndex, normIndex and texIndex
        First position index: posIndex[submeshOffsets[meshId * submeshCount + submeshId]]
        Total number of indices: submeshOffsets[meshCount * submeshCount]
    */
    uint32_t* submeshOffsets;

    uint32_t* posIndex; //Array of position indices

    uint32_t* normIndex; //Array of normals indices

    uint32_t* texIndex; //Array of texture indices


    /**
        Hull offsets. Contains meshCount + 1 element.
        First hull for i-th mesh: hulls[hullsOffsets[i]]
        hullsOffsets[meshCount+1] is total number of hulls
    */
    uint32_t* hullsOffsets;

    CollisionHull** hulls; //Array of pointers to hull for all meshes
};

/**
    An interface for Blast mesh file reader
*/
class IMeshFileReader
{
public:
    
    /**
        Delete this object
    */
    virtual void            release() = 0;

    /*
        Load from the specified file path
    */
    virtual void            loadFromFile(const char* filename) = 0;

    /**
        Number of loaded vertices
    */
    virtual uint32_t        getVerticesCount() const = 0;

    /**
        Number of loaded indices
    */
    virtual uint32_t        getIndicesCount() const = 0;

    /**
        Get loaded vertex positions
    */
    virtual NvcVec3*    getPositionArray() = 0;

    /**
        Get loaded vertex normals
    */
    virtual NvcVec3*    getNormalsArray() = 0;

    /**
        Get loaded vertex uv-coordinates
    */
    virtual NvcVec2*    getUvArray() = 0;

    /**
        Get loaded per triangle material ids.
    */
    virtual int32_t*        getMaterialIds() = 0;

    /**
            Get loaded per triangle smoothing groups.
    */
    virtual int32_t*        getSmoothingGroups() = 0;

    /**
        Get material name.
    */
    virtual const char*     getMaterialName(int32_t id) = 0;

    /**
        Get material count.
    */
    virtual int32_t         getMaterialCount() = 0;



    /**
        Get loaded triangle indices
    */
    virtual uint32_t*       getIndexArray() = 0;


    /**
        Check whether file contained an collision geometry
    */
    virtual bool            isCollisionLoaded() = 0;

    /**
        Retrieve collision geometry if it exist
        \note User should call NVBLAST_FREE for hulls and hullsOffset when it not needed anymore

        \param[out] hullsOffset     Array of hull offsets for hulls array. The size is meshCount + 1.
        \param[out] hulls           Array of hull. The first i-th mesh hull: hulls[hullsOffset[i]]. The size is written to hullsOffset[meshCount]
        \return                     Number of meshes (meshCount)
    */
    virtual uint32_t        getCollision(uint32_t*& hullsOffset, CollisionHull**& hulls) = 0;

};

/**
    An interface for fbx file reader
*/
class IFbxFileReader : public IMeshFileReader
{
public:
    /**
    Retrieve bone influence if it exist, this is a bone index for each vertex in the mesh
    \note User should call NVBLAST_FREE for out when it not needed anymore

    \param[out] out         Array of bone influences.
    \return                 Number of bones influences (boneCount)
    */
    virtual uint32_t getBoneInfluences(uint32_t*& out) = 0;

    /**
        Return number of bones in fbx file
    */
    virtual uint32_t getBoneCount() = 0;
};

/**
    An interface for Blast mesh file writer
*/
class IMeshFileWriter
{
public:

    /**
        Delete this object
    */
    virtual void release() = 0;

    /**
    Append rendermesh to scene. Meshes constructed from arrays of triangles.
    */
    virtual bool appendMesh(const AuthoringResult& aResult, const char* assetName, bool nonSkinned = false) = 0;

    /**
    Append rendermesh to scene. Meshes constructed from arrays of vertices and indices
    */
    virtual bool appendMesh(const ExporterMeshData& meshData, const char* assetName, bool nonSkinned = false) = 0;

    /**
    Save scene to file.
    */
    virtual bool saveToFile(const char* assetName, const char* outputPath) = 0;

    /**
        Set material index for interior surface. By default new material will be created;
    */
    virtual void setInteriorIndex(int32_t index) = 0;
};

}
}

/**
    Creates an instance of IMeshFileReader for reading obj file.
*/
NV_C_API Nv::Blast::IMeshFileReader* NvBlastExtExporterCreateObjFileReader();

/**
    Creates an instance of IFbxFileReader for reading fbx file.
*/
NV_C_API Nv::Blast::IFbxFileReader* NvBlastExtExporterCreateFbxFileReader();

/**
    Creates an instance of IMeshFileWriter for writing obj file.
*/
NV_C_API Nv::Blast::IMeshFileWriter* NvBlastExtExporterCreateObjFileWriter();

/**
    Creates an instance of IMeshFileWriter for writing fbx file.

    \param[in] outputFBXAscii   If true writes fbx in ascii format otherwise write in binary.
*/
NV_C_API Nv::Blast::IMeshFileWriter* NvBlastExtExporterCreateFbxFileWriter(bool outputFBXAscii = false);

#endif //NVBLASTEXTEXPORTER_H