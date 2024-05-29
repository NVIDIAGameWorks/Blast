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


#ifndef NVBLASTAUTHORINGRTMULTITHREADEDIMPL_H
#define NVBLASTAUTHORINGRTMULTITHREADEDIMPL_H

#include <NvBlastExtRT.h>
#include <NvBlastExtRTImpl.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>

namespace Nv
{
    namespace Blast
    {
        class FractureRTMultithreadedImpl : public FractureRT
        {
        public:
            FractureRTMultithreadedImpl(uint32_t threadCount);

            void release() override;
            void processMesh(DamagePattern* pattern, const Mesh* msh) override;
            uint32_t getResultChunkCount() override;
            Vertex* getVertexBuffer() override;
            uint32_t* getIndexBuffer() override;
            uint32_t* getVertexOffset() override;
            uint32_t* getIndexOffset() override;
            PerTriangleAdditionalData* getPerTriangleData() override;
            void dumpChunksToObj(const char* path) override;

        private:
            Vertex* vertexBuffer = nullptr;
            uint32_t* indexBuffer = nullptr;

            uint32_t* vertexOffsets = nullptr;
            uint32_t* indexOffsets = nullptr;

            PerTriangleAdditionalData* adata = nullptr;

            uint32_t chunkCount;

#ifdef USE_MERGED_MESH
            BooleanToolOutputData* outputData;
#endif

            struct PerThreadToolsAndData
            {
                PerThreadToolsAndData();
                ~PerThreadToolsAndData();

                Fracturer*          f = nullptr;
                MeshGenerator*      mgen = nullptr;
                Vertex*             vertexBuffer = nullptr;
                uint32_t*           indexBuffer = nullptr;
                uint32_t*           indexOffsets = nullptr;
                uint32_t*           vertexOffsets = nullptr;
                uint32_t*           perChunkIds = nullptr;
                uint32_t            chunkCount;
                PerTriangleAdditionalData* adata = nullptr;

                SpatialAccelerator* accel = nullptr;
                BooleanToolOutputData* outputData = nullptr;
            };

            struct FractureJob
            {
                FractureJob() {};
                FractureJob(uint32_t chunkId, const Mesh* mesh, Mesh* cell,
                    int32_t stage = FractureRT::Stage::ALL, DamagePattern* pattern = nullptr)
                    : chunkId(chunkId), mesh(mesh), cell(cell), stage(stage), pattern(pattern) {}

                uint32_t chunkId;
                const Nv::Blast::Mesh* mesh;
                Nv::Blast::Mesh* cell;
                int32_t stage = FractureRT::Stage::ALL;
                DamagePattern* pattern = nullptr;
            };

            std::mutex work_mtx;
            std::condition_variable hasAJob;
            std::vector<FractureJob> fractureJobList;
            std::vector<PerThreadToolsAndData> perThreadTd;
            std::vector<std::thread> threadPool;
            std::atomic<int32_t> jobCounter;

            void waitForJob(int32_t threadId);
            bool terminateThreads;
            void pushJob(FractureJob& j);
        };
    }
}

#endif // ifndef NVBLASTAUTHORINGRTMULTITHREADEDIMPL_H
