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
// Copyright (c) 2016-2024 NVIDIA Corporation. All rights reserved.


#ifndef BLAST_MODEL_H
#define BLAST_MODEL_H

#include "Mesh.h"
#include <vector>
#include <memory>


class BlastModel;
typedef std::shared_ptr<const BlastModel> BlastModelPtr;

/**
BlastModel struct represents graphic model.
Now only loading from .obj file is supported.
Can have >=0 materials
Every chunk can have multiple meshes (1 for every material)
*/
class BlastModel
{
public:
    struct Material
    {
        std::string diffuseTexture;
    };

    struct Chunk
    {
        struct Mesh
        {
            uint32_t materialIndex;
            SimpleMesh mesh;
        };

        std::vector<Mesh> meshes;
    };

    std::vector<Material> materials;
    std::vector<Chunk> chunks;

    static BlastModelPtr loadFromFileTinyLoader(const char* path);
    static BlastModelPtr loadFromFbxFile(const char* path);
private:
    BlastModel() {}
};

#endif // ifndef BLAST_MODEL_H