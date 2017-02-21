/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef BLAST_MODEL_H
#define BLAST_MODEL_H

#include "Mesh.h"
#include <vector>
#include <memory>


class BlastModel;
// Add By Lixu Begin
typedef BlastModel* BlastModelPtr;
// Add By Lixu End

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
// Add By Lixu Begin
		std::string name;
// Add By Lixu End
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
private:
	BlastModel() {}
};

#endif // ifndef BLAST_MODEL_H