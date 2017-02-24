#include "ApexDestructibleObjExporter.h"

#include "Log.h"

#include "PsFastXml.h"
#include "PsFileBuffer.h"
#include "PxInputDataFromPxFileBuf.h"
#include <PxVec2.h>
#include <PxVec3.h>
#include <map>
#include <FbxFileWriter.h>
#include <ObjFileWriter.h>


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


bool ApexDestructibleGeometryExporter::exportToFile(NvBlastAsset* asset, const DestructibleAsset& apexAsset, const std::string& name, const std::vector<uint32_t>& chunkReorderInvMap, bool toFbx, bool toObj, bool fbxascii, bool toUe4)
{
	const RenderMeshAsset* rAsset = apexAsset.getRenderMeshAsset();
	uint32_t submeshCount = rAsset->getSubmeshCount();	
	std::vector<std::string> materialPathes;
	// gather materials
	{
		for (uint32_t submeshIndex = 0; submeshIndex < submeshCount; ++submeshIndex)
		{
			const char* materialName = rAsset->getMaterialName(submeshIndex);
			std::ostringstream materialPath;
			materialPath << m_materialsDir << "\\" << materialName;
			std::string texturePath = getTextureFromMaterial(materialPath.str().c_str());
			materialPathes.push_back(texturePath);
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
		for (uint32_t submeshIndex = 0; submeshIndex < submeshCount; ++submeshIndex)
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

		for (uint32_t submeshIndex = 0; submeshIndex < submeshCount; ++submeshIndex)
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
		for (uint32_t submeshIndex = 0; submeshIndex < submeshCount; ++submeshIndex)
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

		uint32_t apexChunkCount = apexAsset.getChunkCount();
		uint32_t chunkCount = static_cast<uint32_t>(chunkReorderInvMap.size());

		std::vector<std::vector<std::vector<int32_t> > > pInd(chunkCount);
		std::vector<std::vector<std::vector<int32_t> > > tInd(chunkCount);
		std::vector<std::vector<std::vector<int32_t> > > nInd(chunkCount);


		for (uint32_t chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex)
		{
			uint32_t apexChunkIndex = chunkReorderInvMap[chunkIndex];
			if (apexChunkIndex >= apexChunkCount)
			{
				continue;
			}
			uint32_t part = apexAsset.getPartIndex(apexChunkIndex);
			uint32_t offset = 0;
			for (uint32_t submeshIndex = 0; submeshIndex < submeshCount; ++submeshIndex)
			{
				const RenderSubmesh& currentSubmesh = rAsset->getSubmesh(submeshIndex);
				const uint32_t* indexArray = currentSubmesh.getIndexBuffer(part);
				uint32_t indexCount = currentSubmesh.getIndexCount(part);
				pInd[chunkIndex].push_back(std::vector<int32_t>());
				tInd[chunkIndex].push_back(std::vector<int32_t>());
				nInd[chunkIndex].push_back(std::vector<int32_t>());

				for (uint32_t i = 0; i < indexCount;++i)
				{
					pInd[chunkIndex].back().push_back(positionsMapping[indexArray[i] + offset]);
					tInd[chunkIndex].back().push_back(texturesMapping[indexArray[i] + offset]);
					nInd[chunkIndex].back().push_back(normalsMapping[indexArray[i] + offset]);
				}
				offset += currentSubmesh.getVertexBuffer().getVertexCount();
			}
		}

		if (toObj)
		{
			ObjFileWriter writer;
			writer.saveToFile(asset, name, m_exportDir, compressedPositions, compressedNormals, compressedTextures, pInd, nInd, tInd, materialPathes, submeshCount);
		}
		if (toFbx)
		{
			FbxFileWriter writer;
			writer.bOutputFBXAscii = fbxascii;
			writer.setConvertToUE4(toUe4);
			writer.saveToFile(asset, name, m_exportDir, compressedPositions, compressedNormals, compressedTextures, pInd, nInd, tInd, materialPathes, submeshCount);
		}
	}

	return true;
}
