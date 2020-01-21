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
// Copyright (c) 2016-2018 NVIDIA Corporation. All rights reserved.


#include "ApexDestructibleObjExporter.h"

#include "Log.h"

#include "PsFastXml.h"
#include "PsFileBuffer.h"
#include <PxVec2.h>
#include <PxVec3.h>
#include <map>
#include <NvBlastExtExporter.h>
#include <NvBlastExtAuthoringTypes.h>



using namespace nvidia;
using namespace Nv::Blast;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												Material Parser
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ApexDestructibleGeometryExporter::exportToFile(NvBlastAsset* asset, const NvParameterized::Interface* dmeshIfs, ApexImporter::ApexImportTool& tool, const std::string& name, 
	const std::vector<uint32_t>& chunkReorderInvMap, bool toFbx, bool toObj, bool fbxascii, bool nonSkinned, const std::vector<std::vector<CollisionHull*> >& hulls)
{

	Nv::Blast::ExporterMeshData meshData;
	meshData.asset = asset;
	tool.importRendermesh(chunkReorderInvMap, dmeshIfs, &meshData, m_materialsDir.c_str());

		if (!hulls.empty())
		{
			meshData.hullsOffsets = new uint32_t[hulls.size() + 1]{ 0 };
			for (uint32_t i = 0; i < hulls.size(); i++)
			{
				meshData.hullsOffsets[i + 1] = meshData.hullsOffsets[i] + (uint32_t)hulls[i].size();
			}
			meshData.hulls = new CollisionHull*[meshData.hullsOffsets[hulls.size()]];
			for (uint32_t i = 0; i < hulls.size(); i++)
			{
				for (uint32_t j = 0; j < hulls[i].size(); j++)
				{
					meshData.hulls[meshData.hullsOffsets[i] + j] = hulls[i][j];
				}
			}
		}
		else
		{
			meshData.hulls = nullptr;
			meshData.hullsOffsets = nullptr;
		}
		if (toObj)
		{
			IMeshFileWriter* fileWriter = NvBlastExtExporterCreateObjFileWriter();
			fileWriter->appendMesh(meshData, name.c_str());
			fileWriter->saveToFile(name.c_str(), m_exportDir.c_str());
			fileWriter->release();
			//writer.saveToFile(asset, name, m_exportDir, compressedPositions, compressedNormals, compressedTextures, pInd, nInd, tInd, materialPathes, submeshCount);
		}
		if (toFbx)
		{
			IMeshFileWriter* fileWriter = NvBlastExtExporterCreateFbxFileWriter(fbxascii);
			fileWriter->appendMesh(meshData, name.c_str(), nonSkinned);
			fileWriter->saveToFile(name.c_str(), m_exportDir.c_str());
			fileWriter->release();
		}

		delete[] meshData.hulls;
		delete[] meshData.hullsOffsets;
		delete[] meshData.normals;
		delete[] meshData.normIndex;
		delete[] meshData.posIndex;
		delete[] meshData.positions;
		delete[] meshData.submeshOffsets;
		delete[] meshData.texIndex;

		delete[] meshData.uvs;

		for (uint32_t i = 0; i < meshData.submeshCount; ++i)
		{
			delete[] meshData.submeshMats[i].diffuse_tex;
			delete[] meshData.submeshMats[i].name;
		}
		delete[] meshData.submeshMats;

		return true;
}
