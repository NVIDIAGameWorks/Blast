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
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.

#include "NvParameterized.h"
#include "XmlSerializer.h"
#include "NsFileBuffer.h"
#include "NvTraits.h"
#include "NsMemoryBuffer.h"

#include "BlastProjectParameters.h"
#include "SimpleScene.h"
#include "ProjectParams.h"

#include "NvErrorCallback.h"
#include "NsGlobals.h"
#include "NsVersionNumber.h"
#include <QtXml\QtXml>
#include <QtWidgets/QMessageBox>
#include "AppMainWindow.h"
#include "FoundationHolder.h"

using namespace nvidia;
using namespace nvidia::parameterized;


struct ProjectParamsContext*	g_projectParamsContext = nullptr;
const char* USER_PRESET_PATH = ".\\BlastUserPreset.userPreset";

struct ProjectParamsContext
{
	//NvFoundation* mFoundation;
	NvParameterized::Traits* mTraits;
	BlastProjectParametersFactory* mBlastProjectParametersFactory;
};

void freeString(NvParameterized::DummyStringStruct& str)
{
	delete[] str.buf;
	str.buf = nullptr;
}

void freeBlast(BPPGraphicsMesh& data)
{
	freeString(data.name);
}

void freeBlast(BPPChunk& data)
{
	freeString(data.name);
	freeString(data.asset);
}

void freeBlast(BPPBond& data)
{
	freeString(data.name);
	freeString(data.asset);
}

void freeBlast(BPPAsset& data)
{
	freeString(data.path);
}

void freeBlast(BPPAssetInstance& data)
{
	freeString(data.name);
	freeString(data.source);
}

void freeBlast(BPPComposite& data)
{
	freeString(data.composite);

	freeBlast(data.blastAssetInstances);
	freeBlast(data.landmarks);
}

void freeBlast(BPPBlast& data)
{
	freeString(data.fileReferences.fbxSourceAsset);
	freeString(data.fileReferences.fbx);
	freeString(data.fileReferences.blast);
	freeString(data.fileReferences.collision);

	freeBlast(data.composite);
	freeBlast(data.blastAssets);
	freeBlast(data.chunks);
	freeBlast(data.bonds);
}

void freeBlast(BPPLandmark& data)
{
	freeString(data.name);
}

void freeBlast(BPPStringArray& data)
{
	for (int i = 0; i < data.arraySizes[0]; ++i)
	{
		freeString(data.buf[i]);
	}

	delete[] data.buf;
	data.buf = nullptr;
	data.arraySizes[0] = 0;
}

void freeBlast(BPPGraphicsMeshArray& data)
{
	for (int i = 0; i < data.arraySizes[0]; ++i)
	{
		freeBlast(data.buf[i]);
	}

	delete[] data.buf;
	data.buf = nullptr;
	data.arraySizes[0] = 0;
}

void freeBlast(BPPChunkArray& data)
{
	for (int i = 0; i < data.arraySizes[0]; ++i)
	{
		freeBlast(data.buf[i]);
	}

	delete[] data.buf;
	data.buf = nullptr;
	data.arraySizes[0] = 0;
}

void freeBlast(BPPBondArray& data)
{
	for (int i = 0; i < data.arraySizes[0]; ++i)
	{
		freeBlast(data.buf[i]);
	}

	delete[] data.buf;
	data.buf = nullptr;
	data.arraySizes[0] = 0;
}

void freeBlast(BPPAssetArray& data)
{
	for (int i = 0; i < data.arraySizes[0]; ++i)
	{
		freeBlast(data.buf[i]);
	}

	delete[] data.buf;
	data.buf = nullptr;
	data.arraySizes[0] = 0;
}

void freeBlast(BPPAssetInstanceArray& data)
{
	for (int i = 0; i < data.arraySizes[0]; ++i)
	{
		freeBlast(data.buf[i]);
	}

	delete[] data.buf;
	data.buf = nullptr;
	data.arraySizes[0] = 0;
}

void freeBlast(BPPLandmarkArray& data)
{
	for (int i = 0; i < data.arraySizes[0]; ++i)
	{
		freeBlast(data.buf[i]);
	}

	delete[] data.buf;
	data.buf = nullptr;
	data.arraySizes[0] = 0;
}

void copy(NvParameterized::DummyStringStruct& dest, const char* source)
{
	delete[] dest.buf;
	dest.buf = nullptr;
	//dest.isAllocated = false;

	if (source != nullptr)
	{
		dest.buf = new char[strlen(source) + 1];
		strcpy(const_cast<char*>(dest.buf), source);
		//dest.isAllocated = true;
	}
}

void copy(NvParameterized::DummyStringStruct& dest, NvParameterized::DummyStringStruct& source)
{
	copy(dest, source.buf);
}

void copy(BPPStringArray& dest, BPPStringArray& source)
{
	{
		for (int i = 0; i < dest.arraySizes[0]; ++i)
		{
			delete[] dest.buf[i].buf;
		}
		delete[] dest.buf;
		dest.buf = nullptr;
		dest.arraySizes[0] = 0;
	}


	if (source.arraySizes[0] > 0)
	{
		dest.buf = new NvParameterized::DummyStringStruct[source.arraySizes[0]];

		for (int i = 0; i < source.arraySizes[0]; ++i)
		{
			NvParameterized::DummyStringStruct& destItem = dest.buf[i];
			NvParameterized::DummyStringStruct& sourceItem = source.buf[i];

			destItem.buf = nullptr;

			copy(destItem, sourceItem);
		}
		dest.arraySizes[0] = source.arraySizes[0];
	}
}

void copy(BPPGraphicsMaterialArray& dest, BPPGraphicsMaterialArray& source)
{
	delete[] dest.buf;
	dest.buf = nullptr;
	dest.arraySizes[0] = 0;

	if (source.arraySizes[0] > 0)
	{
		dest.buf = new BPPGraphicsMaterial[source.arraySizes[0]];
		for (int i = 0; i < source.arraySizes[0]; ++i)
		{
			BPPGraphicsMaterial& destItem = dest.buf[i];
			BPPGraphicsMaterial& sourceItem = source.buf[i];

			init(destItem);

			copy(destItem, sourceItem);
		}
		dest.arraySizes[0] = source.arraySizes[0];
	}
}

void copy(BPPGraphicsMeshArray& dest, BPPGraphicsMeshArray& source)
{
	delete[] dest.buf;
	dest.buf = nullptr;
	dest.arraySizes[0] = 0;

	if (source.arraySizes[0] > 0)
	{
		dest.buf = new BPPGraphicsMesh[source.arraySizes[0]];
		for (int i = 0; i < source.arraySizes[0]; ++i)
		{
			BPPGraphicsMesh& destItem = dest.buf[i];
			BPPGraphicsMesh& sourceItem = source.buf[i];

			destItem.name.buf = nullptr;

			copy(destItem.name, sourceItem.name);
			destItem.visible = sourceItem.visible;
			copy(destItem.materialAssignments, sourceItem.materialAssignments);
		}
		dest.arraySizes[0] = source.arraySizes[0];
	}
}

void copy(BPPBookmarkArray& dest, BPPBookmarkArray& source)
{
	{
		int count = dest.arraySizes[0];
		for (int i = 0; i < count; ++i)
		{
			delete[] dest.buf[i].name.buf;
		}
		delete[] dest.buf;
		dest.buf = nullptr;
		dest.arraySizes[0] = 0;
	}

	{
		int count = source.arraySizes[0];
		dest.arraySizes[0] = count;
		if (count > 0)
		{
			dest.buf = new BPPCameraBookmark[count];
			for (int i = 0; i < count; ++i)
			{
				BPPCameraBookmark& destItem = dest.buf[i];
				BPPCameraBookmark& sourceItem = source.buf[i];

				destItem.name.buf = nullptr;

				copy(destItem.name, sourceItem.name);
				destItem.camera = sourceItem.camera;
			}
		}
	}
}

void copy(BPPLightArray& dest, BPPLightArray& source)
{
	delete[] dest.buf;
	dest.buf = nullptr;
	dest.arraySizes[0] = 0;

	if (source.arraySizes[0] > 0)
	{
		dest.buf = new BPPLight[source.arraySizes[0]];
		for (int i = 0; i < source.arraySizes[0]; ++i)
		{
			BPPLight& destItem = dest.buf[i];
			BPPLight& sourceItem = source.buf[i];

			destItem.name.buf = nullptr;

			copy(destItem.name, sourceItem.name);
			copy(destItem, sourceItem);
		}
		dest.arraySizes[0] = source.arraySizes[0];
	}
}

void copy(BPPChunkArray& dest, BPPChunkArray& source)
{
	freeBlast(dest);

	if (source.arraySizes[0] > 0)
	{
		dest.buf = new BPPChunk[source.arraySizes[0]];
		for (int i = 0; i < source.arraySizes[0]; ++i)
		{
			BPPChunk& destItem = dest.buf[i];
			BPPChunk& sourceItem = source.buf[i];

			destItem.name.buf = nullptr;
			destItem.asset.buf = nullptr;

			copy(destItem, sourceItem);
		}
		dest.arraySizes[0] = source.arraySizes[0];
	}
}

void copy(BPPBondArray& dest, BPPBondArray& source)
{
	freeBlast(dest);

	if (source.arraySizes[0] > 0)
	{
		dest.buf = new BPPBond[source.arraySizes[0]];
		for (int i = 0; i < source.arraySizes[0]; ++i)
		{
			BPPBond& destItem = dest.buf[i];
			BPPBond& sourceItem = source.buf[i];

			destItem.name.buf = nullptr;
			destItem.asset.buf = nullptr;
			destItem.support.healthMask.buf = nullptr;

			copy(destItem, sourceItem);
		}
		dest.arraySizes[0] = source.arraySizes[0];
	}
}

void copy(BPPProjectileArray& dest, BPPProjectileArray& source)
{
	delete[] dest.buf;
	dest.buf = nullptr;
	dest.arraySizes[0] = 0;

	if (source.arraySizes[0] > 0)
	{
		dest.buf = new BPPProjectile[source.arraySizes[0]];
		for (int i = 0; i < source.arraySizes[0]; ++i)
		{
			BPPProjectile& destItem = dest.buf[i];
			BPPProjectile& sourceItem = source.buf[i];

			destItem.name.buf = nullptr;

			copy(destItem, sourceItem);
		}
		dest.arraySizes[0] = source.arraySizes[0];
	}
}

void copy(BPPAssetArray& dest, BPPAssetArray& source)
{
	delete[] dest.buf;
	dest.buf = nullptr;
	dest.arraySizes[0] = 0;

	if (source.arraySizes[0] > 0)
	{
		dest.buf = new BPPAsset[source.arraySizes[0]];
		for (int i = 0; i < source.arraySizes[0]; ++i)
		{
			BPPAsset& destItem = dest.buf[i];
			BPPAsset& sourceItem = source.buf[i];

			destItem.path.buf = nullptr;
			destItem.activePreset.buf = nullptr;

			copy(destItem, sourceItem);
		}
		dest.arraySizes[0] = source.arraySizes[0];
	}
}

void copy(BPPAssetInstanceArray& dest, BPPAssetInstanceArray& source)
{
	delete[] dest.buf;
	dest.buf = nullptr;
	dest.arraySizes[0] = 0;

	if (source.arraySizes[0] > 0)
	{
		dest.buf = new BPPAssetInstance[source.arraySizes[0]];
		for (int i = 0; i < source.arraySizes[0]; ++i)
		{
			BPPAssetInstance& destItem = dest.buf[i];
			BPPAssetInstance& sourceItem = source.buf[i];

			destItem.name.buf = nullptr;
			destItem.source.buf = nullptr;

			copy(destItem, sourceItem);
		}
		dest.arraySizes[0] = source.arraySizes[0];
	}
}

void copy(BPPLandmarkArray& dest, BPPLandmarkArray& source)
{
	delete[] dest.buf;
	dest.buf = nullptr;
	dest.arraySizes[0] = 0;

	if (source.arraySizes[0] > 0)
	{
		dest.buf = new BPPLandmark[source.arraySizes[0]];
		for (int i = 0; i < source.arraySizes[0]; ++i)
		{
			BPPLandmark& destItem = dest.buf[i];
			BPPLandmark& sourceItem = source.buf[i];

			destItem.name.buf = nullptr;

			copy(destItem, sourceItem);
		}
		dest.arraySizes[0] = source.arraySizes[0];
	}
}

void copy(BPPU32Array& dest, BPPU32Array& source)
{
	delete[] dest.buf;
	dest.buf = nullptr;
	dest.arraySizes[0] = 0;

	if (source.arraySizes[0] > 0)
	{
		dest.buf = new uint32_t[source.arraySizes[0]];
		for (int i = 0; i < source.arraySizes[0]; ++i)
		{
			uint32_t& destItem = dest.buf[i];
			uint32_t& sourceItem = source.buf[i];

			destItem = sourceItem;
		}
		dest.arraySizes[0] = source.arraySizes[0];
	}
}

void copy(BPPFilterPresetArray& dest, BPPFilterPresetArray& source)
{
	delete[] dest.buf;
	dest.buf = nullptr;
	dest.arraySizes[0] = 0;

	if (source.arraySizes[0] > 0)
	{
		dest.buf = new BPPFilterPreset[source.arraySizes[0]];
		for (int i = 0; i < source.arraySizes[0]; ++i)
		{
			BPPFilterPreset& destItem = dest.buf[i];
			BPPFilterPreset& sourceItem = source.buf[i];

			destItem.name.buf = nullptr;
			destItem.depthFilters.buf = nullptr;

			copy(destItem, sourceItem);
		}
		dest.arraySizes[0] = source.arraySizes[0];
	}
}

void copy(BPPLight& dest, BPPLight& source)
{
	copy(dest.name, source.name);
	dest.enable = source.enable;
	dest.useShadows = source.useShadows;
	dest.lockToRoot = source.lockToRoot;
	dest.visualize = source.visualize;
	dest.type = source.type;
	dest.shadowMapResolution = source.shadowMapResolution;
	dest.color = source.color;
	dest.diffuseColor = source.diffuseColor;
	dest.ambientColor = source.ambientColor;
	dest.specularColor = source.specularColor;
	dest.intensity = source.intensity;
	dest.distance = source.distance;
	dest.spotFalloffStart = source.spotFalloffStart;
	dest.spotFalloffEnd = source.spotFalloffEnd;
	dest.lightAxisX = source.lightAxisX;
	dest.lightAxisY = source.lightAxisY;
	dest.lightAxisZ = source.lightAxisZ;
	dest.lightPos = source.lightPos;
}

void copy(BPPGraphicsMaterial& dest, BPPGraphicsMaterial& source)
{
	copy(dest.name, source.name);
	dest.useTextures = source.useTextures;
	copy(dest.diffuseTextureFilePath, source.diffuseTextureFilePath);
	copy(dest.specularTextureFilePath, source.specularTextureFilePath);
	copy(dest.normalTextureFilePath, source.normalTextureFilePath);
	dest.diffuseColor = source.diffuseColor;
	dest.specularColor = source.specularColor;
	dest.specularShininess = source.specularShininess;
}

void copy(BPPGraphicsMesh& dest, BPPGraphicsMesh& source)
{
	copy(dest.name, source.name);
	dest.visible = source.visible;
	copy(dest.materialAssignments, source.materialAssignments);
}

void copy(BPPMaterialAssignments& dest, BPPMaterialAssignments& source)
{
	dest.materialIndexes[0] = source.materialIndexes[0];
	dest.materialIndexes[1] = source.materialIndexes[1];
	dest.materialIndexes[2] = source.materialIndexes[2];
	dest.materialIndexes[3] = source.materialIndexes[3];
}

void copy(BPPProjectile& dest, BPPProjectile& source)
{
	copy(dest.name, source.name);
	dest.visible = source.visible;
}

void copy(BPPSupportStructure& dest, BPPSupportStructure& source)
{
	copy(dest.healthMask, source.healthMask);
	dest.bondStrength = source.bondStrength;
	dest.enableJoint = source.enableJoint;
}

void copy(BPPChunk& dest, BPPChunk& source)
{
	dest.ID = source.ID;
	dest.parentID = source.parentID;
	copy(dest.name, source.name);
	copy(dest.asset, source.asset);
	dest.visible = source.visible;
	dest.support = source.support;
	dest.staticFlag = source.staticFlag;
}

void copy(BPPBond& dest, BPPBond& source)
{
	copy(dest.name, source.name);
	copy(dest.asset, source.asset);
	dest.visible = source.visible;
	dest.fromChunk = source.fromChunk;
	dest.toChunk = source.toChunk;
	copy(dest.support, source.support);
}

void copy(BPPLandmark& dest, BPPLandmark& source)
{
	copy(dest.name, source.name);
	dest.visible = source.visible;
	dest.enable = source.enable;
	dest.radius = source.radius;
}

void copy(BPPRenderer& dest, BPPRenderer& source)
{
	dest.renderFps = source.renderFps;
	dest.frameStartTime = source.frameStartTime;
	dest.frameEndTime = source.frameEndTime;
	dest.animationFps = source.animationFps;
	dest.animate = source.animate;
	dest.simulate = source.simulate;
	dest.resetSimulationOnLoop = source.resetSimulationOnLoop;
	dest.simulationFps = source.simulationFps;
	dest.showGraphicsMesh = source.showGraphicsMesh;
	dest.visualizeGrowthMesh = source.visualizeGrowthMesh;
	dest.visualizeLight = source.visualizeLight;
	dest.visualizeWind = source.visualizeWind;
	dest.showStatistics = source.showStatistics;
	dest.renderStyle = source.renderStyle;
	dest.colorizeOption = source.colorizeOption;
	dest.showWireframe = source.showWireframe;
	dest.lockRootBone = source.lockRootBone;
	dest.controlTextureOption = source.controlTextureOption;
	dest.useLighting = source.useLighting;
	dest.showSkinnedMeshOnly = source.showSkinnedMeshOnly;
	dest.lightDir = source.lightDir;
	dest.ambientColor = source.ambientColor;
	dest.windDir = source.windDir;
	dest.windStrength = source.windStrength;
	dest.lightIntensity = source.lightIntensity;
	dest.gravityDir = source.gravityDir;
	dest.gravityScale = source.gravityScale;

	copy(dest.textureFilePath, source.textureFilePath);
	copy(dest.lights, source.lights);
}

void copy(BPPStressSolver& dest, BPPStressSolver& source)
{
	dest.solverMode = source.solverMode;
	dest.linearFactor = source.linearFactor;
	dest.angularFactor = source.angularFactor;
	dest.meanError = source.meanError;
	dest.varianceError = source.varianceError;
	dest.bondsPerFrame = source.bondsPerFrame;
	dest.bondsIterations = source.bondsIterations;
}

void copy(BPPAsset& dest, BPPAsset& source)
{
	copy(dest.path, source.path);
	dest.visible = source.visible;
	dest.stressSolver = source.stressSolver;
	copy(dest.activePreset, source.activePreset);
	dest.defaultDamage = source.defaultDamage;
}

void copy(BPPAssetInstance& dest, BPPAssetInstance& source)
{
	copy(dest.name, source.name);
	dest.visible = source.visible;
	copy(dest.source, source.source);
	dest.transform = source.transform;
}

void copy(BPPComposite& dest, BPPComposite& source)
{
	copy(dest.composite, source.composite);
	dest.visible = source.visible;
	copy(dest.blastAssetInstances, source.blastAssetInstances);
	dest.bondThreshold = source.bondThreshold;
	dest.bondStrength = source.bondStrength;
	copy(dest.landmarks, source.landmarks);
}

void copy(BPPBlast& dest, BPPBlast& source)
{
	copy(dest.fileReferences.fbxSourceAsset, source.fileReferences.fbxSourceAsset);
	copy(dest.fileReferences.fbx, source.fileReferences.fbx);
	copy(dest.fileReferences.blast, source.fileReferences.blast);
	copy(dest.fileReferences.collision, source.fileReferences.collision);

	copy(dest.composite, source.composite);
	copy(dest.blastAssets, source.blastAssets);
	copy(dest.chunks, source.chunks);
	copy(dest.bonds, source.bonds);
	copy(dest.projectiles, source.projectiles);
	copy(dest.graphicsMeshes, source.graphicsMeshes);

	copy(dest.userPreset, source.userPreset);
	copy(dest.healthMask, source.healthMask);
}

void copy(BPPFilter& dest, BPPFilter& source)
{
	dest.activeFilter = source.activeFilter;
	copy(dest.filters, source.filters);
}

void copy(BPPVoronoi& dest, BPPVoronoi& source)
{
	dest.numSites = source.numSites;
	dest.siteGeneration = source.siteGeneration;
	dest.gridSize = source.gridSize;
	dest.gridScale = source.gridScale;
	dest.amplitude = source.amplitude;

	dest.frequency = source.frequency;
	copy(dest.paintMasks, source.paintMasks);
	dest.activePaintMask = source.activePaintMask;
	copy(dest.meshCutters, source.meshCutters);
	dest.activeMeshCutter = source.activeMeshCutter;
	dest.fractureInsideCutter = source.fractureInsideCutter;
	dest.fractureOutsideCutter = source.fractureOutsideCutter;
	copy(dest.textureSites, source.textureSites);
	dest.numTextureSites = source.numTextureSites;
}

void copy(BPPCutoutProjection& dest, BPPCutoutProjection& source)
{
	copy(dest.textures, source.textures);
	dest.cutoutType = source.cutoutType;
	dest.pixelThreshold = source.pixelThreshold;
	dest.tiled = source.tiled;
	dest.invertU = source.invertU;
	dest.invertV = source.invertV;
}

void copy(BPPFracture& dest, BPPFracture& source)
{
	dest.activeFractureMethod = source.activeFractureMethod;
	dest.general = source.general;
	dest.visualization = source.visualization;
	dest.shellCut = source.shellCut;
	copy(dest.voronoi, source.voronoi);
	dest.slice = source.slice;
	copy(dest.cutoutProjection, source.cutoutProjection);
}

void copy(BPPFilterPreset& dest, BPPFilterPreset& source)
{
	copy(dest.name, source.name);
	copy(dest.depthFilters, source.depthFilters);
}

void copy(BPParams& dest, BPParams& source)
{
	dest.camera = source.camera;
	copy(dest.cameraBookmarks, source.cameraBookmarks);
	dest.lightCamera = source.camera;
	dest.windCamera = source.camera;
	copy(dest.graphicsMaterials, source.graphicsMaterials);
	dest.scene = source.scene;
	copy(dest.renderer, source.renderer);
	copy(dest.blast, source.blast);
	copy(dest.filter, source.filter);
	copy(dest.fracture, source.fracture);

	copy(dest.blast.userPreset, USER_PRESET_PATH);
}

void merge(BPPChunkArray& dest, BPPChunkArray& source)
{
	if (source.arraySizes[0] > 0)
	{
		BPPChunk* oriDestArray = dest.buf;
		int oriCount = dest.arraySizes[0];
		int srcCount = source.arraySizes[0];
		dest.buf = new BPPChunk[oriCount + srcCount];
		int i = 0;
		for (; i < oriCount; ++i)
		{
			BPPChunk& destItem = dest.buf[i];
			BPPChunk& oriItem = oriDestArray[i];

			destItem.name.buf = nullptr;
			destItem.asset.buf = nullptr;
			copy(destItem, oriItem);
		}
		for (int j = 0; j < srcCount; ++j, ++i)
		{
			BPPChunk& destItem = dest.buf[i];
			BPPChunk& sourceItem = source.buf[j];

			destItem.name.buf = nullptr;
			destItem.asset.buf = nullptr;
			copy(destItem, sourceItem);
		}
		for (int m = 0; m < oriCount; ++m)
		{
			freeBlast(oriDestArray[m]);
		}
		delete[] oriDestArray;
		dest.arraySizes[0] = oriCount + srcCount;
	}
}

void merge(BPPBondArray& dest, BPPBondArray& source)
{
	if (source.arraySizes[0] > 0)
	{
		BPPBond* oriDestArray = dest.buf;
		int oriCount = dest.arraySizes[0];
		int srcCount = source.arraySizes[0];
		dest.buf = new BPPBond[oriCount + srcCount];
		int i = 0;
		for (; i < oriCount; ++i)
		{
			BPPBond& destItem = dest.buf[i];
			BPPBond& oriItem = oriDestArray[i];

			destItem.name.buf = nullptr;
			destItem.asset.buf = nullptr;
			destItem.support.healthMask.buf = nullptr;
			copy(destItem, oriItem);
		}
		for (int j = 0; j < srcCount; ++j, ++i)
		{
			BPPBond& destItem = dest.buf[i];
			BPPBond& sourceItem = source.buf[j];

			destItem.name.buf = nullptr;
			destItem.asset.buf = nullptr;
			destItem.support.healthMask.buf = nullptr;
			copy(destItem, sourceItem);
		}
		for (int m = 0; m < oriCount; ++m)
		{
			freeBlast(oriDestArray[m]);
		}
		delete[] oriDestArray;
		dest.arraySizes[0] = oriCount + srcCount;
	}
}

void init(BPPStressSolver& param)
{
	param.solverMode = -1;
	param.linearFactor = 0;
	param.angularFactor = 0;
	param.meanError = 0;
	param.varianceError = 0;
	param.bondsPerFrame = 0;
	param.bondsIterations = 0;
}

void init(BPPGraphicsMaterial& param)
{
	param.name.buf = nullptr;
	param.useTextures = false;
	param.diffuseTextureFilePath.buf = nullptr;
	param.specularTextureFilePath.buf = nullptr;
	param.normalTextureFilePath.buf = nullptr;
	param.specularShininess = 0.0;
}

void init(BPParams& params)
{
	//memset(&params, sizeof(BPParams), 0);

	//params.cameraBookmarks.buf = nullptr;
	//for (int i = 0; i < 4; ++i)
	//	params.renderer.lights.buf[i].name.buf = 
}

StressSolverUserPreset::StressSolverUserPreset(const char* inName)
	: name(inName)
{
	name = name;
	stressSolver.solverMode = -1;
	stressSolver.linearFactor = 0;
	stressSolver.angularFactor = 0;
	stressSolver.meanError = 0;
	stressSolver.varianceError = 0;
	stressSolver.bondsPerFrame = 0;
	stressSolver.bondsIterations = 0;
}

BlastProject& BlastProject::ins()
{
	static BlastProject _ins;
	return _ins;
}

BlastProject::~BlastProject()
{
	delete[] _projectParams.cameraBookmarks.buf;
}

void BlastProject::clear()
{
	freeBlast(_projectParams.blast);
}

bool BlastProject::isGraphicsMaterialNameExist(const char* name)
{
	if (name == nullptr || strlen(name) == 0)
		return false;

	BPPGraphicsMaterialArray& theArray = _projectParams.graphicsMaterials;

	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		BPPGraphicsMaterial& item = theArray.buf[i];
		if (nvidia::shdfnd::strcmp(item.name.buf, name) == 0)
			return true;
	}
	return false;
}

BPPGraphicsMaterial* BlastProject::addGraphicsMaterial(const char* name, const char* diffuseTexture)
{
	if (name == nullptr || strlen(name) == 0)
		return nullptr;

	BPPGraphicsMaterialArray& theArray = _projectParams.graphicsMaterials;
	BPPGraphicsMaterial* oldBuf = theArray.buf;
	theArray.buf = new BPPGraphicsMaterial[theArray.arraySizes[0] + 1];

	int i = 0;
	for (; i < theArray.arraySizes[0]; ++i)
	{
		BPPGraphicsMaterial& newItem = theArray.buf[i];
		BPPGraphicsMaterial& oldItem = oldBuf[i];
		newItem.useTextures = false;
		newItem.name.buf = nullptr;
		newItem.diffuseTextureFilePath.buf = nullptr;
		newItem.specularTextureFilePath.buf = nullptr;
		newItem.normalTextureFilePath.buf = nullptr;
		newItem.specularShininess = 0.0;
		copy(newItem, oldItem);
	}

	BPPGraphicsMaterial& newItem = theArray.buf[i];
	newItem.name.buf = nullptr;
	newItem.useTextures = false;
	newItem.diffuseTextureFilePath.buf = nullptr;
	newItem.specularTextureFilePath.buf = nullptr;
	newItem.normalTextureFilePath.buf = nullptr;
	newItem.specularShininess = 0.0;
	copy(newItem.name, name);
	if (diffuseTexture != nullptr)
	{
		copy(newItem.diffuseTextureFilePath, diffuseTexture);
	}
	theArray.arraySizes[0] += 1;

	delete[] oldBuf;

	return &newItem;
}

void BlastProject::removeGraphicsMaterial(const char* name)
{
	if (name == nullptr || strlen(name) == 0 || !isGraphicsMaterialNameExist(name))
		return;

	BPPGraphicsMaterialArray& theArray = _projectParams.graphicsMaterials;
	BPPGraphicsMaterial* oldBuf = theArray.buf;

	theArray.buf = new BPPGraphicsMaterial[theArray.arraySizes[0] - 1];
	int index = 0;
	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		if (nvidia::shdfnd::strcmp(oldBuf[i].name.buf, name) != 0)
		{
			BPPGraphicsMaterial& newItem = theArray.buf[index++];
			BPPGraphicsMaterial& oldItem = oldBuf[i];
			newItem.useTextures = false;
			newItem.name.buf = nullptr;
			newItem.diffuseTextureFilePath.buf = nullptr;
			newItem.specularTextureFilePath.buf = nullptr;
			newItem.normalTextureFilePath.buf = nullptr;
			copy(newItem, oldItem);
		}
	}
	theArray.arraySizes[0] -= 1;
	delete[] oldBuf;
}

void BlastProject::renameGraphicsMaterial(const char* oldName, const char* newName)
{
	if (oldName == nullptr || newName == nullptr)
		return;

	BPPGraphicsMaterialArray& theArray = _projectParams.graphicsMaterials;

	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		if (nvidia::shdfnd::strcmp(theArray.buf[i].name.buf, oldName) == 0)
		{
			copy(theArray.buf[i].name, newName);
			return;
		}
	}
}

std::vector<BPPAsset*> BlastProject::getSelectedBlastAssets(void)
{
	std::vector<BPPAsset*> assets;
	return assets;
}

bool BlastProject::isAssetInstanceNameExist(const char* name)
{
	if (name == nullptr || strlen(name) == 0)
		return false;

	BPPAssetInstanceArray& theArray = _projectParams.blast.composite.blastAssetInstances;

	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		BPPAssetInstance& item = theArray.buf[i];
		if (nvidia::shdfnd::strcmp(item.name.buf, name) == 0)
			return true;
	}
	return false;
}

BPPAssetInstance* BlastProject::getAssetInstance(const char* assetPath, int instanceIndex)
{
	if (assetPath == nullptr || strlen(assetPath) == 0 || instanceIndex < 0)
	{
		return nullptr;
	}

	BPPAssetInstanceArray& instanceArray = _projectParams.blast.composite.blastAssetInstances;
	if (instanceIndex < instanceArray.arraySizes[0])
	{
		BPPAssetInstance& assetInstance = instanceArray.buf[instanceIndex];
		if (nvidia::shdfnd::strcmp(assetPath, assetInstance.source.buf) == 0)
			return &assetInstance;
	}

	return nullptr;
}

BPPAssetInstance* BlastProject::addAssetInstance(int blastAssetIndex, const char* instanceName)
{
	if (instanceName == nullptr)
		return nullptr;

	BPPAssetArray& assetArray = _projectParams.blast.blastAssets;
	if (blastAssetIndex < 0 && blastAssetIndex > assetArray.arraySizes[0])
		return nullptr;

	BPPComposite& composite = _projectParams.blast.composite;
	BPPAssetInstanceArray& theArray = composite.blastAssetInstances;

	BPPAssetInstance* oldBuf = theArray.buf;
	theArray.buf = new BPPAssetInstance[theArray.arraySizes[0] + 1];

	int i = 0;
	for (; i < theArray.arraySizes[0]; ++i)
	{
		BPPAssetInstance& newItem = theArray.buf[i];
		BPPAssetInstance& oldItem = oldBuf[i];

		newItem.name.buf = nullptr;
		newItem.source.buf = nullptr;
		copy(newItem, oldItem);
	}

	BPPAssetInstance& newItem = theArray.buf[i];
	newItem.name.buf = nullptr;
	newItem.source.buf = nullptr;
	copy(newItem.name, instanceName);
	copy(newItem.source, assetArray.buf[blastAssetIndex].path);
	newItem.visible = true;

	delete[] oldBuf;

	return &newItem;
}

void BlastProject::removeAssetInstance(const char* name)
{
	if (name == nullptr || strlen(name) == 0 || !isAssetInstanceNameExist(name))
		return;

	BPPAssetInstanceArray& theArray = _projectParams.blast.composite.blastAssetInstances;
	BPPAssetInstance* oldBuf = theArray.buf;

	theArray.buf = new BPPAssetInstance[theArray.arraySizes[0] - 1];
	int index = 0;
	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		if (nvidia::shdfnd::strcmp(oldBuf[i].name.buf, name) != 0)
		{
			BPPAssetInstance& newItem = theArray.buf[index++];
			BPPAssetInstance& oldItem = oldBuf[i];
			newItem.name.buf = nullptr;
			newItem.source.buf = nullptr;
			copy(newItem, oldItem);
		}
	}
	theArray.arraySizes[0] -= 1;
	delete[] oldBuf;
}

BPPChunk* BlastProject::getChunk(BPPAsset& asset, int id)
{
	BPPChunkArray& chunkArray = _projectParams.blast.chunks;

	int count = chunkArray.arraySizes[0];
	for (int i = 0; i < count; ++i)
	{
		BPPChunk& chunk = chunkArray.buf[i];
		if (chunk.ID == id && (nvidia::shdfnd::strcmp(chunk.asset.buf, asset.path.buf) == 0))
			return &chunk;
	}

	return nullptr;
}

std::vector<BPPChunk*> BlastProject::getChildrenChunks(BPPAsset& asset, int parentID)
{
	std::vector<BPPChunk*> chunks;

	BPPChunkArray& chunkArray = _projectParams.blast.chunks;

	int count = chunkArray.arraySizes[0];
	for (int i = 0; i < count; ++i)
	{
		BPPChunk& chunk = chunkArray.buf[i];
		if (chunk.parentID == parentID && (nvidia::shdfnd::strcmp(chunk.asset.buf, asset.path.buf) == 0))
			chunks.push_back(&chunk);
	}

	return chunks;
}

std::vector<BPPChunk*> BlastProject::getChildrenChunks(BPPAsset& asset)
{
	std::vector<BPPChunk*> chunks;

	BPPChunkArray& chunkArray = _projectParams.blast.chunks;

	int count = chunkArray.arraySizes[0];
	for (int i = 0; i < count; ++i)
	{
		BPPChunk& chunk = chunkArray.buf[i];
		if (nvidia::shdfnd::strcmp(chunk.asset.buf, asset.path.buf) == 0)
			chunks.push_back(&chunk);
	}

	return chunks;
}

std::vector<BPPBond*> BlastProject::getBondsByChunk(BPPAsset& asset, int chunkID)
{
	std::vector<BPPBond*> bonds;
	BPPBondArray& bondArray = _projectParams.blast.bonds;
	int count = bondArray.arraySizes[0];
	for (int i = 0; i < count; ++i)
	{
		BPPBond&  bond = bondArray.buf[i];
		if ((nvidia::shdfnd::strcmp(bond.asset.buf, asset.path.buf) == 0))
		{
			if (bond.fromChunk == chunkID)
				bonds.push_back(&bond);
			else if (bond.toChunk == chunkID)
				bonds.push_back(&bond);
		}
	}

	return bonds;
}

bool BlastProject::isLandmarkNameExist(const char* name)
{
	if (name == nullptr || strlen(name) == 0)
		return false;

	BPPLandmarkArray& theArray = _projectParams.blast.composite.landmarks;

	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		BPPLandmark& item = theArray.buf[i];
		if (nvidia::shdfnd::strcmp(item.name.buf, name) == 0)
			return true;
	}
	return false;
}

BPPLandmark* BlastProject::addLandmark(const char* name)
{
	if (name == nullptr)
		return nullptr;

	BPPLandmarkArray& theArray = _projectParams.blast.composite.landmarks;
	BPPLandmark* oldBuf = theArray.buf;
	theArray.buf = new BPPLandmark[theArray.arraySizes[0] + 1];

	int i = 0;
	for (; i < theArray.arraySizes[0]; ++i)
	{
		BPPLandmark& newItem = theArray.buf[i];
		BPPLandmark& oldItem = oldBuf[i];

		newItem.name.buf = nullptr;
		copy(newItem, oldItem);
	}

	BPPLandmark& newItem = theArray.buf[i];
	newItem.name.buf = nullptr;
	copy(newItem.name, name);
	newItem.visible = true;
	newItem.enable = true;
	newItem.radius = 0.0;
	theArray.arraySizes[0] += 1;

	delete[] oldBuf;

	return &newItem;
}

void BlastProject::removeLandmark(const char* name)
{
	if (name == nullptr || strlen(name) == 0 || !isLandmarkNameExist(name))
		return ;

	BPPLandmarkArray& theArray = _projectParams.blast.composite.landmarks;
	BPPLandmark* oldBuf = theArray.buf;
	
	theArray.buf = new BPPLandmark[theArray.arraySizes[0] - 1];
	int index = 0;
	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		if (nvidia::shdfnd::strcmp(oldBuf[i].name.buf, name) != 0)
		{
			BPPLandmark& newItem = theArray.buf[index++];
			BPPLandmark& oldItem = oldBuf[i];
			newItem.name.buf = nullptr;
			copy(newItem, oldItem);
		}
	}
	theArray.arraySizes[0] -= 1;
	delete[] oldBuf;
}

BPPLandmark* BlastProject::getLandmark(const char* name)
{
	if (name == nullptr)
		return nullptr;

	BPPLandmarkArray& theArray = _projectParams.blast.composite.landmarks;

	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		if (nvidia::shdfnd::strcmp(theArray.buf[i].name.buf, name) == 0)
			return &theArray.buf[i];
	}

	return nullptr;
}

void BlastProject::renameLandmark(const char* oldName, const char* newName)
{
	if (oldName == nullptr || newName == nullptr)
		return ;

	BPPLandmarkArray& theArray = _projectParams.blast.composite.landmarks;

	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		if (nvidia::shdfnd::strcmp(theArray.buf[i].name.buf, oldName) == 0)
		{
			copy(theArray.buf[i].name, newName);
			return;
		}
	}
}

bool BlastProject::isUserPresetNameExist(const char* name)
{
	if (name == nullptr || strlen(name) == 0)
		return false;

	for (size_t i = 0; i < _userPresets.size(); ++i)
	{
		StressSolverUserPreset& item = _userPresets[i];
		if (item.name == name)
			return true;
	}

	return false;
}

std::vector<StressSolverUserPreset>& BlastProject::getUserPresets()
{
	return _userPresets;
}

void BlastProject::addUserPreset(const char* name)
{
	_userPresets.push_back(StressSolverUserPreset(name));
}

void BlastProject::saveUserPreset()
{
	QFile file(_projectParams.blast.userPreset.buf);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		return;
	}
	QTextStream out(&file);

	QDomDocument    xmlDoc;
	QDomElement rootElm = xmlDoc.createElement(QObject::tr("UserPreSet"));
	xmlDoc.appendChild(rootElm);

	for (size_t i = 0; i < _userPresets.size(); ++i)
	{
		_saveStressSolverPreset(rootElm, _userPresets[i]);
	}

	// 4 is count of indent
	xmlDoc.save(out, 4);
}

void BlastProject::loadUserPreset()
{
	QFile file(_projectParams.blast.userPreset.buf);

	if (!file.open(QIODevice::ReadOnly))
	{
		return;
	}

	QDomDocument	xmlDoc;
	if (!xmlDoc.setContent(&file))
	{
		file.close();
		return;
	}
	file.close();

	if (xmlDoc.isNull() || xmlDoc.documentElement().tagName() != QObject::tr("UserPreSet"))
	{
		QMessageBox::warning(&AppMainWindow::Inst(), QObject::tr("Warning"), QObject::tr("The file you selected is empty or not a blast user preset file."));
		return;
	}

	QDomNodeList elms = xmlDoc.documentElement().elementsByTagName(QObject::tr("StressSolverPreset"));
	for (int i = 0; i < elms.count(); ++i)
	{
		StressSolverUserPreset preset("");
		_userPresets.push_back(preset);
		_loadStressSolverPreset(elms.at(i).toElement(), _userPresets[i]);
	}

}

bool BlastProject::isFilterPresetNameExist(const char* name)
{
	if (name == nullptr || strlen(name) == 0)
		return false;

	BPPFilterPresetArray& theArray = _projectParams.filter.filters;

	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		BPPFilterPreset& item = theArray.buf[i];
		if (nvidia::shdfnd::strcmp(item.name.buf, name) == 0)
			return true;
	}
	return false;
}

std::vector<BPPFilterPreset*> BlastProject::getFilterPresets()
{
	std::vector<BPPFilterPreset*> presets;
	return presets;
}

void BlastProject::addFilterPreset(const char* name)
{
	if (name == nullptr)
		return;

	BPPFilterPresetArray& theArray = _projectParams.filter.filters;
	BPPFilterPreset* oldBuf = theArray.buf;
	theArray.buf = new BPPFilterPreset[theArray.arraySizes[0] + 1];

	int i = 0;
	for (; i < theArray.arraySizes[0]; ++i)
	{
		BPPFilterPreset& newItem = theArray.buf[i];
		BPPFilterPreset& oldItem = oldBuf[i];

		newItem.name.buf = nullptr;
		newItem.depthFilters.buf = nullptr;
		newItem.depthFilters.arraySizes[0] = 0;
		copy(newItem, oldItem);
	}

	BPPFilterPreset& newItem = theArray.buf[i];
	newItem.name.buf = nullptr;
	newItem.depthFilters.buf = nullptr;
	newItem.depthFilters.arraySizes[0] = 0;
	copy(newItem.name, name);
	theArray.arraySizes[0] += 1;

	delete[] oldBuf;
}

void BlastProject::removeFilterPreset(const char* name)
{
	if (name == nullptr || strlen(name) == 0 || !isFilterPresetNameExist(name))
		return;

	BPPFilterPresetArray& theArray = _projectParams.filter.filters;
	BPPFilterPreset* oldBuf = theArray.buf;

	theArray.buf = new BPPFilterPreset[theArray.arraySizes[0] - 1];
	int index = 0;
	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		if (nvidia::shdfnd::strcmp(oldBuf[i].name.buf, name) != 0)
		{
			BPPFilterPreset& newItem = theArray.buf[index++];
			BPPFilterPreset& oldItem = oldBuf[i];
			newItem.name.buf = nullptr;
			newItem.depthFilters.buf = nullptr;
			newItem.depthFilters.arraySizes[0] = 0;
			copy(newItem, oldItem);
		}
	}
	theArray.arraySizes[0] -= 1;
	delete[] oldBuf;
}

BPPFilterPreset* BlastProject::getFilterPreset(const char* name)
{
	if (name == nullptr)
		return nullptr;

	BPPFilterPresetArray& theArray = _projectParams.filter.filters;

	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		if (nvidia::shdfnd::strcmp(theArray.buf[i].name.buf, name) == 0)
			return &theArray.buf[i];
	}

	return nullptr;
}

void BlastProject::renameFilterPreset(const char* oldName, const char* newName)
{
	if (oldName == nullptr || newName == nullptr)
		return;

	BPPFilterPresetArray& theArray = _projectParams.filter.filters;

	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		if (nvidia::shdfnd::strcmp(theArray.buf[i].name.buf, oldName) == 0)
		{
			copy(theArray.buf[i].name, newName);
			return;
		}
	}
}

void BlastProject::addFilterDepth(const char* filterName, int depth)
{
	if (filterName == nullptr || depth < 0)
		return;

	BPPFilterPresetArray& theArray = _projectParams.filter.filters;

	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		if (nvidia::shdfnd::strcmp(theArray.buf[i].name.buf, filterName) == 0)
		{
			BPPU32Array& depthArray = theArray.buf[i].depthFilters;
			for (int j = 0; j < depthArray.arraySizes[0]; ++j)
			{
				if (depthArray.buf[j] == depth)
					return;
			}

			uint32_t* oldBuf = depthArray.buf;
			depthArray.buf = new uint32_t[theArray.arraySizes[0] + 1];

			int m = 0, n = 0;
			for (; n < depthArray.arraySizes[0];)
			{
				if (oldBuf[n] < depth)
				{
					depthArray.buf[m++] = oldBuf[n++];
				}
				else
				{
					if (m == n)
						depthArray.buf[m++] = depth;
					else
						depthArray.buf[m++] = oldBuf[n++];
				}
			}

			if (m == n)
			{
				depthArray.buf[m] = depth;
			}
			depthArray.arraySizes[0] += 1;
			return;
		}
	}
}

void BlastProject::removeFilterDepth(const char* filterName, int depth)
{
	if (filterName == nullptr || depth < 0)
		return;

	BPPFilterPresetArray& theArray = _projectParams.filter.filters;

	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		if (nvidia::shdfnd::strcmp(theArray.buf[i].name.buf, filterName) == 0)
		{
			bool foundDepth = false;
			BPPU32Array& depthArray = theArray.buf[i].depthFilters;
			for (int j = 0; j < depthArray.arraySizes[0]; ++j)
			{
				if (depthArray.buf[j] == depth)
				{
					foundDepth = true;
					break;
				}
			}

			if (!foundDepth)
				return;

			uint32_t* oldBuf = depthArray.buf;
			depthArray.buf = new uint32_t[theArray.arraySizes[0] - 1];

			int m = 0, n = 0;
			for (; n < depthArray.arraySizes[0];)
			{
				if (oldBuf[n] != depth)
				{
					depthArray.buf[m++] = oldBuf[n++];
				}
				else
				{
					depthArray.buf[m] = depthArray.buf[n++];
				}
			}

			depthArray.arraySizes[0] -= 1;
			return;
		}
	}
}

bool BlastProject::isCutoutTextureNameExist(const char* name)
{
	if (name == nullptr || strlen(name) == 0)
		return false;

	BPPStringArray& theArray = BlastProject::ins().getParams().fracture.cutoutProjection.textures;

	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		NvParameterized::DummyStringStruct& item = theArray.buf[i];
		if (nvidia::shdfnd::strcmp(item.buf, name) == 0)
			return true;
	}
	return false;
}

void BlastProject::addCutoutTexture(const char* name)
{
	BPPStringArray& theArray = BlastProject::ins().getParams().fracture.cutoutProjection.textures;
	_addStringItem(theArray, name);
}

void BlastProject::removeCutoutTexture(const char* name)
{
	if (name == nullptr || strlen(name) == 0 || !isCutoutTextureNameExist(name))
		return;

	BPPStringArray& theArray = BlastProject::ins().getParams().fracture.cutoutProjection.textures;
	_removeStringItem(theArray, name);
}

bool BlastProject::isPaintMaskNameExist(const char* name)
{
	if (name == nullptr || strlen(name) == 0)
		return false;

	BPPStringArray& theArray = BlastProject::ins().getParams().fracture.voronoi.paintMasks;

	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		NvParameterized::DummyStringStruct& item = theArray.buf[i];
		if (nvidia::shdfnd::strcmp(item.buf, name) == 0)
			return true;
	}
	return false;
}

void BlastProject::addPaintMasks(const char* name)
{
	BPPStringArray& theArray = BlastProject::ins().getParams().fracture.voronoi.paintMasks;
	_addStringItem(theArray, name);
}

void BlastProject::removePaintMasks(const char* name)
{
	if (name == nullptr || strlen(name) == 0 || !isPaintMaskNameExist(name))
		return;

	BPPStringArray& theArray = BlastProject::ins().getParams().fracture.voronoi.paintMasks;
	_removeStringItem(theArray, name);
}

bool BlastProject::isMeshCutterNameExist(const char* name)
{
	if (name == nullptr || strlen(name) == 0)
		return false;

	BPPStringArray& theArray = BlastProject::ins().getParams().fracture.voronoi.meshCutters;

	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		NvParameterized::DummyStringStruct& item = theArray.buf[i];
		if (nvidia::shdfnd::strcmp(item.buf, name) == 0)
			return true;
	}
	return false;
}

void BlastProject::addMeshCutter(const char* name)
{
	BPPStringArray& theArray = BlastProject::ins().getParams().fracture.voronoi.meshCutters;
	_addStringItem(theArray, name);
}

void BlastProject::removeMeshCutter(const char* name)
{
	if (name == nullptr || strlen(name) == 0 || !isMeshCutterNameExist(name))
		return;

	BPPStringArray& theArray = BlastProject::ins().getParams().fracture.voronoi.meshCutters;
	_removeStringItem(theArray, name);
}

bool BlastProject::isVoronoiTextureNameExist(const char* name)
{
	if (name == nullptr || strlen(name) == 0)
		return false;

	BPPStringArray& theArray = BlastProject::ins().getParams().fracture.voronoi.textureSites;

	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		NvParameterized::DummyStringStruct& item = theArray.buf[i];
		if (nvidia::shdfnd::strcmp(item.buf, name) == 0)
			return true;
	}
	return false;
}

void BlastProject::addVoronoiTexture(const char* name)
{
	BPPStringArray& theArray = BlastProject::ins().getParams().fracture.voronoi.textureSites;
	_addStringItem(theArray, name);
}

void BlastProject::removeVoronoiTexture(const char* name)
{
	if (name == nullptr || strlen(name) == 0 || !isVoronoiTextureNameExist(name))
		return;

	BPPStringArray& theArray = BlastProject::ins().getParams().fracture.voronoi.textureSites;
	_removeStringItem(theArray, name);
}

BlastProject::BlastProject()
{
	_projectParams.cameraBookmarks.buf = nullptr;
	_projectParams.cameraBookmarks.arraySizes[0] = 0;

	_projectParams.graphicsMaterials.buf = nullptr;
	_projectParams.graphicsMaterials.arraySizes[0] = 0;

	_projectParams.renderer.textureFilePath.buf = nullptr;

	_projectParams.renderer.lights.buf = new BPPLight[4];
	_projectParams.renderer.lights.arraySizes[0] = 4;

	for (int i = 0; i < 4; ++i)
	{
		_projectParams.renderer.lights.buf[i].name.buf = nullptr;
	}

	_projectParams.blast.fileReferences.blast.buf = nullptr;
	_projectParams.blast.fileReferences.fbx.buf = nullptr;
	_projectParams.blast.fileReferences.fbxSourceAsset.buf = nullptr;
	_projectParams.blast.fileReferences.collision.buf = nullptr;

	_projectParams.blast.composite.composite.buf = nullptr;
	_projectParams.blast.composite.blastAssetInstances.buf = nullptr;
	_projectParams.blast.composite.blastAssetInstances.arraySizes[0] = 0;

	_projectParams.blast.composite.landmarks.buf = nullptr;
	_projectParams.blast.composite.landmarks.arraySizes[0] = 0;

	_projectParams.blast.blastAssets.buf = nullptr;
	_projectParams.blast.blastAssets.arraySizes[0] = 0;

	_projectParams.blast.projectiles.buf = nullptr;
	_projectParams.blast.projectiles.arraySizes[0] = 0;

	_projectParams.blast.graphicsMeshes.buf = nullptr;
	_projectParams.blast.graphicsMeshes.arraySizes[0] = 0;

	_projectParams.blast.userPreset.buf = nullptr;
	_projectParams.blast.healthMask.buf = nullptr;

	_projectParams.filter.filters.buf = nullptr;
	_projectParams.filter.filters.arraySizes[0] = 0;

	_projectParams.fracture.voronoi.paintMasks.buf = nullptr;
	_projectParams.fracture.voronoi.paintMasks.arraySizes[0] = 0;

	_projectParams.fracture.voronoi.meshCutters.buf = nullptr;
	_projectParams.fracture.voronoi.meshCutters.arraySizes[0] = 0;

	_projectParams.fracture.voronoi.textureSites.buf = nullptr;
	_projectParams.fracture.voronoi.textureSites.arraySizes[0] = 0;

	_projectParams.fracture.cutoutProjection.textures.buf = nullptr;
	_projectParams.fracture.cutoutProjection.textures.arraySizes[0] = 0;
}

void BlastProject::_saveStressSolverPreset(QDomElement& parentElm, StressSolverUserPreset& stressSolverUserPreset)
{
	QDomElement newElm = parentElm.ownerDocument().createElement(QObject::tr("StressSolverPreset"));
	parentElm.appendChild(newElm);
	newElm.setAttribute(QObject::tr("Name"), stressSolverUserPreset.name.c_str());
	_saveStressSolver(newElm, stressSolverUserPreset.stressSolver);
}

void BlastProject::_saveStressSolver(QDomElement& parentElm, BPPStressSolver& stressSolver)
{
	QDomElement newElm = parentElm.ownerDocument().createElement(QObject::tr("StressSolver"));
	parentElm.appendChild(newElm);
	newElm.setAttribute(QObject::tr("SolverMode"), stressSolver.solverMode);
	newElm.setAttribute(QObject::tr("LinearFactor"), stressSolver.linearFactor);
	newElm.setAttribute(QObject::tr("AngularFactor"), stressSolver.angularFactor);
	newElm.setAttribute(QObject::tr("MeanError"), stressSolver.meanError);
	newElm.setAttribute(QObject::tr("VarianceError"), stressSolver.varianceError);
	newElm.setAttribute(QObject::tr("BondsPerFrame"), stressSolver.bondsPerFrame);
	newElm.setAttribute(QObject::tr("BondsIterations"), stressSolver.bondsIterations);
}

void BlastProject::_loadStressSolverPreset(QDomElement& parentElm, StressSolverUserPreset& stressSolverUserPreset)
{
	stressSolverUserPreset.name = parentElm.attribute(QObject::tr("Name")).toUtf8().data();

	QDomElement stressSolverElm = parentElm.firstChildElement(QObject::tr("StressSolver"));
	_loadStressSolver(stressSolverElm, stressSolverUserPreset.stressSolver);
}

void BlastProject::_loadStressSolver(QDomElement& parentElm, BPPStressSolver& stressSolver)
{
	stressSolver.solverMode = parentElm.attribute(QObject::tr("SolverMode")).toInt();
	stressSolver.linearFactor = parentElm.attribute(QObject::tr("LinearFactor")).toFloat();
	stressSolver.angularFactor = parentElm.attribute(QObject::tr("AngularFactor")).toFloat();
	stressSolver.meanError = parentElm.attribute(QObject::tr("MeanError")).toFloat();
	stressSolver.varianceError = parentElm.attribute(QObject::tr("VarianceError")).toFloat();
	stressSolver.bondsPerFrame = parentElm.attribute(QObject::tr("BondsPerFrame")).toUInt();
	stressSolver.bondsIterations = parentElm.attribute(QObject::tr("BondsIterations")).toUInt();
}

void BlastProject::_addStringItem(BPPStringArray& theArray, const char* name)
{
	if (name == nullptr)
		return;

	NvParameterized::DummyStringStruct* oldBuf = theArray.buf;
	theArray.buf = new NvParameterized::DummyStringStruct[theArray.arraySizes[0] + 1];

	int i = 0;
	for (; i < theArray.arraySizes[0]; ++i)
	{
		NvParameterized::DummyStringStruct& newItem = theArray.buf[i];
		NvParameterized::DummyStringStruct& oldItem = oldBuf[i];
		newItem.buf = nullptr;
		copy(newItem, oldItem);
	}

	NvParameterized::DummyStringStruct& newItem = theArray.buf[i];
	newItem.buf = nullptr;
	copy(newItem, name);
	theArray.arraySizes[0] += 1;

	delete[] oldBuf;
}

void BlastProject::_removeStringItem(BPPStringArray& theArray, const char* name)
{
	if (name == nullptr || strlen(name) == 0)
		return;

	bool nameExist = false;
	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		NvParameterized::DummyStringStruct& item = theArray.buf[i];
		if (nvidia::shdfnd::strcmp(item.buf, name) == 0)
		{
			nameExist = true;
			break;
		}
	}

	if (!nameExist)
		return;

	NvParameterized::DummyStringStruct* oldBuf = theArray.buf;

	theArray.buf = new NvParameterized::DummyStringStruct[theArray.arraySizes[0] - 1];
	int index = 0;
	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		if (nvidia::shdfnd::strcmp(oldBuf[i].buf, name) != 0)
		{
			NvParameterized::DummyStringStruct& newItem = theArray.buf[index++];
			NvParameterized::DummyStringStruct& oldItem = oldBuf[i];
			newItem.buf = nullptr;
			copy(newItem, oldItem);
		}
	}
	theArray.arraySizes[0] -= 1;
	delete[] oldBuf;
}

static bool LoadParamVec2Array(NvParameterized::Interface* iface,
                               const char* paramName,
                               nvidia::NvVec2** outValue)
{
	NvParameterized::Handle handle(iface);
	if (iface->getParameterHandle(paramName, handle) == NvParameterized::ERROR_NONE)
	{
		int arraySize;
		handle.getArraySize(arraySize);
		*outValue = new nvidia::NvVec2[arraySize];
		handle.getParamVec2Array((nvidia::NvVec2*)*outValue, arraySize);
		return true;
	}
	return false;
}

static bool LoadParamVec3Array(NvParameterized::Interface* iface,
                               const char* paramName,
                               nvidia::NvVec3** outValue)
{
	NvParameterized::Handle handle(iface);
	if (iface->getParameterHandle(paramName, handle) == NvParameterized::ERROR_NONE)
	{
		int arraySize;
		handle.getArraySize(arraySize);
		*outValue = new nvidia::NvVec3[arraySize];
		handle.getParamVec3Array((nvidia::NvVec3*)*outValue, arraySize);
		return true;
	}
	return false;
}

static bool LoadParamVec4Array(NvParameterized::Interface* iface,
                               const char* paramName,
                               nvidia::NvVec4** outValue)
{
	NvParameterized::Handle handle(iface);
	if (iface->getParameterHandle(paramName, handle) == NvParameterized::ERROR_NONE)
	{
		int arraySize;
		handle.getArraySize(arraySize);
		*outValue = new nvidia::NvVec4[arraySize];
		handle.getParamVec4Array((nvidia::NvVec4*)*outValue, arraySize);
		return true;
	}
	return false;
}

static bool LoadParamU8Array(NvParameterized::Interface* iface,
                             const char* paramName,
                             NvUInt8** outValue)
{
	NvParameterized::Handle handle(iface);
	if (iface->getParameterHandle(paramName, handle) == NvParameterized::ERROR_NONE)
	{
		int arraySize;
		handle.getArraySize(arraySize);
		*outValue = new NvUInt8[arraySize];
		handle.getParamU8Array((NvUInt8*)*outValue, arraySize);
		return true;
	}
	return false;
}

static bool LoadParamU32Array(NvParameterized::Interface* iface,
                              const char* paramName,
                              NvUInt32** outValue)
{
	NvParameterized::Handle handle(iface);
	if (iface->getParameterHandle(paramName, handle) == NvParameterized::ERROR_NONE)
	{
		int arraySize;
		handle.getArraySize(arraySize);
		*outValue = new NvUInt32[arraySize];
		handle.getParamU32Array((NvUInt32*)*outValue, arraySize);
		return true;
	}
	return false;
}

static bool LoadParamString(NvParameterized::Interface* iface,
                            const char* paramName,
                            char* outString)
{
	if (outString == NV_NULL)
	{
		return false;
	}
	NvParameterized::Handle handle(iface);
	if (iface->getParameterHandle(paramName, handle) == NvParameterized::ERROR_NONE)
	{
		const char* var;
		handle.getParamString(var);
		if (var)
		{
			strcpy(outString, var);
		}
		else
		{
			outString[0] = '\0';
		}
		return true;
	}
	return false;
}

static bool SaveParamVec2Array(NvParameterized::Interface* iface,
                               const char* paramName,
                               const nvidia::NvVec2* value,
                               int arraySize)
{
	NvParameterized::Handle handle(iface);
	if (iface->getParameterHandle(paramName, handle) == NvParameterized::ERROR_NONE)
	{
		handle.resizeArray(arraySize);
		handle.setParamVec2Array((nvidia::NvVec2*)value, arraySize);
		return true;
	}
	return false;
}

static bool SaveParamVec3Array(NvParameterized::Interface* iface,
                               const char* paramName,
                               const nvidia::NvVec3* value,
                               int arraySize)
{
	NvParameterized::Handle handle(iface);
	if (iface->getParameterHandle(paramName, handle) == NvParameterized::ERROR_NONE)
	{
		handle.resizeArray(arraySize);
		handle.setParamVec3Array((nvidia::NvVec3*)value, arraySize);
		return true;
	}
	return false;
}

static bool SaveParamVec4Array(NvParameterized::Interface* iface,
                               const char* paramName,
                               const nvidia::NvVec4* value,
                               int arraySize)
{
	NvParameterized::Handle handle(iface);
	if (iface->getParameterHandle(paramName, handle) == NvParameterized::ERROR_NONE)
	{
		handle.resizeArray(arraySize);
		handle.setParamVec4Array((nvidia::NvVec4*)value, arraySize);
		return true;
	}
	return false;
}

static bool SaveParamU8Array(NvParameterized::Interface* iface,
                             const char* paramName,
                             const NvUInt8* value,
                             int arraySize)
{
	NvParameterized::Handle handle(iface);
	if (iface->getParameterHandle(paramName, handle) == NvParameterized::ERROR_NONE)
	{
		handle.resizeArray(arraySize);
		handle.setParamU8Array((NvUInt8*)value, arraySize);
		return true;
	}
	return false;
}

static bool SaveParamU32Array(NvParameterized::Interface* iface,
                              const char* paramName,
                              const NvUInt32* value,
                              int arraySize)
{
	NvParameterized::Handle handle(iface);
	if (iface->getParameterHandle(paramName, handle) == NvParameterized::ERROR_NONE)
	{
		handle.resizeArray(arraySize);
		handle.setParamU32Array((NvUInt32*)value, arraySize);
		return true;
	}
	return false;
}

static bool SaveParamString(NvParameterized::Interface* iface,
                            const char* paramName,
                            const char* inString)
{
	NvParameterized::Handle handle(iface);
	if (iface->getParameterHandle(paramName, handle) == NvParameterized::ERROR_NONE)
	{
		handle.setParamString(inString);
		return true;
	}
	return false;
}

bool CreateProjectParamsContext()
{
	FoundationHolder::GetFoundation();

	ProjectParamsContext* context = new ProjectParamsContext;
	g_projectParamsContext = context;
	if (context == nullptr)
		return false;

	//context->mFoundation = FoundationHolder::GetFoundation();
	//assert(context->mFoundation != NV_NULL);
	context->mTraits = NvParameterized::createTraits();
	context->mBlastProjectParametersFactory = new BlastProjectParametersFactory;
	context->mTraits->registerFactory(*context->mBlastProjectParametersFactory);
	return true;
}

void ReleaseProjectParamsContext()
{
	g_projectParamsContext->mTraits->release();
	delete g_projectParamsContext->mBlastProjectParametersFactory;
	delete g_projectParamsContext;
}

bool ProjectParamsLoad(const char* filePath,
                       SimpleScene* scene)
{
	if (g_projectParamsContext == NV_NULL) return false;
	NvFileBuf* stream = new NvFileBufferBase(filePath, NvFileBuf::OPEN_READ_ONLY);
	if (!stream || !stream->isOpen())
	{
		// file open error
		if (stream) stream->release();
		return false;
	}
	NvParameterized::Serializer::DeserializedData data;
	NvParameterized::Serializer::ErrorType serError = NvParameterized::Serializer::ERROR_NONE;
	NvParameterized::XmlSerializer serializer(g_projectParamsContext->mTraits);
	bool isUpdated;
	serError = serializer.deserialize(*stream, data, isUpdated);
	if (data.size() < 1)
	{
		if (stream) stream->release();
		return false;
	}

	scene->Clear();

	for (int idx = 0; idx < (int)data.size(); ++idx) {
		NvParameterized::Interface* iface = data[idx];
		if (::strcmp(iface->className(), BlastProjectParameters::staticClassName()) == 0)
		{
			scene->LoadParameters(iface);
		}
	}
	stream->release();
	return true;
}

bool ProjectParamsSave(const char* filePath,
                       SimpleScene* scene)
{
	if (g_projectParamsContext == NV_NULL) return false;
	NvParameterized::XmlSerializer serializer(g_projectParamsContext->mTraits);
	NvFileBuf* stream = new NvFileBufferBase(filePath, NvFileBuf::OPEN_WRITE_ONLY);
	if (!stream || !stream->isOpen())
	{
		// file open error
		if (stream) stream->release();
		return false;
	}
	NvParameterized::Traits* traits = g_projectParamsContext->mTraits;
	int numObjects = 0;
	const int kMaxObjects = 1;
	NvParameterized::Interface* objects[kMaxObjects];
	
	if (1)
	{
		BlastProjectParameters* params = new BlastProjectParameters(traits);
		objects[numObjects++] = params;
		NvParameterized::Interface* iface = static_cast<NvParameterized::Interface*>(params);
		scene->SaveParameters(iface);
	}

	NV_ASSERT(numObjects <= kMaxObjects);
	NvParameterized::Serializer::ErrorType serError = NvParameterized::Serializer::ERROR_NONE;
	bool isUpdate = false;
	serError = serializer.serialize(*stream, (const NvParameterized::Interface**)&objects[0], numObjects, isUpdate);
	//for (int idx = 0; idx < numObjects; ++idx)
	//{
	//	delete objects[idx];
	//}
	stream->release();
	return true;
}

// Utility function to get the child parameter handle from the parent handle.
bool ParamGetChild(NvParameterized::Handle& parentHandle, NvParameterized::Handle& outChildHandle, const char* childName)
{
	if (parentHandle.getChildHandle(parentHandle.getInterface(), childName, outChildHandle) == NvParameterized::ERROR_NONE)
	{
		return true;
	}
	return false;
}
