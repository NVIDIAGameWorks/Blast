/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "BlastAssetModel.h"
#include "Renderer.h"
#include "BlastController.h"
#include "Utils.h"
#include "ResourceManager.h"
#include "PsFileBuffer.h"
#include "NvBlastExtPxAsset.h"
#include <sstream>


BlastAssetModel::BlastAssetModel(TkFramework& framework, PxPhysics& physics, PxCooking& cooking, Renderer& renderer, const char* modelName)
	: BlastAsset(renderer)
{
	ResourceManager& resourceManager = m_renderer.getResourceManager();

	// Physics Asset
	std::ostringstream blastFileName;
	blastFileName << modelName << ".bpxa";
	std::string path;
	if (resourceManager.findFile(blastFileName.str(), path))
	{
		PsFileBuffer fileBuf(path.c_str(), PxFileBuf::OPEN_READ_ONLY);
		m_pxAsset = ExtPxAsset::deserialize(fileBuf, framework, physics);
		ASSERT_PRINT(m_pxAsset != nullptr, "can't load bpxa file");
	}
	else
	{
		ASSERT_PRINT(false, "wrong blastFilename");
	}

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
	else
	{
		ASSERT_PRINT(false, "wrong objFileName");
	}

	validate();
}


BlastAssetModel::~BlastAssetModel()
{
	m_pxAsset->release();
}
