// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2016-2020 NVIDIA Corporation. All rights reserved.


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

				Fracturer*			f = nullptr;
				MeshGenerator*		mgen = nullptr;
				Vertex*				vertexBuffer = nullptr;
				uint32_t*			indexBuffer = nullptr;
				uint32_t*			indexOffsets = nullptr;
				uint32_t*			vertexOffsets = nullptr;
				uint32_t*			perChunkIds = nullptr;
				uint32_t			chunkCount;
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
