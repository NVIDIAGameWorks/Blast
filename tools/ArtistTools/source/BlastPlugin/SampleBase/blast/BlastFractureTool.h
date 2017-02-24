/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef BLAST_FRACTURETOOL_H
#define BLAST_FRACTURETOOL_H

#include "NvBlastExtAuthoringFractureTool.h"

class BlastAsset;
namespace Nv
{
	namespace Blast
	{
		class Mesh;
	}
}

class BlastFractureTool : public Nv::Blast::FractureTool
{
public:
	BlastFractureTool(NvBlastLog logCallback = nullptr) : FractureTool(logCallback) {}
	~BlastFractureTool() { free(); }

	void setSourceAsset(const BlastAsset* pBlastAsset);

	Nv::Blast::Mesh* getSourceMesh(int32_t chunkId);

private:
	void free();

	std::vector<Nv::Blast::Mesh*> chunkMeshes;
};

#endif //BLAST_FRACTURETOOL_H