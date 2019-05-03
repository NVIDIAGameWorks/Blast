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


#include "BlastAssetModel.h"
#include "Renderer.h"
#include "BlastController.h"
#include "Utils.h"
#include "ResourceManager.h"
#include "NvBlastExtPxAsset.h"
#include <sstream>
#include <fstream>
#include "NvBlastExtExporter.h"
#include "PxPhysics.h"
#include <NvBlastGlobals.h>
#include "NvBlastExtAssetUtils.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlastTkAsset.h"
#include "NvBlastExtSerialization.h"
#include "NvBlastExtLlSerialization.h"
#include "NvBlastExtTkSerialization.h"
#include "NvBlastExtPxSerialization.h"
#include "NvBlastExtAuthoring.h"
#include "NvBlastExtPxManager.h"
#include "NvBlastExtPxCollisionBuilder.h"
#include "PxCooking.h"

BlastAssetModel::BlastAssetModel(TkFramework& framework, PxPhysics& physics, PxCooking& cooking,
                                 ExtSerialization& serialization, Renderer& renderer, const char* modelName)
: BlastAsset(renderer)
{
	const float unitConversion = 1.f;

	const NvcVec3 inputScale = { unitConversion, unitConversion, unitConversion };

	ResourceManager& resourceManager = m_renderer.getResourceManager();
	std::string path;

	// load obj file
	std::ostringstream objFileName;
	objFileName << modelName << ".obj";

	if (resourceManager.findFile(objFileName.str(), path))
	{
		m_model = BlastModel::loadFromFileTinyLoader(path.c_str());
		if (!m_model)
		{
			ASSERT_PRINT(false, "obj load failed");
		}
	}
	else  // Obj is not found, try FBX
	{
		objFileName.clear();
		objFileName.str("");
		objFileName << modelName << ".fbx";
		if (resourceManager.findFile(objFileName.str(), path))
		{
			m_model = BlastModel::loadFromFbxFile(path.c_str());
			if (!m_model)
			{
				ASSERT_PRINT(false, "fbx load failed");
			}
		}
		else
		{
			ASSERT_PRINT(false, "mesh file not found");
		}
	}

	for (auto& chunk : m_model->chunks)
	{
		for (auto& mesh : chunk.meshes)
		{
			SimpleMesh& smesh = const_cast<SimpleMesh&>(mesh.mesh);
			smesh.center *= unitConversion;
			smesh.extents *= unitConversion;
			for (auto& vertex : smesh.vertices)
			{
				vertex.position *= unitConversion;
			}
		}
	}

	// Physics Asset

	// Read file into buffer
	std::ostringstream blastFileName;
	blastFileName << modelName << ".blast";
	if (resourceManager.findFile(blastFileName.str(), path))
	{
		std::ifstream stream(path.c_str(), std::ios::binary);
		std::streampos size = stream.tellg();
		stream.seekg(0, std::ios::end);
		size = stream.tellg() - size;
		stream.seekg(0, std::ios::beg);
		std::vector<char> buffer(size);
		stream.read(buffer.data(), buffer.size());
		stream.close();
		uint32_t objectTypeID;
		void* asset = serialization.deserializeFromBuffer(buffer.data(), buffer.size(), &objectTypeID);
		if (asset == nullptr)
		{
			ASSERT_PRINT(asset != nullptr, "can't load .blast file.");
		}
		else if (objectTypeID == Nv::Blast::ExtPxObjectTypeID::Asset)
		{
			m_pxAsset              = reinterpret_cast<ExtPxAsset*>(asset);
			const TkAsset& tkAsset = m_pxAsset->getTkAsset();
			NvBlastAsset* llasset  = const_cast<NvBlastAsset*>(tkAsset.getAssetLL());
			NvBlastExtAssetTransformInPlace(llasset, &inputScale, nullptr, nullptr);
			ExtPxSubchunk* subchunks = const_cast<ExtPxSubchunk*>(m_pxAsset->getSubchunks());
			for (uint32_t i = 0; i < m_pxAsset->getSubchunkCount(); ++i)
			{
				subchunks[i].geometry.scale.scale = PxVec3(unitConversion);
			}
		}
		else
		{
			TkAsset* tkAsset = nullptr;
			if (objectTypeID == Nv::Blast::TkObjectTypeID::Asset)
			{
				tkAsset               = reinterpret_cast<TkAsset*>(asset);
				NvBlastAsset* llasset = const_cast<NvBlastAsset*>(tkAsset->getAssetLL());
				NvBlastExtAssetTransformInPlace(llasset, &inputScale, nullptr, nullptr);
			}
			else if (objectTypeID == Nv::Blast::LlObjectTypeID::Asset)
			{
				NvBlastAsset* llasset = reinterpret_cast<NvBlastAsset*>(asset);
				NvBlastExtAssetTransformInPlace(llasset, &inputScale, nullptr, nullptr);
				tkAsset = framework.createAsset(llasset, nullptr, 0, true);
			}
			else
			{
				ASSERT_PRINT(false, ".blast file contains unknown object.");
			}

			if (tkAsset != nullptr)
			{
				std::vector<ExtPxAssetDesc::ChunkDesc> physicsChunks;
				std::vector<std::vector<ExtPxAssetDesc::SubchunkDesc> > physicsSubchunks;
				/**
				Try find FBX and check whether it contains collision geometry.
				*/
				objFileName.str("");
				objFileName << modelName << ".fbx";
				if (resourceManager.findFile(objFileName.str(), path))
				{
					std::shared_ptr<IFbxFileReader> rdr(NvBlastExtExporterCreateFbxFileReader(),
					                                    [](IFbxFileReader* p) { p->release(); });
					rdr->loadFromFile(path.c_str());
					if (rdr->isCollisionLoaded() == 0)
					{
						ASSERT_PRINT(false, "fbx doesn't contain collision geometry");
					}
					uint32_t* hullsOffsets = nullptr;
					CollisionHull** hulls  = nullptr;
					uint32_t meshCount     = rdr->getCollision(hullsOffsets, hulls);

					physicsChunks.resize(meshCount);
					physicsSubchunks.resize(meshCount);

					std::shared_ptr<ExtPxCollisionBuilder> collisionBuilder(
					    ExtPxManager::createCollisionBuilder(physics, cooking),
					    [](Nv::Blast::ExtPxCollisionBuilder* cmb) { cmb->release(); });

					for (uint32_t i = 0; i < meshCount; ++i)
					{
						for (uint32_t sbHulls = hullsOffsets[i]; sbHulls < hullsOffsets[i + 1]; ++sbHulls)
						{
							PxConvexMeshGeometry temp =
							    physx::PxConvexMeshGeometry(collisionBuilder.get()->buildConvexMesh(*hulls[sbHulls]));
							if (temp.isValid())
							{
								physicsSubchunks[i].push_back(ExtPxAssetDesc::SubchunkDesc());
								physicsSubchunks[i].back().geometry  = temp;
								physicsSubchunks[i].back().transform = physx::PxTransform(physx::PxIdentity);
							}
						}
					}
					for (uint32_t i = 0; i < meshCount; ++i)
					{
						physicsChunks[i].isStatic      = false;
						physicsChunks[i].subchunkCount = (uint32_t)physicsSubchunks[i].size();
						physicsChunks[i].subchunks     = physicsSubchunks[i].data();
					}
					if (hulls && hullsOffsets)
					{
						for (uint32_t h = 0; h < hullsOffsets[meshCount]; h++)
						{
							collisionBuilder->releaseCollisionHull(hulls[h]);
						}
						NVBLAST_FREE(hulls);
						NVBLAST_FREE(hullsOffsets);
					}
				}
				m_pxAsset = ExtPxAsset::create(tkAsset, physicsChunks.data(), (uint32_t)physicsChunks.size());
				ASSERT_PRINT(m_pxAsset != nullptr, "can't create asset");
			}
		}
	}

	initialize();
}


BlastAssetModel::~BlastAssetModel()
{
	m_pxAsset->release();
}
