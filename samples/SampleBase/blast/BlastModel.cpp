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
	for (uint32_t i = 0; i < rdr->getBoneCount(); ++i)
	{
		std::vector<int32_t> indRemap(rdr->getVerticesCount(), -1);
		std::vector<uint32_t> indices;
		SimpleMesh cmesh;
		for (uint32_t j = 0; j < rdr->getVerticesCount(); ++j)
		{
			if (i == infl[j])
			{
				indRemap[j] = (int32_t)cmesh.vertices.size();
				cmesh.vertices.push_back(SimpleMesh::Vertex());
				cmesh.vertices.back().normal = rdr->getNormalsArray()[j];
				cmesh.vertices.back().position = rdr->getPositionArray()[j];
				cmesh.vertices.back().uv = rdr->getUvArray()[j];	
			}
		}
		for (uint32_t j = 0; j < rdr->getIdicesCount(); j += 3)
		{
			if (i == infl[rdr->getIndexArray()[j]])
			{
				int32_t lind = rdr->getIndexArray()[j + 2];
				cmesh.indices.push_back(indRemap[lind]);
				lind = rdr->getIndexArray()[j + 1];
				cmesh.indices.push_back(indRemap[lind]);
				lind = rdr->getIndexArray()[j];
				cmesh.indices.push_back(indRemap[lind]);
			}
		}

		model->chunks[i].meshes.push_back(Chunk::Mesh());
		model->chunks[i].meshes.back().materialIndex = 0;
		model->chunks[i].meshes.back().mesh = cmesh;

		NVBLAST_FREE(infl);
	}
	return model;
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
			uint32_t materialIndex;
			uint32_t chunkIndex;
			sscanf(pMesh.name.data(), "%d_%d", &chunkIndex, &materialIndex);
			if (model->chunks.size() <= chunkIndex)
			{
				model->chunks.resize(chunkIndex + 1);
			}
			model->chunks[chunkIndex].meshes.push_back(Chunk::Mesh());
			Chunk::Mesh& mesh = model->chunks[chunkIndex].meshes.back();

			mesh.materialIndex = materialIndex;
			SimpleMesh& chunkMesh = mesh.mesh;

			PxVec3 emin(FLT_MAX, FLT_MAX, FLT_MAX);
			PxVec3 emax(FLT_MIN, FLT_MIN, FLT_MIN);



			// create an index buffer
			chunkMesh.indices.resize(pMesh.mesh.indices.size());

			// Check if all faces are triangles
			bool allTriangles = true;
			for (uint32_t i = 0; i < pMesh.mesh.num_vertices.size(); ++i)
			{
				if (pMesh.mesh.num_vertices[i] != 3)
				{
					allTriangles = false;
					break;
				}
			}

			if (pMesh.mesh.indices.size() > 0 && allTriangles)
			{
				for (uint32_t i = 0; i < pMesh.mesh.indices.size(); i += 3)
				{
					chunkMesh.indices[i] = (uint16_t)pMesh.mesh.indices[i + 2];
					chunkMesh.indices[i + 1] = (uint16_t)pMesh.mesh.indices[i + 1];
					chunkMesh.indices[i + 2] = (uint16_t)pMesh.mesh.indices[i];
				}
			}
			// create vertex buffer
			chunkMesh.vertices.resize(pMesh.mesh.positions.size() / 3);
			// copy positions
			uint32_t indexer = 0;
			for (uint32_t i = 0; i < pMesh.mesh.positions.size() / 3; i++)
			{
				chunkMesh.vertices[i].position.x = pMesh.mesh.positions[indexer];
				chunkMesh.vertices[i].position.y = pMesh.mesh.positions[indexer + 1];
				chunkMesh.vertices[i].position.z = pMesh.mesh.positions[indexer + 2];
				indexer += 3;
				// calc min/max
				emin = emin.minimum(chunkMesh.vertices[i].position);
				emax = emax.maximum(chunkMesh.vertices[i].position);
			}

			// copy normals
			if (pMesh.mesh.normals.size() > 0)
			{
				indexer = 0;
				for (uint32_t i = 0; i < pMesh.mesh.normals.size() / 3; i++)
				{
					chunkMesh.vertices[i].normal.x = pMesh.mesh.normals[indexer];
					chunkMesh.vertices[i].normal.y = pMesh.mesh.normals[indexer + 1];
					chunkMesh.vertices[i].normal.z = pMesh.mesh.normals[indexer + 2];

					indexer += 3;
				}
			}

			// copy uv
			if (pMesh.mesh.texcoords.size() > 0)
			{
				indexer = 0;
				for (uint32_t i = 0; i < pMesh.mesh.texcoords.size() / 2; i++)
				{
					chunkMesh.vertices[i].uv.x = pMesh.mesh.texcoords[indexer];
					chunkMesh.vertices[i].uv.y = pMesh.mesh.texcoords[indexer + 1];
					indexer += 2;
				}
			}

			// assign extents
			chunkMesh.extents = (emax - emin) * 0.5f;

			// get the center
			chunkMesh.center = emin + chunkMesh.extents;

		}
	}

	return model;
}
