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
// Copyright (c) 2008-2017 NVIDIA Corporation. All rights reserved.


#include "BlastFractureTool.h"
#include <BlastAssetModel.h>
#include <BlastAsset.h>
#include <NvBlastExtPxAsset.h>
#include <NvBlastTkAsset.h>

void BlastFractureTool::free()
{
	std::vector<Nv::Blast::Mesh*>::iterator it;
	for (it = chunkMeshes.begin(); it != chunkMeshes.end(); it++)
	{
		delete (*it);
		(*it) = nullptr;
	}
	chunkMeshes.clear();
}

void BlastFractureTool::setSourceAsset(const BlastAsset* pBlastAsset)
{
	free();

	BlastAssetModel* pBlastAssetModel = (BlastAssetModel*)pBlastAsset;
	const BlastModel& blastModel = pBlastAssetModel->getModel();
	const std::vector<BlastModel::Chunk>& chunks = blastModel.chunks;
	int chunkSize = chunks.size();
	if (chunkSize == 0)
	{
		return;
	}

	chunkMeshes.resize(chunkSize, nullptr);
	for (int cs = 0; cs < chunkSize; cs++)
	{
		const BlastModel::Chunk& chunk = chunks[cs];
		const std::vector<BlastModel::Chunk::Mesh>& meshes = chunk.meshes;
		int meshSize = meshes.size();

		if (meshSize == 0)
		{
			continue;
		}

		std::vector<physx::PxVec3> positions;
		std::vector<physx::PxVec3> normals;
		std::vector<physx::PxVec2> uv;
		std::vector<uint32_t>  ind;
		std::vector<int>  faceBreakPoint;
		std::vector<uint32_t>  materialIndexes;
		uint16_t curIndex = 0;
		for (int ms = 0; ms < meshSize; ms++)
		{
			const BlastModel::Chunk::Mesh& mesh = meshes[ms];
			materialIndexes.push_back(mesh.materialIndex);
			const SimpleMesh& simpleMesh = mesh.mesh;
			const std::vector<SimpleMesh::Vertex>& vertices = simpleMesh.vertices;
			const std::vector<uint16_t>& indices = simpleMesh.indices;

			int NumVertices = vertices.size();
			for (uint32_t i = 0; i < NumVertices; ++i)
			{
				positions.push_back(physx::PxVec3(vertices[i].position.x, vertices[i].position.y, vertices[i].position.z));
				normals.push_back(physx::PxVec3(vertices[i].normal.x, vertices[i].normal.y, vertices[i].normal.z));
				uv.push_back(physx::PxVec2(vertices[i].uv.x, vertices[i].uv.y));
			}
			int NumIndices = indices.size();
			for (uint32_t i = 0; i < NumIndices; ++i)
			{
				ind.push_back(indices[i] + curIndex);
			}
			curIndex += NumIndices;
			faceBreakPoint.push_back(NumIndices / 3);
		}

		PxVec3* nr = (!normals.empty()) ? normals.data() : 0;
		PxVec2* uvp = (!uv.empty()) ? uv.data() : 0;
		Nv::Blast::Mesh* pMesh = new Nv::Blast::Mesh(
			positions.data(), nr, uvp, static_cast<uint32_t>(positions.size()),
			ind.data(), static_cast<uint32_t>(ind.size()));

		int curFaceIndex = 0;
		int curFaceBreakPoint = faceBreakPoint[curFaceIndex];
		uint32_t curMaterialIndex = materialIndexes[curFaceIndex];
		for (int fc = 0; fc < pMesh->getFacetCount(); fc++)
		{
			if (fc >= curFaceBreakPoint)
			{
				curFaceIndex++;
				curFaceBreakPoint += faceBreakPoint[curFaceIndex];
				curMaterialIndex = materialIndexes[curFaceIndex];
			}

			pMesh->getFacet(fc)->userData = curMaterialIndex;
		}

		chunkMeshes[cs] = pMesh;
	}

	const ExtPxAsset* pExtPxAsset = pBlastAsset->getPxAsset();
	const TkAsset& tkAsset = pExtPxAsset->getTkAsset();

	std::vector<int32_t> parentIds;
	parentIds.resize(chunkSize);
	parentIds.assign(chunkSize, -1);
	{
		const NvBlastChunk* pNvBlastChunk = tkAsset.getChunks();
		for (uint32_t i = 0; i < chunkSize; ++i)
		{
			parentIds[i] = pNvBlastChunk[i].parentChunkIndex;
		}
	}
	std::vector<bool> isLeafs;
	isLeafs.resize(chunkSize);
	isLeafs.assign(chunkSize, false);
	{
		const NvBlastSupportGraph supportGraph = tkAsset.getGraph();
		for (uint32_t i = 0; i < supportGraph.nodeCount; ++i)
		{
			const uint32_t chunkIndex = supportGraph.chunkIndices[i];
			if (chunkIndex < chunkSize)
			{
				isLeafs[chunkIndex] = true;
			}
		}
	}

	setSourceMesh(chunkMeshes[0]);

	mChunkData.resize(chunkSize);
	mChunkData[0].parent = parentIds[0];
	mChunkData[0].isLeaf = isLeafs[0];
	mChunkData[0].chunkId = 0;
	Nv::Blast::Mesh* mesh = nullptr;
	for (int cs = 1; cs < chunkSize; cs++)
	{
		if (chunkMeshes[cs] == nullptr)
			continue;
		mChunkData[cs].meshData = new Nv::Blast::Mesh(*chunkMeshes[cs]);
		mChunkData[cs].parent = parentIds[cs];
		mChunkData[cs].chunkId = mChunkIdCounter++;
		mChunkData[cs].isLeaf = isLeafs[cs];

		mesh = mChunkData[cs].meshData;
		Nv::Blast::Vertex* verticesBuffer = mesh->getVertices();
		for (uint32_t i = 0; i < mesh->getVerticesCount(); ++i)
		{
			verticesBuffer[i].p = (verticesBuffer[i].p - mOffset) * (1.0f / mScaleFactor);
		}
		mesh->getBoundingBox().minimum = (mesh->getBoundingBox().minimum - mOffset) * (1.0f / mScaleFactor);
		mesh->getBoundingBox().maximum = (mesh->getBoundingBox().maximum - mOffset) * (1.0f / mScaleFactor);
		for (uint32_t i = 0; i < mesh->getFacetCount(); ++i)
		{
			mesh->getFacet(i)->userData = chunkMeshes[cs]->getFacet(i)->userData;
		}
	}
}

void BlastFractureTool::setSourceMeshes(std::vector<Nv::Blast::Mesh*>& meshes, std::vector<int32_t>& parentIds)
{
	free();

	int chunkSize = meshes.size();
	if (chunkSize == 0)
	{
		return;
	}

	chunkMeshes.resize(chunkSize, nullptr);
	for (int cs = 0; cs < chunkSize; cs++)
	{
		Nv::Blast::Mesh* pMesh = new Nv::Blast::Mesh(*meshes[cs]);
		chunkMeshes[cs] = pMesh;
	}

	std::vector<bool> isLeafs;
	isLeafs.resize(chunkSize, true);
	for (int cs = 0; cs < chunkSize; cs++)
	{
		int32_t parentId = parentIds[cs];
		if (parentId == -1)
		{
			continue;
		}

		isLeafs[parentId] = false;
	}

	setSourceMesh(chunkMeshes[0]);

	mChunkData.resize(chunkSize);
	mChunkData[0].parent = parentIds[0];
	mChunkData[0].isLeaf = isLeafs[0];
	mChunkData[0].chunkId = 0;
	Nv::Blast::Mesh* mesh = nullptr;
	for (int cs = 1; cs < chunkSize; cs++)
	{
		if (chunkMeshes[cs] == nullptr)
			continue;
		mChunkData[cs].meshData = new Nv::Blast::Mesh(*chunkMeshes[cs]);
		mChunkData[cs].parent = parentIds[cs];
		mChunkData[cs].chunkId = mChunkIdCounter++;
		mChunkData[cs].isLeaf = isLeafs[cs];

		mesh = mChunkData[cs].meshData;
		Nv::Blast::Vertex* verticesBuffer = mesh->getVertices();
		for (uint32_t i = 0; i < mesh->getVerticesCount(); ++i)
		{
			verticesBuffer[i].p = (verticesBuffer[i].p - mOffset) * (1.0f / mScaleFactor);
		}
		mesh->getBoundingBox().minimum = (mesh->getBoundingBox().minimum - mOffset) * (1.0f / mScaleFactor);
		mesh->getBoundingBox().maximum = (mesh->getBoundingBox().maximum - mOffset) * (1.0f / mScaleFactor);
		for (uint32_t i = 0; i < mesh->getFacetCount(); ++i)
		{
			mesh->getFacet(i)->userData = chunkMeshes[cs]->getFacet(i)->userData;
		}
	}
}

Nv::Blast::Mesh* BlastFractureTool::getSourceMesh(int32_t chunkId)
{
	if (chunkId < 0 || chunkId >= chunkMeshes.size())
	{
		return nullptr;
	}
	return chunkMeshes[chunkId];
}