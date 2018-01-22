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
// Copyright (c) 2016-2017 NVIDIA Corporation. All rights reserved.


#include "ApexDestructibleObjExporter.h"

#include "Log.h"

#include "PsFastXml.h"
#include "PsFileBuffer.h"
#include "PxInputDataFromPxFileBuf.h"
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
const float VEC_EPS = 1e-4;

class MaterialXmlParser : public physx::shdfnd::FastXml::Callback
{
public:
	std::string textureFile;

protected:
	// encountered a comment in the XML
	virtual bool processComment(const char* /*comment*/)
	{
		return true;
	}

	virtual bool processClose(const char* /*element*/, unsigned int /*depth*/, bool& /*isError*/)
	{
		return true;
	}

	// return true to continue processing the XML document, false to skip.
	virtual bool processElement(const char* elementName, // name of the element
		const char* elementData, // element data, null if none
		const physx::shdfnd::FastXml::AttributePairs& attr,
		int /*lineno*/) // line number in the source XML file
	{
		PX_UNUSED(attr);
		if (::strcmp(elementName, "sampler2D") == 0)
		{
			int nameIndex = -1;
			for (int i = 0; i < attr.getNbAttr(); i += 2)
			{
				if (::strcmp(attr.getKey(i), "name") == 0)
				{
					nameIndex = i;
					break;
				}
			}

			if (::strcmp(attr.getValue(nameIndex), "diffuseTexture") == 0)
			{
				textureFile = elementData;
			}
		}

		return true;
	}
};

std::string getTextureFromMaterial(const char* materialPath)
{
	PsFileBuffer fileBuffer(materialPath, general_PxIOStream2::PxFileBuf::OPEN_READ_ONLY);
	PxInputDataFromPxFileBuf inputData(fileBuffer);
	MaterialXmlParser parser;
	physx::shdfnd::FastXml* xml = physx::shdfnd::createFastXml(&parser);
	xml->processXml(inputData, false);

	xml->release();

	// trim folders
	std::string textureFile = parser.textureFile.substr(parser.textureFile.find_last_of("/\\") + 1);

	return textureFile;
}


bool ApexDestructibleGeometryExporter::exportToFile(NvBlastAsset* asset, const DestructibleAsset& apexAsset, const std::string& name, 
	const std::vector<uint32_t>& chunkReorderInvMap, bool toFbx, bool toObj, bool fbxascii, bool nonSkinned, const std::vector<std::vector<CollisionHull*> >& hulls)
{
	const RenderMeshAsset* rAsset = apexAsset.getRenderMeshAsset();

	Nv::Blast::ExporterMeshData meshData;
	meshData.asset = asset;
	meshData.submeshCount = rAsset->getSubmeshCount();
	meshData.submeshMats = new Materials[meshData.submeshCount];
	std::vector<std::string> materialPathes;
	materialPathes.reserve(meshData.submeshCount);
	// gather materials
	{
		for (uint32_t submeshIndex = 0; submeshIndex < meshData.submeshCount; ++submeshIndex)
		{
			const char* materialName = rAsset->getMaterialName(submeshIndex);
			std::ostringstream materialPath;
			materialPath << m_materialsDir << "\\" << materialName;
			std::string texturePath = getTextureFromMaterial(materialPath.str().c_str());
			materialPathes.push_back(texturePath);
			meshData.submeshMats[submeshIndex].diffuse_tex = materialPathes[submeshIndex].c_str();
			meshData.submeshMats[submeshIndex].name = materialPathes[submeshIndex].c_str();
		}
	}
	struct vc3Comp
	{
		bool operator()(const PxVec3& a, const PxVec3& b) const
		{
			if (a.x + VEC_EPS < b.x) return true;
			if (a.x - VEC_EPS > b.x) return false;
			if (a.y + VEC_EPS < b.y) return true;
			if (a.y - VEC_EPS > b.y) return false;
			if (a.z + VEC_EPS < b.z) return true;
			return false;
		}
	};
	struct vc2Comp
	{
		bool operator()(const PxVec2& a, const PxVec2& b) const
		{
			if (a.x + VEC_EPS < b.x) return true;
			if (a.x - VEC_EPS > b.x) return false;
			if (a.y + VEC_EPS < b.y) return true;
			return false;
		}
	};

	std::vector<PxVec3> compressedPositions;
	std::vector<PxVec3> compressedNormals;
	std::vector<PxVec2> compressedTextures;

	std::vector<uint32_t> positionsMapping;
	std::vector<uint32_t> normalsMapping;
	std::vector<uint32_t> texturesMapping;

	std::map<PxVec3, uint32_t, vc3Comp> posMap;
	std::map<PxVec3, uint32_t, vc3Comp> normMap;
	std::map<PxVec2, uint32_t, vc2Comp> texMap;


	// gather data for export
	{		
		for (uint32_t submeshIndex = 0; submeshIndex < meshData.submeshCount; ++submeshIndex)
		{
			const RenderSubmesh& currentSubmesh = rAsset->getSubmesh(submeshIndex);
			uint32_t indexCount = currentSubmesh.getVertexBuffer().getVertexCount();
			const VertexFormat& fmt = currentSubmesh.getVertexBuffer().getFormat();
			// Find position buffer index
			uint32_t bufferId = 0;
			{				
				for (; bufferId < fmt.getBufferCount(); ++bufferId)
				{
					if (fmt.getBufferSemantic(bufferId) != RenderVertexSemantic::POSITION)
						continue;
					else
						break;
				}
				if (bufferId == fmt.getBufferCount())
				{
					lout() << "Can't find positions buffer" << std::endl;
					return false;
				}
			}
				const PxVec3* posistions = reinterpret_cast<const PxVec3*>(currentSubmesh.getVertexBuffer().getBuffer(bufferId));
				uint32_t oldSize = (uint32_t)positionsMapping.size();
				positionsMapping.resize(oldSize + indexCount);
				for (uint32_t i = 0; i < indexCount; ++i)
				{
					auto it = posMap.find(posistions[i]);
					if (it == posMap.end())
					{
						posMap[posistions[i]] = (uint32_t)compressedPositions.size();
						positionsMapping[oldSize + i] = (uint32_t)compressedPositions.size();
						compressedPositions.push_back(posistions[i]);
					}
					else
					{
						positionsMapping[oldSize + i] = it->second;
					}
				}			
		}

		for (uint32_t submeshIndex = 0; submeshIndex < meshData.submeshCount; ++submeshIndex)
		{
			const RenderSubmesh& currentSubmesh = rAsset->getSubmesh(submeshIndex);
			uint32_t indexCount = currentSubmesh.getVertexBuffer().getVertexCount();
			const VertexFormat& fmt = currentSubmesh.getVertexBuffer().getFormat();
			// Find normal buffer index
			uint32_t bufferId = 0;
			{
				for (; bufferId < fmt.getBufferCount(); ++bufferId)
				{
					if (fmt.getBufferSemantic(bufferId) != RenderVertexSemantic::NORMAL)
						continue;
					else
						break;
				}
				if (bufferId == fmt.getBufferCount())
				{
					lout() << "Can't find positions buffer" << std::endl;
					return false;
				}
			}
				const PxVec3* normals = reinterpret_cast<const PxVec3*>(currentSubmesh.getVertexBuffer().getBuffer(bufferId));
				uint32_t oldSize = (uint32_t)normalsMapping.size();
				normalsMapping.resize(oldSize + indexCount);
				for (uint32_t i = 0; i < indexCount; ++i)
				{
					auto it = normMap.find(normals[i]);
					if (it == normMap.end())
					{
						normMap[normals[i]] = (uint32_t)compressedNormals.size();
						normalsMapping[oldSize + i] = (uint32_t)compressedNormals.size();
						compressedNormals.push_back(normals[i]);
					}
					else
					{
						normalsMapping[oldSize + i] = it->second;
					}
				}
			
		}
		for (uint32_t submeshIndex = 0; submeshIndex < meshData.submeshCount; ++submeshIndex)
		{
			const RenderSubmesh& currentSubmesh = rAsset->getSubmesh(submeshIndex);
			uint32_t indexCount = currentSubmesh.getVertexBuffer().getVertexCount();
			const VertexFormat& fmt = currentSubmesh.getVertexBuffer().getFormat();

			// Find texture coords buffer index
			uint32_t bufferId = 0;
			{
				for (; bufferId < fmt.getBufferCount(); ++bufferId)
				{
					if (fmt.getBufferSemantic(bufferId) != RenderVertexSemantic::TEXCOORD0)
						continue;
					else
						break;
				}
				if (bufferId == fmt.getBufferCount())
				{
					lout() << "Can't find positions buffer" << std::endl;
					return false;
				}
			}
				const PxVec2* texCoord = reinterpret_cast<const PxVec2*>(currentSubmesh.getVertexBuffer().getBuffer(bufferId));
				uint32_t oldSize = (uint32_t)texturesMapping.size();
				texturesMapping.resize(oldSize + indexCount);
				for (uint32_t i = 0; i < indexCount; ++i)
				{
					auto it = texMap.find(texCoord[i]);
					if (it == texMap.end())
					{
						texMap[texCoord[i]] = (uint32_t)compressedTextures.size();
						texturesMapping[oldSize + i] = (uint32_t)compressedTextures.size();
						compressedTextures.push_back(texCoord[i]);
					}
					else
					{
						texturesMapping[oldSize + i] = it->second;
					}
				}
		}
		for (uint32_t i = 0; i < compressedTextures.size(); ++i)
		{
			std::swap(compressedTextures[i].x, compressedTextures[i].y);
		}


		meshData.positionsCount = (uint32_t)compressedPositions.size();
		//meshData.positions = compressedPositions.data();
		meshData.positions = new PxVec3[meshData.positionsCount];
		memcpy(meshData.positions, compressedPositions.data(), sizeof(PxVec3) * meshData.positionsCount);
		meshData.normalsCount = (uint32_t)compressedNormals.size();
		//meshData.normals = compressedNormals.data();
		meshData.normals = new PxVec3[meshData.normalsCount];
		memcpy(meshData.normals, compressedNormals.data(), sizeof(PxVec3) * meshData.normalsCount);
		meshData.uvsCount = (uint32_t)compressedTextures.size();
		//meshData.uvs = compressedTextures.data();
		meshData.uvs = new PxVec2[meshData.uvsCount];
		memcpy(meshData.uvs, compressedTextures.data(), sizeof(PxVec2) * meshData.uvsCount);

		uint32_t apexChunkCount = apexAsset.getChunkCount();
		meshData.meshCount = (uint32_t)chunkReorderInvMap.size();
		meshData.submeshOffsets = new uint32_t[meshData.meshCount * meshData.submeshCount + 1]{ 0 };
	
		//count total number of indices
		for (uint32_t chunkIndex = 0; chunkIndex < meshData.meshCount; ++chunkIndex)
		{
			uint32_t apexChunkIndex = chunkReorderInvMap[chunkIndex];
			if (apexChunkIndex >= apexChunkCount)
			{
				PX_ALWAYS_ASSERT();
				continue;
			}
			uint32_t part = apexAsset.getPartIndex(apexChunkIndex);
			for (uint32_t submeshIndex = 0; submeshIndex < meshData.submeshCount; ++submeshIndex)
			{
				uint32_t indexCount = rAsset->getSubmesh(submeshIndex).getIndexCount(part);
				uint32_t* firstIdx = meshData.submeshOffsets + chunkIndex * meshData.submeshCount + submeshIndex;
				*(firstIdx + 1) = *firstIdx + indexCount;
			}
		}
		meshData.posIndex = new uint32_t[meshData.submeshOffsets[meshData.meshCount * meshData.submeshCount]];
		meshData.normIndex = new uint32_t[meshData.submeshOffsets[meshData.meshCount * meshData.submeshCount]];
		meshData.texIndex = new uint32_t[meshData.submeshOffsets[meshData.meshCount * meshData.submeshCount]];
		//copy indices
		for (uint32_t chunkIndex = 0; chunkIndex < meshData.meshCount; ++chunkIndex)
		{
			uint32_t apexChunkIndex = chunkReorderInvMap[chunkIndex];
			if (apexChunkIndex >= apexChunkCount)
			{
				PX_ALWAYS_ASSERT();
				continue;
			}
			uint32_t part = apexAsset.getPartIndex(apexChunkIndex);
			uint32_t offset = 0;
			for (uint32_t submeshIndex = 0; submeshIndex < meshData.submeshCount; ++submeshIndex)
			{
				const RenderSubmesh& currentSubmesh = rAsset->getSubmesh(submeshIndex);
				const uint32_t* indexArray = currentSubmesh.getIndexBuffer(part);
				uint32_t indexCount = currentSubmesh.getIndexCount(part);
				uint32_t firstIdx = meshData.submeshOffsets[chunkIndex * meshData.submeshCount + submeshIndex];

				for (uint32_t i = 0; i < indexCount;++i)
				{
					meshData.posIndex[firstIdx + i] = positionsMapping[indexArray[i] + offset];
					meshData.normIndex[firstIdx + i] = normalsMapping[indexArray[i] + offset];
					meshData.texIndex[firstIdx + i] = texturesMapping[indexArray[i] + offset];
				}
				offset += currentSubmesh.getVertexBuffer().getVertexCount();
			}
		}
		
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
		delete[] meshData.submeshMats;
		delete[] meshData.uvs;
	}

	return true;
}
