/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "GlobalSettings.h"
#include "BlastAssetModel.h"
#include "Renderer.h"
#include "BlastController.h"
#include "Utils.h"
#include "PsFileBuffer.h"
#include "NvBlastExtPxAsset.h"
#include <QtCore/QFile>
#include <sstream>


BlastAssetModel::BlastAssetModel(TkFramework& framework, PxPhysics& physics, PxCooking& cooking, Renderer& renderer, const char* modelName, const char* modelPath)
	: BlastAsset(renderer)
{
	// Add By Lixu Begin
	if (modelPath == NULL)
	{
		GlobalSettings& globalSettings = GlobalSettings::Inst();
		modelPath = globalSettings.m_projectFileDir.c_str();
		assert(globalSettings.m_projectFileDir.length() > 0);
	}
	// Physics Asset
	std::string blastFileName = std::string(modelName) + ".bpxa";
	// Add By Lixu Begin
	std::string path = GlobalSettings::MakeFileName(modelPath, blastFileName.c_str());
	if (QFile::exists(path.c_str()))
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
	std::string objFileName = std::string(modelName) + ".obj";
	// Add By Lixu Begin
	path = GlobalSettings::MakeFileName(modelPath, objFileName.c_str());
	if (QFile::exists(path.c_str()))
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
