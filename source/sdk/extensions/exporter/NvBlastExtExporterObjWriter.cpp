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
// Copyright (c) 2022 NVIDIA Corporation. All rights reserved.

#define _CRT_SECURE_NO_WARNINGS

#include "NvBlastExtExporterObjWriter.h"
#include <foundation/PxVec3.h>
#include <sstream>
#include "NvBlastExtAuthoringTypes.h"
#include "NvBlastExtAuthoringMesh.h"
#include <algorithm>


using namespace physx;
using namespace Nv::Blast;

const char* gTexPath = "";

void ObjFileWriter::release()
{
    delete this;
}

void ObjFileWriter::setInteriorIndex(int32_t index)
{
    mIntSurfaceMatIndex = index;
}

bool CompByMaterial(const Triangle& a, const Triangle& b)
{
    return a.materialId < b.materialId;
}

bool ObjFileWriter::appendMesh(const AuthoringResult& aResult, const char* /*assetName*/, bool /*nonSkinned*/)
{
    mMeshData = std::shared_ptr<ExporterMeshData>(new ExporterMeshData(), [](ExporterMeshData* md)
    {
        //delete[] md->hulls;
        //delete[] md->hullsOffsets;
        delete[] md->normals;
        //delete[] md->normIndex;
        delete[] md->posIndex;
        delete[] md->positions;
        delete[] md->submeshOffsets;
        //delete[] md->texIndex;
        delete[] md->submeshMats;
        delete[] md->uvs;
        delete md;
    });

    
    ExporterMeshData& md = *mMeshData.get();
    uint32_t triCount = aResult.geometryOffset[aResult.chunkCount];
    md.meshCount = aResult.chunkCount;
    md.submeshCount = aResult.materialCount;
    
    int32_t additionalMats = 0;

    if (mIntSurfaceMatIndex == -1 || mIntSurfaceMatIndex >= (int32_t)md.submeshCount)
    {
        md.submeshCount += 1;
        mIntSurfaceMatIndex = md.submeshCount - 1;
        additionalMats = 1;
    }

    md.submeshOffsets = new uint32_t[md.meshCount * md.submeshCount + 1];
    md.submeshMats = new Material[md.submeshCount];

    for (uint32_t i = 0; i < md.submeshCount - additionalMats; ++i)
    {
        md.submeshMats[i].name = aResult.materialNames[i];
        md.submeshMats[i].diffuse_tex = nullptr;
    }

    if (additionalMats)
    {
        md.submeshMats[mIntSurfaceMatIndex].name = interiorNameStr.c_str();
        md.submeshMats[mIntSurfaceMatIndex].diffuse_tex = nullptr;
    }
    md.positionsCount = triCount * 3;
    md.normalsCount = md.positionsCount;
    md.uvsCount = md.positionsCount;
    md.positions = new NvcVec3[md.positionsCount];
    md.normals = new NvcVec3[md.normalsCount];
    md.uvs = new NvcVec2[md.uvsCount];

    md.posIndex = new uint32_t[triCount * 3];
    md.normIndex = md.posIndex;
    md.texIndex = md.posIndex;



    /**
        Now we need to sort input trianles chunk they belong to, then by material;
    */
    std::vector<Triangle> sorted;
    sorted.reserve(triCount);


    int32_t perChunkOffset = 0;
    for (uint32_t i = 0; i < md.meshCount; ++i)
    {
        std::vector<uint32_t> perMaterialCount(md.submeshCount);

        uint32_t first = aResult.geometryOffset[i];
        uint32_t last = aResult.geometryOffset[i + 1];
        uint32_t firstInSorted = (uint32_t)sorted.size();
        for (uint32_t t = first; t < last; ++t)
        {
            sorted.push_back(aResult.geometry[t]);
            int32_t cmat = sorted.back().materialId;
            if (cmat == kMaterialInteriorId)
            {
                cmat = mIntSurfaceMatIndex;
            }
            perMaterialCount[cmat]++;
        }
        for (uint32_t mof = 0; mof < md.submeshCount; ++mof)
        {
            md.submeshOffsets[i * md.submeshCount + mof] = perChunkOffset * 3;
            perChunkOffset += perMaterialCount[mof];
        }
        std::sort(sorted.begin() + firstInSorted, sorted.end(), CompByMaterial);
    }
    md.submeshOffsets[md.meshCount * md.submeshCount] = perChunkOffset * 3;

    for (uint32_t vc = 0; vc < triCount; ++vc)
    {
        Triangle& tri = sorted[vc];
        uint32_t i = vc * 3;
        md.positions[i+0] = tri.a.p;
        md.positions[i+1] = tri.b.p;
        md.positions[i+2] = tri.c.p;

        md.normals[i+0] = tri.a.n;
        md.normals[i+1] = tri.b.n;
        md.normals[i+2] = tri.c.n;
        
        md.uvs[i+0] = tri.a.uv[0];
        md.uvs[i+1] = tri.b.uv[0];
        md.uvs[i+2] = tri.c.uv[0];

        md.posIndex[i + 0] = i + 0;
        md.posIndex[i + 1] = i + 1;
        md.posIndex[i + 2] = i + 2;
    }
    return true;
}

bool ObjFileWriter::appendMesh(const ExporterMeshData& meshData, const char* /*assetName*/, bool /*nonSkinned*/)
{
    mMeshData = std::shared_ptr<ExporterMeshData>(new ExporterMeshData(meshData));
    return true;
}

bool ObjFileWriter::saveToFile(const char* assetName, const char* outputPath)
{
    if (mMeshData.get() == nullptr)
    {
        return false;
    }
    const ExporterMeshData& md = *mMeshData.get();

    uint32_t chunkCount = md.meshCount;

    // export materials (mtl file)
    {
        std::ostringstream mtlFilePath;
        mtlFilePath << outputPath << "\\" << assetName << ".mtl";
        FILE* f = fopen(mtlFilePath.str().c_str(), "w");
        if (!f)
            return false;

        for (uint32_t submeshIndex = 0; submeshIndex < md.submeshCount; ++submeshIndex)
        {
            fprintf(f, "newmtl %s\n", md.submeshMats[submeshIndex].name);
            if (md.submeshMats[submeshIndex].diffuse_tex != nullptr)
            {
                fprintf(f, "\tmap_Kd %s\n", md.submeshMats[submeshIndex].diffuse_tex);
            }
            else
            {
                fprintf(f, "\tKd %f %f %f\n", float(rand()) / RAND_MAX, float(rand()) / RAND_MAX, float(rand()) / RAND_MAX);
            }
            fprintf(f, "\n");
        }

        fclose(f);
    }

    /// Export geometry to *.obj file
    {
        std::ostringstream objFilePath;
        objFilePath << outputPath << "\\" << assetName << ".obj";
        FILE* f = fopen(objFilePath.str().c_str(), "w");
        if (!f)
            return false;

        fprintf(f, "mtllib %s.mtl\n", assetName);
        fprintf(f, "o frac \n");


        /// Write compressed vertices
        for (uint32_t i = 0; i < md.positionsCount; ++i)
        {
            fprintf(f, "v %.4f %.4f %.4f\n", md.positions[i].x, md.positions[i].y, md.positions[i].z);
        }
        for (uint32_t i = 0; i < md.normalsCount; ++i)
        {
            fprintf(f, "vn %.4f %.4f %.4f\n", md.normals[i].x, md.normals[i].y, md.normals[i].z);
        }
        for (uint32_t i = 0; i < md.uvsCount; ++i)
        {
            fprintf(f, "vt %.4f %.4f\n", md.uvs[i].x, md.uvs[i].y);
        }

        for (uint32_t chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex)
        {
            fprintf(f, "g %d \n", chunkIndex);
            for (uint32_t submeshIndex = 0; submeshIndex < md.submeshCount; ++submeshIndex)
            {
                uint32_t firstIdx = md.submeshOffsets[chunkIndex * md.submeshCount + submeshIndex];
                uint32_t lastIdx = md.submeshOffsets[chunkIndex * md.submeshCount + submeshIndex + 1];
                if (firstIdx == lastIdx) // There is no trianlges in this submesh.
                {
                    continue;
                }
                fprintf(f, "usemtl %s\n", md.submeshMats[submeshIndex].name);
                for (uint32_t i = firstIdx; i < lastIdx; i += 3)
                {
                    fprintf(f, "f %d/%d/%d  ", md.posIndex[i] + 1, md.texIndex[i] + 1, md.normIndex[i] + 1);
                    fprintf(f, "%d/%d/%d  ", md.posIndex[i + 1] + 1, md.texIndex[i + 1] + 1, md.normIndex[i + 1] + 1);
                    fprintf(f, "%d/%d/%d \n", md.posIndex[i + 2] + 1, md.texIndex[i + 2] + 1, md.normIndex[i + 2] + 1);
                }
            }
        }
        fclose(f);
    }
    return true;

}

