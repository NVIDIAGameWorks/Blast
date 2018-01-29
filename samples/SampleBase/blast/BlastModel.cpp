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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.


#include "BlastModel.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include "NvBlastExtExporter.h"
#include "NvBlastGlobals.h"

using namespace physx;

BlastModelPtr BlastModel::loadFromFbxFile(const char* path)
{
	std::shared_ptr<BlastModel> model = std::shared_ptr<BlastModel>(new BlastModel());


	std::shared_ptr<Nv::Blast::IFbxFileReader> rdr(NvBlastExtExporterCreateFbxFileReader(), [](Nv::Blast::IFbxFileReader* p) {p->release(); });
	rdr->loadFromFile(path);
	if (rdr->getBoneCount() == 0)
	{
		return nullptr;
	}

	model->chunks.resize(rdr->getBoneCount());

	model->materials.push_back(BlastModel::Material());
	

	/**
		Produce buffers of appropriate for AssetViewer format
	*/
	uint32_t* infl;
	rdr->getBoneInfluences(infl);

	std::vector<int32_t> indRemap(rdr->getVerticesCount(), -1);
	for (uint32_t i = 0; i < rdr->getBoneCount(); ++i)
	{
		std::fill(indRemap.begin(), indRemap.end(), -1);
		SimpleMesh cmesh;
		const uint32_t vertexCount = rdr->getVerticesCount();
		const auto normalsArray = rdr->getNormalsArray();
		const auto positionArray = rdr->getPositionArray();
		const auto uvArray = rdr->getUvArray();

		for (uint32_t j = 0; j < vertexCount; ++j)
		{
			if (i == infl[j])
			{
				indRemap[j] = (int32_t)cmesh.vertices.size();
				cmesh.vertices.push_back(SimpleMesh::Vertex());
				cmesh.vertices.back().normal = normalsArray[j];
				cmesh.vertices.back().position = positionArray[j];
				cmesh.vertices.back().uv = uvArray[j];
			}
		}
		const uint32_t indicesCount = rdr->getIndicesCount();
		const auto indexArray = rdr->getIndexArray();
		for (uint32_t j = 0; j < indicesCount; j += 3)
		{
			//Reverse the winding order
			for (int tv : { 2, 1, 0})
			{
				uint32_t oldIndex = indexArray[j + tv];
				int32_t newIndex = indRemap[oldIndex];
				if (newIndex >= 0)
				{
					cmesh.indices.push_back(newIndex);
				}
			}
		}

		model->chunks[i].meshes.push_back(Chunk::Mesh());
		model->chunks[i].meshes.back().materialIndex = 0;
		model->chunks[i].meshes.back().mesh = std::move(cmesh);
	}
	NVBLAST_FREE(infl);
	return model;
}


void loadMeshes(std::vector<BlastModel::Chunk::Mesh>& meshes, tinyobj::mesh_t mesh)
{
	// Check if all faces are triangles
	bool allTriangles = true;
	for (uint32_t i = 0; i < mesh.num_vertices.size(); ++i)
	{
		if (mesh.num_vertices[i] != 3)
		{
			allTriangles = false;
			break;
		}
	}
	if (!allTriangles) return;

	std::map<int32_t, uint32_t> matIdToMesh;
	for (int32_t mt : mesh.material_ids)
	{
		auto it = matIdToMesh.find(mt);
		if (it == matIdToMesh.end())
		{
			meshes.push_back(BlastModel::Chunk::Mesh());
			matIdToMesh[mt] = uint32_t(meshes.size()) - 1;
		}
	}

	std::vector<SimpleMesh::Vertex> oldVertices(mesh.positions.size() / 3);
	std::vector<uint32_t> oldIndexToNew;

	for (uint32_t i = 0; i < oldVertices.size(); ++i)
	{
		oldVertices[i].position.x = mesh.positions[i * 3];
		oldVertices[i].position.y = mesh.positions[i * 3 + 1];
		oldVertices[i].position.z = mesh.positions[i * 3 + 2];

		oldVertices[i].normal.x = mesh.normals[i * 3];
		oldVertices[i].normal.y = mesh.normals[i * 3 + 1];
		oldVertices[i].normal.z = mesh.normals[i * 3 + 2];

		oldVertices[i].uv.x = mesh.texcoords[i * 2];
		oldVertices[i].uv.y = mesh.texcoords[i * 2 + 1];
	}

	for (auto matmapping : matIdToMesh)
	{
		int32_t mid = matmapping.first;
		auto sampleMesh = &meshes[matmapping.second];
		sampleMesh->materialIndex = (mid >= 0)? mid : 0;
		oldIndexToNew.assign(oldVertices.size(), -1);
		
		PxVec3 emin(FLT_MAX, FLT_MAX, FLT_MAX);
		PxVec3 emax(FLT_MIN, FLT_MIN, FLT_MIN);


		for (uint32_t i = 0; i < mesh.indices.size() / 3; i++)
		{
			if (mesh.material_ids[i] != mid) continue;
			for (int32_t vi = 2; vi >= 0; --vi)
			{
				int32_t idx = mesh.indices[i * 3 + vi];
				if (oldIndexToNew[idx] == -1)
				{
					oldIndexToNew[idx] = (uint32_t)sampleMesh->mesh.vertices.size();
					sampleMesh->mesh.vertices.push_back(oldVertices[idx]);

					emin = emin.minimum(sampleMesh->mesh.vertices.back().position);
					emax = emax.maximum(sampleMesh->mesh.vertices.back().position);				
				}
				sampleMesh->mesh.indices.push_back(oldIndexToNew[idx]);

				// assign extents
				sampleMesh->mesh.extents = (emax - emin) * 0.5f;

				// get the center
				sampleMesh->mesh.center = emin + sampleMesh->mesh.extents;
			}
		}
	}
}


BlastModelPtr BlastModel::loadFromFileTinyLoader(const char* path)
{
	std::shared_ptr<BlastModel> model = std::shared_ptr<BlastModel>(new BlastModel());

	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> mats;
	std::string err;
	std::string mtlPath;
	for (size_t i = strnlen(path, 255) - 1; i >= 0; --i)
	{
		if (path[i] == '\\')
		{
			mtlPath.resize(i + 2, 0);
			strncpy(&mtlPath[0], path, i + 1);
			break;
		}
	}


	bool ret = tinyobj::LoadObj(shapes, mats, err, path, mtlPath.data());

	// can't load?
	if (!ret)
		return false;

	// one submodel per material
	uint32_t materialsCount = (uint32_t)mats.size();
	model->materials.resize(materialsCount);

	// fill submodel materials
	for (uint32_t i = 0; i < materialsCount; i++)
	{
		tinyobj::material_t *pMaterial = &mats[i];

		if (!pMaterial->diffuse_texname.empty())
		{
			model->materials[i].diffuseTexture = pMaterial->diffuse_texname;
		}
	}

	// estimate
	model->chunks.reserve(shapes.size() / materialsCount + 1);

	if (shapes.size() > 0)
	{
		uint32_t meshIndex = 0;
		for (uint32_t m = 0; m < shapes.size(); m++)
		{
			tinyobj::shape_t& pMesh = shapes[m];
			int32_t materialIndex = 0;	// This is actually not set
			uint32_t chunkIndex;
			int32_t sc = sscanf(pMesh.name.data(), "%d_%d", &chunkIndex, &materialIndex);
			if (sc == 0)
			{
				return nullptr;
			}
			if (model->chunks.size() <= chunkIndex)
			{
				model->chunks.resize(chunkIndex + 1);
			}
			loadMeshes(model->chunks.back().meshes, pMesh.mesh);
		}
	}

	return model;
}
