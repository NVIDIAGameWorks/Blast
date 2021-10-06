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

#include "NvBlastExtRTMultithreadedImpl.h"
#include "NvBlastExtRTGeometry.h"
#include "NvBlastExtAuthoringMeshImpl.h"
#include "NvBlastExtAuthoringAccelerator.h"
#include "NvBlastExtAuthoringPatternGenerator.h"
#include "NvBlastPxSharedHelpers.h"

#include "NvBlastGlobals.h"
#include "NvBlastAssert.h"
//#include "PxPhysicsAPI.h"

using namespace Nv::Blast;

#define VBSIZE_PERTHREAD 8192
#define IBSIZE_PERTHREAD 8192 * 3

#define SAFE_RELEASE(x) if (x) {(x)->release(); (x) = nullptr;}
#define SAFE_FREE(x) if (x) {NVBLAST_FREE(x); (x) = nullptr;}

#ifdef USE_MERGED_MESH
static bool compareEdge(const BooleanResultEdge& e1, const BooleanResultEdge& e2)
{
	return e1.parentFacet < e2.parentFacet;
}
#endif

FractureRTMultithreadedImpl::PerThreadToolsAndData::PerThreadToolsAndData()
{
	f = NvBlastExtRTCreateFracturer();
	mgen = NvBlastExtRTCreateMeshGenerator();
#ifdef USE_MERGED_MESH
	outputData = CreateBooleanToolOutputData(true);
#else
	outputData = CreateBooleanToolOutputData();
#endif
	
	vertexBuffer = (Vertex*)NVBLAST_ALLOC(sizeof(Vertex) * VBSIZE_PERTHREAD);
	indexBuffer = (uint32_t*)NVBLAST_ALLOC(sizeof(uint32_t) * IBSIZE_PERTHREAD);
	adata = (PerTriangleAdditionalData*)NVBLAST_ALLOC(sizeof(PerTriangleAdditionalData) * VBSIZE_PERTHREAD);

	vertexOffsets = (uint32_t*)NVBLAST_ALLOC(sizeof(uint32_t) * BLASTRT_MAX_CHUNKS);
	indexOffsets = (uint32_t*)NVBLAST_ALLOC(sizeof(uint32_t) * BLASTRT_MAX_CHUNKS);
	perChunkIds = (uint32_t*)NVBLAST_ALLOC(sizeof(uint32_t) * BLASTRT_MAX_CHUNKS);

}

FractureRTMultithreadedImpl::PerThreadToolsAndData::~PerThreadToolsAndData()
{
	SAFE_RELEASE(f);
	SAFE_RELEASE(mgen);
	SAFE_RELEASE(outputData);

	SAFE_FREE(vertexBuffer);
	SAFE_FREE(indexBuffer);
	SAFE_FREE(adata);
	SAFE_FREE(vertexOffsets);
	SAFE_FREE(indexOffsets);
	SAFE_FREE(perChunkIds);
}

FractureRTMultithreadedImpl::FractureRTMultithreadedImpl(uint32_t threadCount)
{
	perThreadTd.resize(threadCount);
	terminateThreads = false;
	for (uint32_t i = 0; i < threadCount; ++i)
	{
		threadPool.push_back(std::thread(&FractureRTMultithreadedImpl::waitForJob, this, i));
		threadPool.back().detach();
	}
#ifdef USE_MERGED_MESH
	outputData = CreateBooleanToolOutputData();
#endif
	vertexBuffer = (Vertex*)NVBLAST_ALLOC(sizeof(Vertex) *  8192 * 6);
	indexBuffer = (uint32_t*)NVBLAST_ALLOC(sizeof(uint32_t) * 8192 * 12);
	vertexOffsets = (uint32_t*)NVBLAST_ALLOC(sizeof(uint32_t) * 1024);
	indexOffsets = (uint32_t*)NVBLAST_ALLOC(sizeof(uint32_t) * 1024);

	adata = (PerTriangleAdditionalData*)NVBLAST_ALLOC(sizeof(PerTriangleAdditionalData) * 8192 * 12);

	chunkCount = 0;
}

void FractureRTMultithreadedImpl::release()
{
#ifdef USE_MERGED_MESH
	SAFE_RELEASE(outputData);
#endif
	SAFE_FREE(vertexBuffer);
	SAFE_FREE(indexBuffer);
	SAFE_FREE(vertexOffsets);
	SAFE_FREE(indexOffsets);
	SAFE_FREE(adata);

	NVBLAST_DELETE(this, FractureRTMultithreadedImpl);
}

void FractureRTMultithreadedImpl::waitForJob(const int32_t threadIndex)
{
	while (!terminateThreads)
	{
		FractureJob j;
		{
			std::unique_lock<std::mutex> mlock(work_mtx);
			hasAJob.wait(mlock, [&]() {return !fractureJobList.empty(); });
			j = fractureJobList.back();
			fractureJobList.pop_back();
		}

		FractureDesc dsc;
		dsc.fr = perThreadTd[threadIndex].f;
		dsc.model = j.mesh;
		dsc.cell = j.cell;
		dsc.modelAccel = perThreadTd[threadIndex].accel;
		dsc.chunkId = j.chunkId;
		dsc.outputData = perThreadTd[threadIndex].outputData;
		dsc.pattern = j.pattern;

		NvBlastExtRTDoFracture(dsc, j.stage, threadIndex, (int32_t)perThreadTd.size());


		if (j.stage & FractureRT::Stage::RETAIN_FROM_FRACTURED_MESH)
		{
			//Debug code
			//std::set<TmpE, CmpTmpE> tmp;
			//for (uint32_t i = 0; i < edgesCount; i++)
			//{
			//	TmpE t;
			//	t.f = edges[i].parentFacet;
			//	t.v1 = dsc.outputData->vertices[edges[i].start].p;
			//	t.v2 = dsc.outputData->vertices[edges[i].end].p;
			//	tmp.insert(t);
			//}

			/**
			Generate output mesh
			*/

			Vertex* vtp = perThreadTd[threadIndex].vertexBuffer + perThreadTd[threadIndex].vertexOffsets[perThreadTd[threadIndex].chunkCount];
			uint32_t* vic = perThreadTd[threadIndex].indexBuffer + perThreadTd[threadIndex].indexOffsets[perThreadTd[threadIndex].chunkCount];
			PerTriangleAdditionalData* padata = perThreadTd[threadIndex].adata + perThreadTd[threadIndex].indexOffsets[perThreadTd[threadIndex].chunkCount] / 3;


			uint32_t nvc = 0;

			int32_t spaceForIndices = IBSIZE_PERTHREAD - perThreadTd[threadIndex].indexOffsets[perThreadTd[threadIndex].chunkCount];
			int32_t spaceForVertices = VBSIZE_PERTHREAD - perThreadTd[threadIndex].vertexOffsets[perThreadTd[threadIndex].chunkCount];

			MeshDesc mgendesc;
			mgendesc.tr = perThreadTd[threadIndex].mgen;

			mgendesc.inVertices = dsc.outputData->vertices;
			mgendesc.meshA = j.mesh;
			mgendesc.meshB = j.cell;

#ifdef USE_MERGED_MESH
			auto edges = (BooleanResultEdge*)dsc.pattern->outputEdges + dsc.chunkId * BLASTRT_MAX_EDGES_PER_CHUNK;
			for (uint32_t e = 0; e < dsc.outputData->edgesCount(); e++)
			{
				edges[dsc.pattern->outputEdgesCount[dsc.chunkId]++] = dsc.outputData->edges[e];
			}
			mgendesc.bEdges = edges;
			mgendesc.edesCount = dsc.pattern->outputEdgesCount[dsc.chunkId];
			dsc.outputData->resetEdges();
			std::sort(edges, edges + mgendesc.edesCount, compareEdge);
#else
			mgendesc.bEdges = dsc.outputData->edges;
			mgendesc.edesCount = dsc.outputData->edgesCount();
#endif

			uint32_t newTriangles = NvBlastExtRTBuildMesh(mgendesc, vtp, nvc, vic, padata, spaceForIndices, spaceForVertices);

			if (newTriangles > 0)
			{
				/**
					Check if generated mesh bounding box is valid.
				*/
				physx::PxBounds3 bds(physx::PxBounds3::empty());
				for (uint32_t i = 0; i < newTriangles * 3; ++i)
				{
					bds.include(toPxShared(vtp[vic[i]].p));
				}
				if (bds.isValid())
				{
					perThreadTd[threadIndex].chunkCount++;
					perThreadTd[threadIndex].indexOffsets[perThreadTd[threadIndex].chunkCount] = perThreadTd[threadIndex].indexOffsets[perThreadTd[threadIndex].chunkCount - 1] + newTriangles * 3;
					perThreadTd[threadIndex].vertexOffsets[perThreadTd[threadIndex].chunkCount] = perThreadTd[threadIndex].vertexOffsets[perThreadTd[threadIndex].chunkCount - 1] + nvc;
				}
			}
		}

		jobCounter--;
	}
}

void FractureRTMultithreadedImpl::pushJob(FractureJob& j)
{
	std::lock_guard<std::mutex> lck(work_mtx);
	fractureJobList.push_back(j);
	hasAJob.notify_one();
}

void FractureRTMultithreadedImpl::processMesh(DamagePattern* pattern, const Mesh* msh)
{
	NVBLAST_ASSERT(pattern);
	NVBLAST_ASSERT(msh);
	//NVBLAST_ASSERT(preparedMeshA);

	chunkCount = 0;

	std::shared_ptr<Grid> meshGrid(NVBLAST_NEW(Grid)(5), [](Grid* d) {NVBLAST_DELETE(d, Grid); });
	meshGrid->setMesh(msh);

	std::vector<std::shared_ptr<GridWalker>> perThreadAccels;
	perThreadAccels.reserve(threadPool.size());
	for (uint32_t i = 0; i < threadPool.size(); ++i)
	{
		std::shared_ptr<GridWalker> wlk(NVBLAST_NEW(GridWalker)(meshGrid.get()), [](GridWalker* d) {
			NVBLAST_DELETE(d, GridWalker); 
		});
		perThreadAccels.push_back(wlk);
		perThreadTd[i].accel = perThreadAccels[i].get();
	}


	for (uint32_t i = 0; i < threadPool.size(); ++i)
	{
		perThreadTd[i].chunkCount = 0;
		perThreadTd[i].vertexOffsets[0] = 0;
		perThreadTd[i].indexOffsets[0] = 0;
		perThreadTd[i].outputData->reset();
	}

	uint32_t finalStage = FractureRT::Stage::ALL;

#ifdef USE_MERGED_MESH
	uint32_t threadCount = (uint32_t)threadPool.size();

	outputData->reset();
	std::vector<BooleanToolOutputData*> outputDataPtrs(threadPool.size());
	for (uint32_t i = 0; i < threadPool.size(); ++i)
	{
		outputDataPtrs[i] = perThreadTd[i].outputData;
		outputDataPtrs[i]->copyVerticesAndResults(outputData);
		perThreadTd[i].outputData = outputData;
	}
	for (uint32_t i = 0; i < pattern->cellsCount; i++)
	{
		pattern->outputEdgesCount[i] = 0;
	}

	jobCounter = threadCount;
	for (uint32_t i = 0; i < threadCount; ++i)
	{
		FractureJob jb(i, msh, pattern->mergedMesh, preparedMeshA, pattern->preparedMergedMesh, FractureRT::Stage::FACET_FACET_TEST, pattern);
		pushJob(jb);
	}
	while (jobCounter != 0);

	sortResultBuffer(outputData->ffResult, outputData->ffResultCount());

	jobCounter = threadCount;
	for (uint32_t i = 0; i < threadCount; ++i)
	{
		FractureJob jb(i, msh, pattern->mergedMesh, preparedMeshA, pattern->preparedMergedMesh, FractureRT::Stage::RETAIN_FROM_PATTERN, pattern);
		pushJob(jb);
	}
	while (jobCounter != 0);

	procesOutputEdges(outputData, pattern, msh->getFacetCount());

	for (uint32_t i = 0; i < threadCount; ++i)
	{
		perThreadTd[i].outputData = outputDataPtrs[i];
	}

	finalStage = FractureRT::Stage::RETAIN_FROM_FRACTURED_MESH;
#endif

	jobCounter = pattern->cellsCount;
	for (uint32_t i = 0; i < pattern->cellsCount; ++i)
	{
		FractureJob jb(i, msh, pattern->cellsMeshes[i], finalStage, pattern);
		pushJob(jb);
	}
	while (jobCounter != 0); // Wait for all threads to finish


	Vertex* vtp = vertexBuffer;
	uint32_t* vic = indexBuffer;
	PerTriangleAdditionalData* adc = adata;


	int32_t voff = 0;
	int32_t ioff = 0;
	uint32_t glbc = 0;
	for (uint32_t i = 0; i < threadPool.size(); ++i)
	{
		if (perThreadTd[i].chunkCount == 0)
		{
			continue;
		}
		chunkCount += perThreadTd[i].chunkCount;
		int32_t tc = perThreadTd[i].indexOffsets[perThreadTd[i].chunkCount] / 3;
		memcpy(vtp, perThreadTd[i].vertexBuffer, sizeof(Vertex) * perThreadTd[i].vertexOffsets[perThreadTd[i].chunkCount]);
		memcpy(vic, perThreadTd[i].indexBuffer, sizeof(uint32_t) * perThreadTd[i].indexOffsets[perThreadTd[i].chunkCount]);
		memcpy(adc, perThreadTd[i].adata, sizeof(PerTriangleAdditionalData) * tc);

		vtp += perThreadTd[i].vertexOffsets[perThreadTd[i].chunkCount];
		vic += perThreadTd[i].indexOffsets[perThreadTd[i].chunkCount];
		adc += tc;


		for (uint32_t ch = 0; ch < perThreadTd[i].chunkCount; ++ch)
		{
			vertexOffsets[glbc] = perThreadTd[i].vertexOffsets[ch] + voff;
			indexOffsets[glbc] = perThreadTd[i].indexOffsets[ch] + ioff;
			glbc++;
		}
		voff += perThreadTd[i].vertexOffsets[perThreadTd[i].chunkCount];
		ioff += perThreadTd[i].indexOffsets[perThreadTd[i].chunkCount];
	}
	vertexOffsets[glbc] = voff;
	indexOffsets[glbc] = ioff;
}


uint32_t FractureRTMultithreadedImpl::getResultChunkCount() { return chunkCount; };
Nv::Blast::Vertex* FractureRTMultithreadedImpl::getVertexBuffer() { return vertexBuffer; };
uint32_t* FractureRTMultithreadedImpl::getIndexBuffer() { return indexBuffer; };

uint32_t* FractureRTMultithreadedImpl::getIndexOffset() { return indexOffsets; };
uint32_t* FractureRTMultithreadedImpl::getVertexOffset() { return vertexOffsets; };


void FractureRTMultithreadedImpl::dumpChunksToObj(const char* path)
{
	FILE* output = fopen(path, "w");

	for (uint32_t i = 0; i < vertexOffsets[chunkCount]; ++i)
	{
		fprintf(output, "v %f %f %f\n", vertexBuffer[i].p.x, vertexBuffer[i].p.y, vertexBuffer[i].p.z);
	}

	for (uint32_t i = 0; i < vertexOffsets[chunkCount]; ++i)
	{
		fprintf(output, "vn %f %f %f\n", vertexBuffer[i].n.x, vertexBuffer[i].n.y, vertexBuffer[i].n.z);
	}
	for (uint32_t i = 0; i < vertexOffsets[chunkCount]; ++i)
	{
		fprintf(output, "vt %f %f\n", vertexBuffer[i].uv[0].x, vertexBuffer[i].uv[0].y);
	}

	for (uint32_t chunk = 0; chunk < chunkCount; ++chunk)
	{
		fprintf(output, "g chunk_%d\n", chunk);
		uint32_t trc = (indexOffsets[chunk + 1] - indexOffsets[chunk]) / 3;
		uint32_t* idx = &indexBuffer[indexOffsets[chunk]];
		uint32_t vofs = vertexOffsets[chunk] + 1;
		for (uint32_t t = 0; t < trc; ++t)
		{
			fprintf(output, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", vofs + idx[0], vofs + idx[0], vofs + idx[0],
				vofs + idx[1], vofs + idx[1], vofs + idx[1],
				vofs + idx[2], vofs + idx[2], vofs + idx[2]);
			idx += 3;
		}
	}
	fclose(output);
}


PerTriangleAdditionalData* FractureRTMultithreadedImpl::getPerTriangleData()
{
	return adata;
}

