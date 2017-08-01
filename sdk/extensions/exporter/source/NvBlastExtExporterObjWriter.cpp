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
// Copyright (c) 2017 NVIDIA Corporation. All rights reserved.


#include "NvBlastExtExporterObjWriter.h"
#include <PxVec3.h>
#include <sstream>
#include "NvBlastExtAuthoringTypes.h"
#include "NvBlastExtAuthoringMesh.h"


using namespace physx;
using namespace Nv::Blast;

char* gTexPath = "";

void ObjFileWriter::release()
{
	delete this;
}

bool ObjFileWriter::appendMesh(const AuthoringResult& aResult, const char* /*assetName*/, bool /*nonSkinned*/)
{
	mMeshData = std::shared_ptr<ExporterMeshData>(new ExporterMeshData(), [](ExporterMeshData* md)
	{
		//delete[] md->hulls;
		//delete[] md->hullsOffsets;
		delete[] md->normals;
		//delete[] md->normIndex;
		delete[] md->posIndex;
		delete[] md->positions;
		delete[] md->submeshOffsets;
		//delete[] md->texIndex;
		delete[] md->submeshNames;
		delete[] md->uvs;
		delete md;
	});
	ExporterMeshData& md = *mMeshData.get();
	uint32_t triCount = aResult.geometryOffset[aResult.chunkCount];
	md.meshCount = aResult.chunkCount;
	md.submeshOffsets = new uint32_t[md.meshCount + 1];
	for (uint32_t i = 0; i < md.meshCount + 1; i++)
	{ 
		md.submeshOffsets[i] = aResult.geometryOffset[i] * 3;
	}
	//md.submeshOffsets = md.meshOffsets;
	md.submeshCount = 1;
	//md.indicesCount = triCount * 3;
	md.positionsCount = triCount * 3;
	md.normalsCount = md.positionsCount;
	md.uvsCount = md.positionsCount;
	md.positions = new PxVec3[md.positionsCount];
	md.normals = new PxVec3[md.normalsCount];
	md.uvs = new PxVec2[md.uvsCount];
	md.posIndex = new uint32_t[triCount * 3];
	md.normIndex = md.posIndex;
	md.texIndex = md.posIndex;
	md.submeshNames = new const char*[1]{ gTexPath };
	for (uint32_t vc = 0; vc < triCount; ++vc)
	{
		Triangle& tri = aResult.geometry[vc];
		uint32_t i = vc * 3;
		md.positions[i+0] = tri.a.p;
		md.positions[i+1] = tri.b.p;
		md.positions[i+2] = tri.c.p;

		md.normals[i+0] = tri.a.n;
		md.normals[i+1] = tri.b.n;
		md.normals[i+2] = tri.c.n;
		
		md.uvs[i+0] = tri.a.uv[0];
		md.uvs[i+1] = tri.b.uv[0];
		md.uvs[i+2] = tri.c.uv[0];

		md.posIndex[i + 0] = i + 0;
		md.posIndex[i + 1] = i + 1;
		md.posIndex[i + 2] = i + 2;
	}
	return true;
}

bool ObjFileWriter::appendMesh(const ExporterMeshData& meshData, const char* /*assetName*/, bool /*nonSkinned*/)
{
	mMeshData = std::shared_ptr<ExporterMeshData>(new ExporterMeshData(meshData));
	return true;
}

bool ObjFileWriter::saveToFile(const char* assetName, const char* outputPath)
{
	if (mMeshData.get() == nullptr)
	{
		return false;
	}
	const ExporterMeshData& md = *mMeshData.get();

	uint32_t chunkCount = md.meshCount;

	// export materials (mtl file)
	{
		std::ostringstream mtlFilePath;
		mtlFilePath << outputPath << "\\" << assetName << ".mtl";
		FILE* f = fopen(mtlFilePath.str().c_str(), "w");
		if (!f)
			return false;

		for (uint32_t submeshIndex = 0; submeshIndex < md.submeshCount; ++submeshIndex)
		{
			fprintf(f, "newmtl mat%d\n", submeshIndex);
			fprintf(f, "\tmap_Kd %s\n", md.submeshNames[submeshIndex]);
			fprintf(f, "\n");
		}

		fclose(f);
	}

	/// Export geometry to *.obj file
	{
		std::ostringstream objFilePath;
		objFilePath << outputPath << "\\" << assetName << ".obj";
		FILE* f = fopen(objFilePath.str().c_str(), "w");
		if (!f)
			return false;

		fprintf(f, "mtllib %s.mtl\n", assetName);
		fprintf(f, "o frac \n");


		/// Write compressed vertices
		for (uint32_t i = 0; i < md.positionsCount; ++i)
		{
			fprintf(f, "v %.4f %.4f %.4f\n", md.positions[i].x, md.positions[i].y, md.positions[i].z);
		}
		for (uint32_t i = 0; i < md.normalsCount; ++i)
		{
			fprintf(f, "vn %.4f %.4f %.4f\n", md.normals[i].x, md.normals[i].y, md.normals[i].z);
		}
		for (uint32_t i = 0; i < md.uvsCount; ++i)
		{
			fprintf(f, "vt %.4f %.4f\n", md.uvs[i].x, md.uvs[i].y);
		}

		for (uint32_t chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex)
		{
			for (uint32_t submeshIndex = 0; submeshIndex < md.submeshCount; ++submeshIndex)
			{
				uint32_t firstIdx = md.submeshOffsets[chunkIndex * md.submeshCount + submeshIndex];
				uint32_t lastIdx = md.submeshOffsets[chunkIndex * md.submeshCount + submeshIndex + 1];
				fprintf(f, "g %d_%d \n", chunkIndex, submeshIndex);
				fprintf(f, "usemtl mat%d\n", submeshIndex);

				for (uint32_t i = firstIdx; i < lastIdx; i += 3)
				{
					fprintf(f, "f %d/%d/%d  ", md.posIndex[i] + 1, md.texIndex[i] + 1, md.normIndex[i] + 1);
					fprintf(f, "%d/%d/%d  ", md.posIndex[i + 1] + 1, md.texIndex[i + 1] + 1, md.normIndex[i + 1] + 1);
					fprintf(f, "%d/%d/%d \n", md.posIndex[i + 2] + 1, md.texIndex[i + 2] + 1, md.normIndex[i + 2] + 1);
				}
			}
		}
		fclose(f);
	}
	return true;

}

