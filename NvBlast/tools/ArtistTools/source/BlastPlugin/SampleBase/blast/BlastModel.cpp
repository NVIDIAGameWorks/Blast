/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "BlastModel.h"

// Add By Lixu Begin
//#define TINYOBJLOADER_IMPLEMENTATION
// Add By Lixu End
#include "tiny_obj_loader.h"


using namespace physx;

BlastModelPtr BlastModel::loadFromFileTinyLoader(const char* path)
{
// Add By Lixu Begin
	BlastModel* model = new BlastModel();
// Add By Lixu End

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
// Add By Lixu Begin
			model->materials[i].name = pMaterial->name;
// Add By Lixu End
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
