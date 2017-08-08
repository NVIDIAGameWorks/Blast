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
#include <limits> 
#include "BlastSceneTree.h"

using namespace nvidia;
using namespace nvidia::parameterized;
using namespace nvidia::parameterized::BlastProjectParametersNS;

struct ProjectParamsContext*	g_projectParamsContext = nullptr;
const char* USER_PRESET = "UserPreset.userPreset";
const char* FRACTURE_PRESET = "FracturePreset.fracturePreset";
const char* FILTER_PRESET = "FilterPreset.filterPreset";

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
	delete[] data.materialAssignments.buf;
	data.materialAssignments.buf = nullptr;
	data.materialAssignments.arraySizes[0] = 0;

	delete[] data.positions.buf;
	data.positions.buf = nullptr;
	data.positions.arraySizes[0] = 0;

	delete[] data.normals.buf;
	data.normals.buf = nullptr;
	data.normals.arraySizes[0] = 0;

	delete[] data.tangents.buf;
	data.tangents.buf = nullptr;
	data.tangents.arraySizes[0] = 0;

	delete[] data.texcoords.buf;
	data.texcoords.buf = nullptr;
	data.texcoords.arraySizes[0] = 0;

	delete data.positions.buf;
	data.positions.buf = nullptr;
	data.positions.arraySizes[0] = 0;

	delete[] data.positionIndexes.buf;
	data.positionIndexes.buf = nullptr;
	data.positionIndexes.arraySizes[0] = 0;

	delete[] data.normalIndexes.buf;
	data.normalIndexes.buf = nullptr;
	data.normalIndexes.arraySizes[0] = 0;

	delete[] data.tangentIndexes.buf;
	data.tangentIndexes.buf = nullptr;
	data.tangentIndexes.arraySizes[0] = 0;

	delete[] data.texcoordIndexes.buf;
	data.texcoordIndexes.buf = nullptr;
	data.texcoordIndexes.arraySizes[0] = 0;

	delete[] data.materialIDs.buf;
	data.materialIDs.buf = nullptr;
	data.materialIDs.arraySizes[0] = 0;
}

void freeBlast(BPPChunk& data)
{
	freeString(data.name);
	freeBlast(data.graphicsMesh);
}

void freeBlast(BPPBond& data)
{
	freeString(data.name);
}

void freeBlast(BPPAsset& data)
{
	freeString(data.name);
	freeString(data.fbx);
	freeString(data.obj);
	freeString(data.llasset);
	freeString(data.tkasset);
	freeString(data.bpxa);
}

void freeBlast(BPPAssetInstance& data)
{
	freeString(data.name);
	freeString(data.exMaterial);
	freeString(data.inMaterial);
}

void freeBlast(BPPBlast& data)
{
	freeString(data.fileReferences.fbxSourceAsset);

	freeBlast(data.blastAssets);
	freeBlast(data.blastAssetInstances);
	freeBlast(data.chunks);
	freeBlast(data.bonds);
}

void freeBlast(BPPGraphicsMaterial& data)
{
	freeString(data.name);
	freeString(data.diffuseTextureFilePath);
	freeString(data.specularTextureFilePath);
	freeString(data.normalTextureFilePath);
}

void freeBlast(BPPDefaultDamage& data)
{
	delete[] data.damageStructs.buf;
	data.damageStructs.buf = nullptr;
	data.damageStructs.arraySizes[0] = 0;
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

void freeBlast(BPPGraphicsMaterialArray& data)
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

bool isItemExist(BPPStringArray& dest, const char* item)
{
	if (nullptr == item || 0 == strlen(item))
	{
		return false;
	}

	for (int i = 0; i < dest.arraySizes[0]; ++i)
	{
		NvParameterized::DummyStringStruct& curItem = dest.buf[i];

		if (nvidia::shdfnd::strcmp(curItem.buf, item) == 0)
			return true;
	}

	return false;
}

void addItem(BPPStringArray& dest, const char* item)
{
	if (nullptr == item || 0 == strlen(item))
	{
		return;
	}

	NvParameterized::DummyStringStruct* oldBuf = dest.buf;
	dest.buf = new NvParameterized::DummyStringStruct[dest.arraySizes[0] + 1];
	int i = 0;
	for (; i < dest.arraySizes[0]; ++i)
	{
		copy(dest.buf[i], oldBuf[i]);
	}

	NvParameterized::DummyStringStruct& newItem = dest.buf[i];
	copy(newItem, item);
	dest.arraySizes[0] += 1;

	delete[] oldBuf;
}

void removeItem(BPPStringArray& dest, const char* item)
{
	if (!isItemExist(dest, item))
	{
		return;
	}

	NvParameterized::DummyStringStruct* oldBuf = dest.buf;
	dest.buf = new NvParameterized::DummyStringStruct[dest.arraySizes[0] - 1];

	int index = 0;
	for (int i = 0; i < dest.arraySizes[0]; ++i)
	{
		if (nvidia::shdfnd::strcmp(oldBuf[i].buf, item) != 0)
		{
			NvParameterized::DummyStringStruct& newItem = dest.buf[index++];
			NvParameterized::DummyStringStruct& oldItem = oldBuf[i];
			copy(newItem, oldItem);
		}
	}
	dest.arraySizes[0] -= 1;
	delete[] oldBuf;
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

void copy(BPPMaterialAssignmentsArray& dest, BPPMaterialAssignmentsArray& source)
{
	delete[] dest.buf;
	dest.buf = nullptr;
	dest.arraySizes[0] = 0;

	if (source.arraySizes[0] > 0)
	{
		dest.buf = new BPPMaterialAssignments[source.arraySizes[0]];
		for (int i = 0; i < source.arraySizes[0]; ++i)
		{
			BPPMaterialAssignments& destItem = dest.buf[i];
			BPPMaterialAssignments& sourceItem = source.buf[i];

			copy(destItem, sourceItem);
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

			init(destItem);

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

			init(destItem);

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

			init(destItem);

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

			init(destItem);

			copy(destItem, sourceItem);
		}
		dest.arraySizes[0] = source.arraySizes[0];
	}
}

void copy(BPPI32Array& dest, BPPI32Array& source)
{
	delete[] dest.buf;
	dest.buf = nullptr;
	dest.arraySizes[0] = 0;

	if (source.arraySizes[0] > 0)
	{
		dest.buf = new int32_t[source.arraySizes[0]];
		for (int i = 0; i < source.arraySizes[0]; ++i)
		{
			int32_t& destItem = dest.buf[i];
			int32_t& sourceItem = source.buf[i];

			destItem = sourceItem;
		}
		dest.arraySizes[0] = source.arraySizes[0];
	}
}

void copy(BPPVEC3Array& dest, BPPVEC3Array& source)
{
	delete[] dest.buf;
	dest.buf = nullptr;
	dest.arraySizes[0] = 0;

	if (source.arraySizes[0] > 0)
	{
		dest.buf = new nvidia::NvVec3[source.arraySizes[0]];
		for (int i = 0; i < source.arraySizes[0]; ++i)
		{
			nvidia::NvVec3& destItem = dest.buf[i];
			nvidia::NvVec3& sourceItem = source.buf[i];

			destItem = sourceItem;
		}
		dest.arraySizes[0] = source.arraySizes[0];
	}
}

void copy(BPPVEC2Array& dest, BPPVEC2Array& source)
{
	delete[] dest.buf;
	dest.buf = nullptr;
	dest.arraySizes[0] = 0;

	if (source.arraySizes[0] > 0)
	{
		dest.buf = new nvidia::NvVec2[source.arraySizes[0]];
		for (int i = 0; i < source.arraySizes[0]; ++i)
		{
			nvidia::NvVec2& destItem = dest.buf[i];
			nvidia::NvVec2& sourceItem = source.buf[i];

			destItem = sourceItem;
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
	copy(dest.materialAssignments, source.materialAssignments);
	copy(dest.positions, source.positions);
	copy(dest.normals, source.normals);
	copy(dest.tangents, source.tangents);
	copy(dest.texcoords, source.texcoords);
	dest.vertextCountInFace = source.vertextCountInFace;
	copy(dest.positionIndexes, source.positionIndexes);
	copy(dest.normalIndexes, source.normalIndexes);
	copy(dest.tangentIndexes, source.tangentIndexes);
	copy(dest.texcoordIndexes, source.texcoordIndexes);
	copy(dest.materialIDs, source.materialIDs);
}

void copy(BPPMaterialAssignments& dest, BPPMaterialAssignments& source)
{
	dest.libraryMaterialID = source.libraryMaterialID;
	dest.faceMaterialID = source.faceMaterialID;
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
	dest.asset = source.asset;
	dest.visible = source.visible;
	dest.support = source.support;
	dest.staticFlag = source.staticFlag;
	copy(dest.graphicsMesh, source.graphicsMesh);
}

void copy(BPPBond& dest, BPPBond& source)
{
	copy(dest.name, source.name);
	dest.asset = source.asset;
	dest.visible = source.visible;
	dest.fromChunk = source.fromChunk;
	dest.toChunk = source.toChunk;
	copy(dest.support, source.support);
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
	dest.hardness = source.hardness;
	dest.linearFactor = source.linearFactor;
	dest.angularFactor = source.angularFactor;
	dest.bondIterationsPerFrame = source.bondIterationsPerFrame;
	dest.graphReductionLevel = source.graphReductionLevel;
}

void copy(BPPAsset& dest, BPPAsset& source)
{
	dest.ID = source.ID;
	copy(dest.name, source.name);
	dest.visible = source.visible;
	dest.stressSolver = source.stressSolver;
	copy(dest.activeUserPreset, source.activeUserPreset);
	copy(dest.fbx, source.fbx);
	copy(dest.obj, source.obj);
	copy(dest.llasset, source.llasset);
	copy(dest.tkasset, source.tkasset);
	copy(dest.bpxa, source.bpxa);
	dest.exportFBX = source.exportFBX;
	dest.embedFBXCollision = source.embedFBXCollision;
	dest.exportOBJ = source.exportOBJ;
	dest.exportLLAsset = source.exportLLAsset;
	dest.exportTKAsset = source.exportTKAsset;
	dest.exportBPXA = source.exportBPXA;
}

void copy(BPPAssetInstance& dest, BPPAssetInstance& source)
{
	copy(dest.name, source.name);
	dest.visible = source.visible;
	dest.asset = source.asset;
	dest.transform = source.transform;
	copy(dest.exMaterial, source.exMaterial);
	copy(dest.inMaterial, source.inMaterial);
}

void copy(BPPBlast& dest, BPPBlast& source)
{
	copy(dest.fileReferences.fbxSourceAsset, source.fileReferences.fbxSourceAsset);

	copy(dest.blastAssets, source.blastAssets);
	copy(dest.blastAssetInstances, source.blastAssetInstances);
	copy(dest.chunks, source.chunks);
	copy(dest.bonds, source.bonds);
	copy(dest.healthMask, source.healthMask);
}

void copy(BPPFractureGeneral& dest, BPPFractureGeneral& source)
{
	copy(dest.fracturePreset, source.fracturePreset);
	dest.fractureType = source.fractureType;
	dest.applyMaterial = source.applyMaterial;
	dest.autoSelectNewChunks = source.autoSelectNewChunks;
	dest.selectionDepthTest = source.selectionDepthTest;
}

void copy(BPPVoronoi& dest, BPPVoronoi& source)
{
	dest.siteGeneration = source.siteGeneration;
	dest.numSites = source.numSites;
	dest.numberOfClusters = source.numberOfClusters;
	dest.sitesPerCluster = source.sitesPerCluster;
	dest.clusterRadius = source.clusterRadius;
}

void copy(BPPFracture& dest, BPPFracture& source)
{
	copy(dest.general, source.general);
	dest.visualization = source.visualization;
	copy(dest.voronoi, source.voronoi);
	dest.slice = source.slice;
}

void copy(BPPDefaultDamage& dest, BPPDefaultDamage& source)
{
	dest.damageAmount = source.damageAmount;
	dest.explosiveImpulse = source.explosiveImpulse;
	dest.stressDamageForce = source.stressDamageForce;
	dest.damageProfile = source.damageProfile;

	int count = source.damageStructs.arraySizes[0];

	if(dest.damageStructs.buf != nullptr && dest.damageStructs.arraySizes[0] != count)
	{
		delete[] dest.damageStructs.buf;
		dest.damageStructs.buf = nullptr;
		dest.damageStructs.arraySizes[0] = 0;
	}

	if (count == 0)
		return;

	if (dest.damageStructs.buf == nullptr)
	{
		dest.damageStructs.buf = new BPPDamageStruct[count];
		dest.damageStructs.arraySizes[0] = count;
	}

	for (int i = 0; i < count; ++i)
	{
		BPPDamageStruct& destItem = dest.damageStructs.buf[i];
		BPPDamageStruct& sourceItem = source.damageStructs.buf[i];

		destItem.damageRadius = sourceItem.damageRadius;
		destItem.continuously = sourceItem.continuously;
	}
}

void copy(BPPFilter& dest, BPPFilter& source)
{
	copy(dest.activeFilter, source.activeFilter);
	copy(dest.filterRestrictions, source.filterRestrictions);
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
	copy(dest.fracture, source.fracture);
	copy(dest.defaultDamage, source.defaultDamage);
	copy(dest.filter, source.filter);
}

void merge(BPPAssetArray& dest, BPPAssetArray& source)
{
	if (source.arraySizes[0] > 0)
	{
		BPPAsset* oriDestArray = dest.buf;
		int oriCount = dest.arraySizes[0];
		int srcCount = source.arraySizes[0];
		dest.buf = new BPPAsset[oriCount + srcCount];
		int i = 0;
//		std::map<BPPAsset*, BPPAsset*> changeMap;
		for (; i < oriCount; ++i)
		{
			BPPAsset& destItem = dest.buf[i];
			BPPAsset& oriItem = oriDestArray[i];

			init(destItem);
			copy(destItem, oriItem);
//			changeMap[&oriItem] = &destItem;
		}
//		BlastTreeData::ins().refreshProjectDataToNodeMap(changeMap);
		for (int j = 0; j < srcCount; ++j, ++i)
		{
			BPPAsset& destItem = dest.buf[i];
			BPPAsset& sourceItem = source.buf[j];

			init(destItem);
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

void merge(BPPAssetInstanceArray& dest, BPPAssetInstanceArray& source)
{
	if (source.arraySizes[0] > 0)
	{
		BPPAssetInstance* oriDestArray = dest.buf;
		int oriCount = dest.arraySizes[0];
		int srcCount = source.arraySizes[0];
		dest.buf = new BPPAssetInstance[oriCount + srcCount];
		int i = 0;
//		std::map<BPPAssetInstance*, BPPAssetInstance*> changeMap;
		for (; i < oriCount; ++i)
		{
			BPPAssetInstance& destItem = dest.buf[i];
			BPPAssetInstance& oriItem = oriDestArray[i];

			init(destItem);
			copy(destItem, oriItem);
//			changeMap[&oriItem] = &destItem;
		}
//		BlastTreeData::ins().refreshProjectDataToNodeMap(changeMap);
		for (int j = 0; j < srcCount; ++j, ++i)
		{
			BPPAssetInstance& destItem = dest.buf[i];
			BPPAssetInstance& sourceItem = source.buf[j];

			init(destItem);
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

			init(destItem);
			copy(destItem, oriItem);
		}
		for (int j = 0; j < srcCount; ++j, ++i)
		{
			BPPChunk& destItem = dest.buf[i];
			BPPChunk& sourceItem = source.buf[j];

			init(destItem);
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

			init(destItem);
			copy(destItem, oriItem);
		}
		for (int j = 0; j < srcCount; ++j, ++i)
		{
			BPPBond& destItem = dest.buf[i];
			BPPBond& sourceItem = source.buf[j];

			init(destItem);
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

/*
void apart(BPPAssetArray& dest, BPPAssetArray& source)
{
	if (source.arraySizes[0] == 0)
		return;

	int destCount = dest.arraySizes[0];
	int srcCount = source.arraySizes[0];
	std::vector<int> indexes;
	for (int i = 0; i < destCount; ++i)
	{
		bool find = false;
		for (int j = 0; j < srcCount; ++j)
		{
			if (dest.buf[i].ID == source.buf[j].ID)
			{
				find = true;
				break;
			}
		}

		if (!find)
		{
			indexes.push_back(i);
		}
	}

	int newSize = indexes.size();
	BPPAsset* newArray = nullptr;
	if (newSize > 0)
	{
		newArray = new BPPAsset[newSize];
		std::map<BPPAsset*, BPPAsset*> changeMap;
		for (int n = 0; n < newSize; ++n)
		{
			BPPAsset& newItem = newArray[n];
			BPPAsset& oriItem = dest.buf[indexes[n]];
			init(newItem);
			copy(newItem, oriItem);
			changeMap[&oriItem] = &newItem;
		}
		BlastTreeData::ins().refreshProjectDataToNodeMap(changeMap);
	}

	freeBlast(dest);
	dest.buf = newArray;
	dest.arraySizes[0] = newSize;
}

void apart(BPPAssetInstanceArray& dest, BPPAssetInstanceArray& source)
{
	if (source.arraySizes[0] == 0)
		return;

	int destCount = dest.arraySizes[0];
	int srcCount = source.arraySizes[0];
	std::vector<int> indexes;
	for (int i = 0; i < destCount; ++i)
	{
		bool find = false;
		for (int j = 0; j < srcCount; ++j)
		{
			if (::strcmp(dest.buf[i].name.buf, source.buf[j].name.buf) == 0
				&& dest.buf[i].asset == source.buf[j].asset)
			{
				find = true;
				break;
			}
		}

		if (!find)
		{
			indexes.push_back(i);
		}
	}

	int newSize = indexes.size();
	BPPAssetInstance* newArray = nullptr;
	if (newSize > 0)
	{
		newArray = new BPPAssetInstance[newSize];
		std::map<BPPAssetInstance*, BPPAssetInstance*> changeMap;
		for (int n = 0; n < newSize; ++n)
		{
			BPPAssetInstance& newItem = newArray[n];
			BPPAssetInstance& oriItem = dest.buf[indexes[n]];
			init(newItem);
			copy(newItem, oriItem);
			changeMap[&oriItem] = &newItem;
		}
		BlastTreeData::ins().refreshProjectDataToNodeMap(changeMap);
	}

	freeBlast(dest);
	dest.buf = newArray;
	dest.arraySizes[0] = newSize;
}

void apart(BPPChunkArray& dest, BPPChunkArray& source)
{
	if (source.arraySizes[0] == 0)
		return;

	int destCount = dest.arraySizes[0];
	int srcCount = source.arraySizes[0];
	std::vector<int> indexes;
	for (int i = 0; i < destCount; ++i)
	{
		bool find = false;
		for (int j = 0; j < srcCount; ++j)
		{
			if (::strcmp(dest.buf[i].name.buf, source.buf[j].name.buf) == 0
				&& dest.buf[i].asset == source.buf[j].asset)
			{
				find = true;
				break;
			}
		}

		if (!find)
		{
			indexes.push_back(i);
		}
	}

	int newSize = indexes.size();
	BPPChunk* newArray = nullptr;
	if (newSize > 0)
	{
		newArray = new BPPChunk[newSize];

		for (int n = 0; n < newSize; ++n)
		{
			BPPChunk& newItem = newArray[n];
			init(newItem);
			copy(newItem, dest.buf[indexes[n]]);
		}
	}

	freeBlast(dest);
	dest.buf = newArray;
	dest.arraySizes[0] = newSize;
}

void apart(BPPBondArray& dest, BPPBondArray& source)
{
	if (source.arraySizes[0] == 0)
		return;

	int destCount = dest.arraySizes[0];
	int srcCount = source.arraySizes[0];
	std::vector<int> indexes;
	for (int i = 0; i < destCount; ++i)
	{
		bool find = false;
		for (int j = 0; j < srcCount; ++j)
		{
			if (::strcmp(dest.buf[i].name.buf, source.buf[j].name.buf) == 0
				&& dest.buf[i].asset == source.buf[j].asset)
			{
				find = true;
				break;
			}
		}

		if (!find)
		{
			indexes.push_back(i);
		}
	}

	int newSize = indexes.size();
	BPPBond* newArray = nullptr;
	if (newSize > 0)
	{
		newArray = new BPPBond[newSize];

		for (int n = 0; n < newSize; ++n)
		{
			BPPBond& newItem = newArray[n];
			init(newItem);
			copy(newItem, dest.buf[indexes[n]]);
		}
	}

	freeBlast(dest);
	dest.buf = newArray;
	dest.arraySizes[0] = newSize;
}

*/
void apart(BPPAssetArray& dest, int32_t assetId)
{
	if (assetId < 0)
	{
		return;
	}

	int destCount = dest.arraySizes[0];
	std::vector<int> indexes;
	for (int i = 0; i < destCount; ++i)
	{
		if (dest.buf[i].ID == assetId)
		{
			continue;
		}

		indexes.push_back(i);
	}

	int newSize = indexes.size();
	BPPAsset* newArray = nullptr;
	if (newSize > 0)
	{
		newArray = new BPPAsset[newSize];
//		std::map<BPPAsset*, BPPAsset*> changeMap;
		for (int n = 0; n < newSize; ++n)
		{
			BPPAsset& newItem = newArray[n];
			BPPAsset& oriItem = dest.buf[indexes[n]];
			init(newItem);
			copy(newItem, oriItem);
//			changeMap[&oriItem] = &newItem;
		}
//		BlastTreeData::ins().refreshProjectDataToNodeMap(changeMap);
	}

	freeBlast(dest);
	dest.buf = newArray;
	dest.arraySizes[0] = newSize;
}

void apart(BPPAssetInstanceArray& dest, int32_t assetId)
{
	if (assetId < 0)
	{
		return;
	}

	int destCount = dest.arraySizes[0];
	std::vector<int> indexes;
	for (int i = 0; i < destCount; ++i)
	{
		if (dest.buf[i].asset == assetId)
		{
			continue;
		}

		indexes.push_back(i);
	}

	int newSize = indexes.size();
	BPPAssetInstance* newArray = nullptr;
	if (newSize > 0)
	{
		newArray = new BPPAssetInstance[newSize];
//		std::map<BPPAssetInstance*, BPPAssetInstance*> changeMap;
		for (int n = 0; n < newSize; ++n)
		{
			BPPAssetInstance& newItem = newArray[n];
			BPPAssetInstance& oriItem = dest.buf[indexes[n]];
			init(newItem);
			copy(newItem, oriItem);
//			changeMap[&oriItem] = &newItem;
		}
//		BlastTreeData::ins().refreshProjectDataToNodeMap(changeMap);
	}

	freeBlast(dest);
	dest.buf = newArray;
	dest.arraySizes[0] = newSize;
}

void apart(BPPAssetInstanceArray& dest, int32_t assetId, const char* instanceName)
{
	if (assetId < 0 || instanceName == nullptr)
	{
		return;
	}

	int destCount = dest.arraySizes[0];
	std::vector<int> indexes;
	for (int i = 0; i < destCount; ++i)
	{
		if (dest.buf[i].asset == assetId &&
			::strcmp(dest.buf[i].name.buf, instanceName) == 0)
		{
			continue;
		}

		indexes.push_back(i);
	}

	int newSize = indexes.size();
	BPPAssetInstance* newArray = nullptr;
	if (newSize > 0)
	{
		newArray = new BPPAssetInstance[newSize];
//		std::map<BPPAssetInstance*, BPPAssetInstance*> changeMap;
		for (int n = 0; n < newSize; ++n)
		{
			BPPAssetInstance& newItem = newArray[n];
			BPPAssetInstance& oriItem = dest.buf[indexes[n]];
			init(newItem);
			copy(newItem, oriItem);
//			changeMap[&oriItem] = &newItem;
		}
//		BlastTreeData::ins().refreshProjectDataToNodeMap(changeMap);
	}

	freeBlast(dest);
	dest.buf = newArray;
	dest.arraySizes[0] = newSize;
}

void apart(BPPChunkArray& dest, int32_t assetId)
{
	if (assetId < 0)
	{
		return;
	}

	int destCount = dest.arraySizes[0];
	std::vector<int> indexes;
	for (int i = 0; i < destCount; ++i)
	{
		if (dest.buf[i].asset == assetId)
		{
			continue;
		}

		indexes.push_back(i);
	}

	int newSize = indexes.size();
	BPPChunk* newArray = nullptr;
	if (newSize > 0)
	{
		newArray = new BPPChunk[newSize];
//		std::map<BPPChunk*, BPPChunk*> changeMap;
		for (int n = 0; n < newSize; ++n)
		{
			BPPChunk& newItem = newArray[n];
			BPPChunk& oriItem = dest.buf[indexes[n]];
			init(newItem);
			copy(newItem, oriItem);
//			changeMap[&oriItem] = &newItem;
		}
//		BlastTreeData::ins().refreshProjectDataToNodeMap(changeMap);
	}

	freeBlast(dest);
	dest.buf = newArray;
	dest.arraySizes[0] = newSize;
}

void apart(BPPBondArray& dest, int32_t assetId)
{
	if (assetId < 0)
	{
		return;
	}

	int destCount = dest.arraySizes[0];
	std::vector<int> indexes;
	for (int i = 0; i < destCount; ++i)
	{
		if (dest.buf[i].asset == assetId)
		{
			continue;
		}

		indexes.push_back(i);
	}

	int newSize = indexes.size();
	BPPBond* newArray = nullptr;
	if (newSize > 0)
	{
		newArray = new BPPBond[newSize];
//		std::map<BPPBond*, BPPBond*> changeMap;
		for (int n = 0; n < newSize; ++n)
		{
			BPPBond& newItem = newArray[n];
			BPPBond& oriItem = dest.buf[indexes[n]];
			init(newItem);
			copy(newItem, oriItem);
//			changeMap[&oriItem] = &newItem;
		}
//		BlastTreeData::ins().refreshProjectDataToNodeMap(changeMap);
	}

	freeBlast(dest);
	dest.buf = newArray;
	dest.arraySizes[0] = newSize;
}

void init(BPPStressSolver& param)
{
	param.hardness = 1000.0f;
	param.linearFactor = 0.25f;
	param.angularFactor = 0.75f;
	param.bondIterationsPerFrame = 18000;
	param.graphReductionLevel = 3;
}

void init(BPPGraphicsMaterial& param)
{
	param.ID = -1;
	param.name.buf = nullptr;
	param.useTextures = false;
	param.diffuseTextureFilePath.buf = nullptr;
	param.specularTextureFilePath.buf = nullptr;
	param.normalTextureFilePath.buf = nullptr;
	param.specularShininess = 20.0;
}

void init(BPPGraphicsMesh& param)
{
	param.materialAssignments.buf = 0;
	param.materialAssignments.arraySizes[0];

	param.positions.buf = nullptr;
	param.positions.arraySizes[0] = 0;

	param.normals.buf = nullptr;
	param.normals.arraySizes[0] = 0;

	param.tangents.buf = nullptr;
	param.tangents.arraySizes[0] = 0;

	param.texcoords.buf = nullptr;
	param.texcoords.arraySizes[0] = 0;

	init<I32_DynamicArray1D_Type>(param.positionIndexes);
	init<I32_DynamicArray1D_Type>(param.normalIndexes);
	init<I32_DynamicArray1D_Type>(param.tangentIndexes);
	init<I32_DynamicArray1D_Type>(param.texcoordIndexes);
	init<I32_DynamicArray1D_Type>(param.materialIDs);
}

void init(BPPBond& param)
{
	param.name.buf = nullptr;
	param.asset = -1;
	param.visible = true;
	param.support.healthMask.buf = nullptr;
	param.support.bondStrength = 1.0;
	param.support.enableJoint = false;
}

void init(BPPChunk& param)
{
	param.name.buf = nullptr;
	param.asset = -1;
	param.visible = true;

	init(param.graphicsMesh);
}

void init(BPPDefaultDamage& param)
{
	/*
	param.compressiveDamage = 1.0f;
	param.explosiveImpulse = 100.0f;
	param.damageRadius = 5.0f;
	param.stressDamageForce = 1.0f;
	param.damageProfile = 0;
	*/
	param.damageStructs.buf = nullptr;
	param.damageStructs.arraySizes[0] = 0;
	param.damageProfile = -1;
}

void init(BPPAsset& param)
{
	param.ID = -1;
	param.name.buf = nullptr;
	param.activeUserPreset.buf = nullptr;
	init(param.stressSolver);
	param.obj.buf = nullptr;
	param.fbx.buf = nullptr;
	param.llasset.buf = nullptr;
	param.tkasset.buf = nullptr;
	param.bpxa.buf = nullptr;
	param.exportFBX = false;
	param.embedFBXCollision = true;
	param.exportOBJ = false;
	param.exportLLAsset = false;
	param.exportTKAsset = false;
	param.exportBPXA = false;
}

void init(BPPAssetInstance& param)
{
	param.name.buf = nullptr;
	param.asset = -1;
	param.visible = true;
	param.exMaterial.buf = nullptr;
	param.inMaterial.buf = nullptr;
}

void init(BPPVoronoi& param)
{
	param.siteGeneration = 0;
	param.numSites = 5;
	param.numberOfClusters = 1;
	param.sitesPerCluster = 1.0f;
	param.clusterRadius = 1.0f;
}

void init(BPPSlice& param)
{
	param.numSlicesX = 1;
	param.numSlicesY = 1;
	param.numSlicesZ = 1;
	param.offsetVariation = 0.0f;
	param.rotationVariation = 0.0f;
	param.noiseAmplitude = 0.0f;
	param.noiseFrequency = 1.0f;
	param.noiseOctaveNumber = 1;
	param.noiseSeed = 1;
	param.surfaceResolution = 1;
}

void init(BPPFractureVisualization& param)
{
	param.displayFractureWidget = false;
	param.fracturePreview = false;
}

void init(BPParams& params)
{
	//memset(&params, sizeof(BPParams), 0);

	//params.cameraBookmarks.buf = nullptr;
	//for (int i = 0; i < 4; ++i)
	//	params.renderer.lights.buf[i].name.buf = 
}

const char* convertFilterRestrictionToString(EFilterRestriction& restriction)
{
	switch (restriction)
	{
	case eFilterRestriction_AllDescendants:
		return "AllDescendants";
	case eFilterRestriction_AllParents:
		return "AllParents";
	case eFilterRestriction_DepthAll:
		return "DepthAll";
	case eFilterRestriction_Depth0:
		return "Depth0";
	case eFilterRestriction_Depth1:
		return "Depth1";
	case eFilterRestriction_Depth2:
		return "Depth2";
	case eFilterRestriction_Depth3:
		return "Depth3";
	case eFilterRestriction_Depth4:
		return "Depth4";
	case eFilterRestriction_Depth5:
		return "Depth5";
	case eFilterRestriction_ItemTypeAll:
		return "ItemTypeAll";
	case eFilterRestriction_Chunk:
		return "Chunk";
	case eFilterRestriction_SupportChunk:
		return "SupportChunk";
	case eFilterRestriction_StaticSupportChunk:
		return "StaticSupportChunk";
	case eFilterRestriction_Bond:
		return "Bond";
	case eFilterRestriction_WorldBond:
		return "WorldBond";
	case eFilterRestriction_EqualTo:
		return "EqualTo";
	case eFilterRestriction_NotEquaTo:
		return "NotEqualTo";
	}

	return "";
}

EFilterRestriction convertStringToFilterRestriction(const char* restriction)
{
	static std::map<std::string, EFilterRestriction> stringRestrictionMap;
	if (0 == stringRestrictionMap.size())
	{
		stringRestrictionMap["AllDescendants"]	= eFilterRestriction_AllDescendants;
		stringRestrictionMap["AllParents"]		= eFilterRestriction_AllParents;
		stringRestrictionMap["DepthAll"]		= eFilterRestriction_DepthAll;
		stringRestrictionMap["Depth0"]			= eFilterRestriction_Depth0;
		stringRestrictionMap["Depth1"]			= eFilterRestriction_Depth1;
		stringRestrictionMap["Depth2"]			= eFilterRestriction_Depth2;
		stringRestrictionMap["Depth3"]			= eFilterRestriction_Depth3;
		stringRestrictionMap["Depth4"]			= eFilterRestriction_Depth4;
		stringRestrictionMap["Depth5"]			= eFilterRestriction_Depth5;
		stringRestrictionMap["ItemTypeAll"]		= eFilterRestriction_ItemTypeAll;
		stringRestrictionMap["Chunk"]			= eFilterRestriction_Chunk;
		stringRestrictionMap["SupportChunk"]	= eFilterRestriction_SupportChunk;
		stringRestrictionMap["StaticSupportChunk"] = eFilterRestriction_StaticSupportChunk;
		stringRestrictionMap["Bond"]			= eFilterRestriction_Bond;
		stringRestrictionMap["WorldBond"]		= eFilterRestriction_WorldBond;
		stringRestrictionMap["EqualTo"]			= eFilterRestriction_EqualTo;
		stringRestrictionMap["NotEqualTo"]		= eFilterRestriction_NotEquaTo;
	}

	if (nullptr == restriction || 0 == strlen(restriction))
		return eFilterRestriction_Invalid;

	for (std::map<std::string, EFilterRestriction>::iterator itr = stringRestrictionMap.begin(); itr != stringRestrictionMap.end(); ++itr)
	{
		if (0 == nvidia::shdfnd::stricmp(itr->first.c_str(), restriction))
			return itr->second;
	}

	return eFilterRestriction_Invalid;
}

FilterPreset::FilterPreset(const char* inName)
{
	name = inName;
}

StressSolverUserPreset::StressSolverUserPreset(const char* inName)
	: name(inName)
{
	name = name;
	init(stressSolver);
}

FracturePreset::FracturePreset(const char* inName, FractureType inType)
	: name(inName)
	, type(inType)
{
	init();
}

void FracturePreset::setType(FractureType inType)
{
	type = inType;

	if (eFractureType_Voronoi == type)
	{
		BPPVoronoi& voronoi = fracture.voronoi;
		::init(voronoi);
	}
	else if (eFractureType_Slice == type)
	{
		BPPSlice& slice = fracture.slice;
		::init(slice);
	}
}

void FracturePreset::init()
{
	if (eFractureType_Voronoi == type)
	{
		BPPVoronoi& voronoi = fracture.voronoi;
		::init(voronoi);
	}
	else if (eFractureType_Slice == type)
	{
		BPPSlice& slice = fracture.slice;
		::init(slice);
	}

	::init(visualization);
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
	freeBlast(_projectParams.graphicsMaterials);
	_projectParams.fracture.general.applyMaterial = -1;
	_projectParams.fracture.general.autoSelectNewChunks = false;
	_projectParams.fracture.general.selectionDepthTest = true;
}

std::string BlastProject::getAseetNameByID(int assetID)
{
	BPPAssetArray& assetArray = _projectParams.blast.blastAssets;
	for (int i = 0; i < assetArray.arraySizes[0]; ++i)
	{
		if (assetArray.buf[i].ID == assetID)
			return assetArray.buf[i].name.buf;
	}
	return "";
}

int BlastProject::getAssetIDByName(const char* name)
{
	if (name == nullptr || strlen(name) == 0)
		return -1;

	BPPAssetArray& assetArray = _projectParams.blast.blastAssets;
	for (int i = 0; i < assetArray.arraySizes[0]; ++i)
	{
		if (nvidia::shdfnd::strcmp(assetArray.buf[i].name, name) == 0)
			return assetArray.buf[i].ID;
	}
	return -1;
}

BPPAsset* BlastProject::getAsset(const char* name)
{
	if (name == nullptr || strlen(name) == 0)
		return nullptr;

	BPPAssetArray& assetArray = _projectParams.blast.blastAssets;
	for (int i = 0; i < assetArray.arraySizes[0]; ++i)
	{
		if (nvidia::shdfnd::strcmp(assetArray.buf[i].name, name) == 0)
			return assetArray.buf + i;
	}
	return nullptr;
}

int BlastProject::generateNewAssetID()
{
	int id = 0;
	for (; id < (std::numeric_limits<int>::max)(); ++id)
	{
		BPPAssetArray& assetArray = _projectParams.blast.blastAssets;
		bool find = false;

		if (assetArray.arraySizes[0] == 0)
			find = false;

		for (int i = 0; i < assetArray.arraySizes[0]; ++i)
		{
			if (assetArray.buf[i].ID == id)
			{
				find = true;
				break;
			}
		}

		if (!find)
		{
			break;
		}
	}

	return id;
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

BPPGraphicsMaterial* BlastProject::addGraphicsMaterial(const char* name)
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
		init(newItem);
		copy(newItem, oldItem);
	}

	BPPGraphicsMaterial& newItem = theArray.buf[i];
	init(newItem);
	copy(newItem.name, name);
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
			init(newItem);
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

void BlastProject::reloadDiffuseColor(const char* name, float r, float g, float b, float a)
{
	if (name == nullptr)
		return;

	BPPGraphicsMaterialArray& theArray = _projectParams.graphicsMaterials;

	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		if (nvidia::shdfnd::strcmp(theArray.buf[i].name.buf, name) == 0)
		{
			theArray.buf[i].diffuseColor[0] = r;
			theArray.buf[i].diffuseColor[1] = g;
			theArray.buf[i].diffuseColor[2] = b;
			theArray.buf[i].diffuseColor[3] = a;
			return;
		}
	}
}

void BlastProject::reloadSpecularColor(const char* name, float r, float g, float b, float a)
{
	if (name == nullptr)
		return;

	BPPGraphicsMaterialArray& theArray = _projectParams.graphicsMaterials;

	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		if (nvidia::shdfnd::strcmp(theArray.buf[i].name.buf, name) == 0)
		{
			theArray.buf[i].specularColor[0] = r;
			theArray.buf[i].specularColor[1] = g;
			theArray.buf[i].specularColor[2] = b;
			theArray.buf[i].specularColor[3] = a;
			return;
		}
	}
}

void BlastProject::reloadSpecularShininess(const char* name, float specularShininess)
{
	if (name == nullptr)
		return;

	BPPGraphicsMaterialArray& theArray = _projectParams.graphicsMaterials;

	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		if (nvidia::shdfnd::strcmp(theArray.buf[i].name.buf, name) == 0)
		{
			theArray.buf[i].specularShininess = specularShininess;
			return;
		}
	}
}

void BlastProject::reloadDiffuseTexture(const char* name, const char* diffuseTexture)
{
	if (name == nullptr)
		return;

	BPPGraphicsMaterialArray& theArray = _projectParams.graphicsMaterials;

	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		if (nvidia::shdfnd::strcmp(theArray.buf[i].name.buf, name) == 0)
		{
			copy(theArray.buf[i].diffuseTextureFilePath, diffuseTexture);
			return;
		}
	}
}

void BlastProject::reloadSpecularTexture(const char* name, const char* specularTexture)
{
	if (name == nullptr)
		return;

	BPPGraphicsMaterialArray& theArray = _projectParams.graphicsMaterials;

	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		if (nvidia::shdfnd::strcmp(theArray.buf[i].name.buf, name) == 0)
		{
			copy(theArray.buf[i].specularTextureFilePath, specularTexture);
			return;
		}
	}
}

void BlastProject::reloadNormalTexture(const char* name, const char* normalTexture)
{
	if (name == nullptr)
		return;

	BPPGraphicsMaterialArray& theArray = _projectParams.graphicsMaterials;

	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		if (nvidia::shdfnd::strcmp(theArray.buf[i].name.buf, name) == 0)
		{
			copy(theArray.buf[i].normalTextureFilePath, normalTexture);
			return;
		}
	}
}

void BlastProject::reloadEnvTexture(const char* name, const char* envTexture)
{
	// to do
}

BPPGraphicsMaterial* BlastProject::getGraphicsMaterial(const char* name)
{
	if (name == nullptr || strlen(name) == 0)
		return nullptr;

	BPPGraphicsMaterialArray& theArray = _projectParams.graphicsMaterials;

	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		if (nvidia::shdfnd::strcmp(theArray.buf[i].name.buf, name) == 0)
		{
			return &theArray.buf[i];
		}
	}

	return nullptr;
}

std::string BlastProject::generateNewMaterialName(const char* name)
{
	std::string nName = "";
	if (name != nullptr)
		nName = name;

	char materialName[MAX_PATH];

	BPPGraphicsMaterialArray& theArray = _projectParams.graphicsMaterials;
	for (int m = 0; ;m++)
	{
		sprintf(materialName, "%s_%d", nName.c_str(), m);

		bool exist = false;
		for (int i = 0; i < theArray.arraySizes[0]; ++i)
		{
			BPPGraphicsMaterial& item = theArray.buf[i];
			if (nvidia::shdfnd::strcmp(item.name.buf, materialName) == 0)
			{
				exist = true;
				break;
			}
		}
		if (!exist)
		{
			break;
		}
	}

	return materialName;
}

bool BlastProject::isAssetInstanceNameExist(const char* name)
{
	if (name == nullptr || strlen(name) == 0)
		return false;

	BPPAssetInstanceArray& theArray = _projectParams.blast.blastAssetInstances;

	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		BPPAssetInstance& item = theArray.buf[i];
		if (nvidia::shdfnd::strcmp(item.name.buf, name) == 0)
			return true;
	}
	return false;
}

int BlastProject::getAssetInstanceCount(int assetID)
{
	std::vector<BPPAssetInstance*> instances;
	getAssetInstances(assetID, instances);
	return instances.size();
}

void BlastProject::getAssetInstances(int assetID, std::vector<BPPAssetInstance*>& instances)
{
	instances.clear();

	if (assetID < 0)
	{
		return;
	}

	/*
	assetID may not less than assetArray.arraySizes[0]
	for example : there is only one asset and its id is two

	BPPAssetArray& assetArray = _projectParams.blast.blastAssets;
	if (assetID >= assetArray.arraySizes[0])
	{
		return;
	}
	*/

	BPPAssetInstanceArray& instanceArray = _projectParams.blast.blastAssetInstances;
	for (int i = 0; i < instanceArray.arraySizes[0]; i++)
	{
		BPPAssetInstance& instance = instanceArray.buf[i];
		if (assetID == instance.asset)
			instances.push_back(&instance);
	}
}

BPPAssetInstance* BlastProject::getAssetInstance(int assetID, int instanceIndex)
{
	std::vector<BPPAssetInstance*> instances;
	getAssetInstances(assetID, instances);

	int instanceSize = instances.size();
	if (instanceSize == 0 || instanceSize <= instanceIndex)
	{
		return nullptr;
	}

	return instances[instanceIndex];
}

BPPAssetInstance* BlastProject::getAssetInstance(int assetID, const char* instanceName)
{
	std::vector<BPPAssetInstance*> instances;
	getAssetInstances(assetID, instances);

	int instanceSize = instances.size();
	if (instanceSize == 0)
	{
		return nullptr;
	}

	BPPAssetInstance* instance = nullptr;
	for (int is = 0; is < instanceSize; is++)
	{
		if (::strcmp(instanceName, instances[is]->name.buf) == 0)
		{
			instance = instances[is];
			break;
		}
	}
	return instance;
}

BPPAssetInstance* BlastProject::addAssetInstance(int blastAssetIndex, const char* instanceName)
{
	if (instanceName == nullptr)
		return nullptr;

	BPPAssetArray& assetArray = _projectParams.blast.blastAssets;
	if (blastAssetIndex < 0 && blastAssetIndex > assetArray.arraySizes[0])
		return nullptr;

	BPPAssetInstanceArray& theArray = _projectParams.blast.blastAssetInstances;

	BPPAssetInstance* oldBuf = theArray.buf;
	theArray.buf = new BPPAssetInstance[theArray.arraySizes[0] + 1];

	int i = 0;
	for (; i < theArray.arraySizes[0]; ++i)
	{
		BPPAssetInstance& newItem = theArray.buf[i];
		BPPAssetInstance& oldItem = oldBuf[i];

		init(newItem);
		copy(newItem, oldItem);
	}

	BPPAssetInstance& newItem = theArray.buf[i];
	init(newItem);
	copy(newItem.name, instanceName);
	newItem.asset = -1;
	newItem.visible = true;

	delete[] oldBuf;

	return &newItem;
}

void BlastProject::removeAssetInstance(const char* name)
{
	if (name == nullptr || strlen(name) == 0 || !isAssetInstanceNameExist(name))
		return;

	BPPAssetInstanceArray& theArray = _projectParams.blast.blastAssetInstances;
	BPPAssetInstance* oldBuf = theArray.buf;

	theArray.buf = new BPPAssetInstance[theArray.arraySizes[0] - 1];
	int index = 0;
	for (int i = 0; i < theArray.arraySizes[0]; ++i)
	{
		if (nvidia::shdfnd::strcmp(oldBuf[i].name.buf, name) != 0)
		{
			BPPAssetInstance& newItem = theArray.buf[index++];
			BPPAssetInstance& oldItem = oldBuf[i];
			init(newItem);
			copy(newItem, oldItem);
		}
	}
	theArray.arraySizes[0] -= 1;
	delete[] oldBuf;
}

BPPChunk* BlastProject::getChunk(BPPAsset& asset, int chunkID)
{
	BPPChunkArray& chunkArray = _projectParams.blast.chunks;

	int count = chunkArray.arraySizes[0];
	for (int i = 0; i < count; ++i)
	{
		BPPChunk& chunk = chunkArray.buf[i];
		if (chunk.ID == chunkID && chunk.asset == asset.ID)
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
		if (chunk.parentID == parentID && chunk.asset == asset.ID)
			chunks.push_back(&chunk);
	}

	return chunks;
}

std::vector<BPPChunk*> BlastProject::getChildrenChunks(BPPAsset& asset)
{
	return getChildrenChunks(asset.ID);
}

std::vector<BPPChunk*> BlastProject::getChildrenChunks(int assetID)
{
	std::vector<BPPChunk*> chunks;

	BPPChunkArray& chunkArray = _projectParams.blast.chunks;

	int count = chunkArray.arraySizes[0];
	for (int i = 0; i < count; ++i)
	{
		BPPChunk& chunk = chunkArray.buf[i];
		if (chunk.asset == assetID)
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
		if (bond.asset == asset.ID)
		{
			if (bond.fromChunk == chunkID)
				bonds.push_back(&bond);
			else if (bond.toChunk == chunkID)
				bonds.push_back(&bond);
		}
	}

	return bonds;
}

std::vector<BPPBond*> BlastProject::getChildrenBonds(BPPAsset& asset)
{
	std::vector<BPPBond*> bonds;

	BPPBondArray& bondArray = _projectParams.blast.bonds;

	int count = bondArray.arraySizes[0];
	for (int i = 0; i < count; ++i)
	{
		BPPBond& bond = bondArray.buf[i];
		if (bond.asset == asset.ID)
			bonds.push_back(&bond);
	}

	return bonds;
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
	QString presetFolder = QCoreApplication::applicationDirPath() + "/Preset/";
	QDir presetDir(presetFolder);
	QString presetFilePath = presetFolder + USER_PRESET;
	if (!presetDir.exists())
	{
		if (!presetDir.mkdir(presetFolder))
			return;
	}
	QFile file(presetFilePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		return;
	}
	QTextStream out(&file);

	QDomDocument    xmlDoc;
	QDomElement rootElm = xmlDoc.createElement(QObject::tr("UserPreset"));
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
	QString presetFilePath = QCoreApplication::applicationDirPath() + "/Preset/" + USER_PRESET;

	QFile file(presetFilePath);

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

	if (xmlDoc.isNull() || xmlDoc.documentElement().tagName() != QObject::tr("UserPreset"))
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

bool BlastProject::isFracturePresetNameExist(const char* name)
{
	if (name == nullptr || strlen(name) == 0)
		return false;

	for (size_t i = 0; i < _fracturePresets.size(); ++i)
	{
		FracturePreset& item = _fracturePresets[i];
		if (item.name == name)
			return true;
	}

	return false;
}

std::vector<FracturePreset>& BlastProject::getFracturePresets()
{
	return _fracturePresets;
}

void BlastProject::addFracturePreset(const char* name, FractureType type)
{
	_fracturePresets.push_back(FracturePreset(name, type));
}

void BlastProject::saveFracturePreset()
{
	QString presetFolder = QCoreApplication::applicationDirPath() + "/Preset/";
	QDir presetDir(presetFolder);
	QString presetFilePath = presetFolder + FRACTURE_PRESET;
	if (!presetDir.exists())
	{
		if (!presetDir.mkdir(presetFolder))
			return;
	}
	QFile file(presetFilePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		return;
	}
	QTextStream out(&file);

	QDomDocument    xmlDoc;
	QDomElement rootElm = xmlDoc.createElement(QObject::tr("FracturePresets"));
	xmlDoc.appendChild(rootElm);

	for (size_t i = 0; i < _fracturePresets.size(); ++i)
	{
		_saveFracturePreset(rootElm, _fracturePresets[i]);
	}

	// 4 is count of indent
	xmlDoc.save(out, 4);
}

void BlastProject::loadFracturePreset()
{
	QString presetFilePath = QCoreApplication::applicationDirPath() + "/Preset/" + FRACTURE_PRESET;
	QFile file(presetFilePath);

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

	if (xmlDoc.isNull() || xmlDoc.documentElement().tagName() != QObject::tr("FracturePresets"))
	{
		QMessageBox::warning(&AppMainWindow::Inst(), QObject::tr("Warning"), QObject::tr("The file you selected is empty or not a blast user preset file."));
		return;
	}

	QDomNodeList elms = xmlDoc.documentElement().elementsByTagName(QObject::tr("FracturePreset"));
	for (int i = 0; i < elms.count(); ++i)
	{
		FracturePreset preset("", eFractureType_Voronoi);
		_fracturePresets.push_back(preset);
		_loadFracturePreset(elms.at(i).toElement(), _fracturePresets[i]);
	}
}

bool BlastProject::isFilterPresetNameExist(const char* name)
{
	if (name == nullptr || strlen(name) == 0)
		return false;

	for (FilterPreset preset : _filterPresets)
	{
		if (preset.name == name)
			return true;
	}

	return false;
}

std::vector<FilterPreset>& BlastProject::getFilterPresets()
{
	return _filterPresets;
}

void BlastProject::addFilterPreset(const char* name)
{
	if (name == nullptr)
		return;

	_filterPresets.push_back(FilterPreset(name));
}

void BlastProject::removeFilterPreset(const char* name)
{
	if (name == nullptr || strlen(name) == 0 || !isFilterPresetNameExist(name))
		return;

	for (std::vector<FilterPreset>::iterator itr = _filterPresets.begin(); itr != _filterPresets.end(); ++itr)
	{
		if (itr->name == name)
		{
			_filterPresets.erase(itr);
			return;
		}
	}
}

FilterPreset* BlastProject::getFilterPreset(const char* name)
{
	if (name == nullptr)
		return nullptr;

	for (std::vector<FilterPreset>::iterator itr = _filterPresets.begin(); itr != _filterPresets.end(); ++itr)
	{
		if (itr->name == name)
		{
			return &(*itr);
		}
	}

	return nullptr;
}

void BlastProject::renameFilterPreset(const char* oldName, const char* newName)
{
	if (oldName == nullptr || newName == nullptr)
		return;

	for (std::vector<FilterPreset>::iterator itr = _filterPresets.begin(); itr != _filterPresets.end(); ++itr)
	{
		if (itr->name == oldName)
		{
			(*itr).name = newName;
			return;
		}
	}
}

void BlastProject::addFilterRestriction(const char* filterName, EFilterRestriction restriction)
{
	if (filterName == nullptr || strlen(filterName) == 0 || !isFilterPresetNameExist(filterName))
		return;

	for (std::vector<FilterPreset>::iterator itr = _filterPresets.begin(); itr != _filterPresets.end(); ++itr)
	{
		if (itr->name == filterName)
		{
			(*itr).filters.push_back(restriction);
			return;
		}
	}
}

void BlastProject::removeFilterRestriction(const char* filterName, EFilterRestriction restriction)
{
	if (filterName == nullptr || strlen(filterName) == 0 || !isFilterPresetNameExist(filterName))
		return;

	for (std::vector<FilterPreset>::iterator itr = _filterPresets.begin(); itr != _filterPresets.end(); ++itr)
	{
		if (itr->name == filterName)
		{
			std::vector<EFilterRestriction>& filters = (*itr).filters;
			filters.erase(std::find(filters.begin(), filters.end(), restriction));
			return;
		}
	}
}

void BlastProject::saveFilterPreset()
{
	QString presetFolder = QCoreApplication::applicationDirPath() + "/Preset/";
	QDir presetDir(presetFolder);
	QString presetFilePath = presetFolder + FILTER_PRESET;
	if (!presetDir.exists())
	{
		if (!presetDir.mkdir(presetFolder))
			return;
	}
	QFile file(presetFilePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		return;
	}
	QTextStream out(&file);

	QDomDocument    xmlDoc;
	QDomElement rootElm = xmlDoc.createElement(QObject::tr("FilterPresets"));
	xmlDoc.appendChild(rootElm);

	for (size_t i = 0; i < _filterPresets.size(); ++i)
	{
		_saveFilterPreset(rootElm, _filterPresets[i]);
	}

	// 4 is count of indent
	xmlDoc.save(out, 4);
}

void BlastProject::loadFilterPreset()
{
	QString presetFilePath = QCoreApplication::applicationDirPath() + "/Preset/" + FILTER_PRESET;
	QFile file(presetFilePath);

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

	if (xmlDoc.isNull() || xmlDoc.documentElement().tagName() != QObject::tr("FilterPresets"))
	{
		QMessageBox::warning(&AppMainWindow::Inst(), QObject::tr("Warning"), QObject::tr("The file you selected is empty or not a blast user preset file."));
		return;
	}

	QDomNodeList elms = xmlDoc.documentElement().elementsByTagName(QObject::tr("FilterPreset"));
	for (int i = 0; i < elms.count(); ++i)
	{
		FilterPreset preset("");
		_filterPresets.push_back(preset);
		_loadFilterPreset(elms.at(i).toElement(), _filterPresets[i]);
	}
}

BlastProject::BlastProject()
{
	_projectParams.renderer.textureFilePath.buf = nullptr;

	_projectParams.renderer.lights.buf = new BPPLight[4];
	_projectParams.renderer.lights.arraySizes[0] = 4;

	for (int i = 0; i < 4; ++i)
	{
		_projectParams.renderer.lights.buf[i].name.buf = nullptr;
	}

	_projectParams.blast.fileReferences.fbxSourceAsset.buf = nullptr;

	_projectParams.blast.healthMask.buf = nullptr;

	_projectParams.fracture.general.fracturePreset.buf = nullptr;

	init(_projectParams.fracture.slice);
	init(_projectParams.fracture.voronoi);

	init(_projectParams.defaultDamage);

	_projectParams.fracture.general.autoSelectNewChunks = false;
	_projectParams.fracture.general.selectionDepthTest = true;
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
	newElm.setAttribute(QObject::tr("Hardness"), stressSolver.hardness);
	newElm.setAttribute(QObject::tr("LinearFactor"), stressSolver.linearFactor);
	newElm.setAttribute(QObject::tr("AngularFactor"), stressSolver.angularFactor);
	newElm.setAttribute(QObject::tr("BondIterationsPerFrame"), stressSolver.bondIterationsPerFrame);
	newElm.setAttribute(QObject::tr("GraphReductionLevel"), stressSolver.graphReductionLevel);
}

void BlastProject::_loadStressSolverPreset(QDomElement& parentElm, StressSolverUserPreset& stressSolverUserPreset)
{
	stressSolverUserPreset.name = parentElm.attribute(QObject::tr("Name")).toUtf8().data();

	QDomElement stressSolverElm = parentElm.firstChildElement(QObject::tr("StressSolver"));
	_loadStressSolver(stressSolverElm, stressSolverUserPreset.stressSolver);
}

void BlastProject::_loadStressSolver(QDomElement& parentElm, BPPStressSolver& stressSolver)
{
	stressSolver.hardness = parentElm.attribute(QObject::tr("Hardness")).toFloat();
	stressSolver.linearFactor = parentElm.attribute(QObject::tr("LinearFactor")).toFloat();
	stressSolver.angularFactor = parentElm.attribute(QObject::tr("AngularFactor")).toFloat();
	stressSolver.bondIterationsPerFrame = parentElm.attribute(QObject::tr("BondIterationsPerFrame")).toUInt();
	stressSolver.graphReductionLevel = parentElm.attribute(QObject::tr("GraphReductionLevel")).toUInt();
}

void BlastProject::_saveFracturePreset(QDomElement& parentElm, FracturePreset& fracturePreset)
{
	QDomElement newElm = parentElm.ownerDocument().createElement(QObject::tr("FracturePreset"));
	parentElm.appendChild(newElm);
	newElm.setAttribute(QObject::tr("Name"), fracturePreset.name.c_str());

	if (eFractureType_Voronoi == fracturePreset.type)
	{
		_saveFracture(newElm, fracturePreset.fracture.voronoi);
	}
	else if (eFractureType_Slice == fracturePreset.type)
	{
		_saveFracture(newElm, fracturePreset.fracture.slice);
	}

	QDomElement visualizationElm = parentElm.ownerDocument().createElement(QObject::tr("Visualization"));
	newElm.appendChild(visualizationElm);
	visualizationElm.setAttribute(QObject::tr("FracturePreview"), fracturePreset.visualization.fracturePreview);
	visualizationElm.setAttribute(QObject::tr("DisplayFractureWidget"), fracturePreset.visualization.displayFractureWidget);
}

void BlastProject::_saveFracture(QDomElement& parentElm, BPPVoronoi& voronoi)
{
	QDomElement newElm = parentElm.ownerDocument().createElement(QObject::tr("Voronoi"));
	parentElm.appendChild(newElm);
	newElm.setAttribute(QObject::tr("SiteGeneration"), voronoi.siteGeneration);
	newElm.setAttribute(QObject::tr("NumSites"), voronoi.numSites);
	newElm.setAttribute(QObject::tr("NumberOfClusters"), voronoi.numberOfClusters);
	newElm.setAttribute(QObject::tr("SitesPerCluster"), voronoi.sitesPerCluster);
	newElm.setAttribute(QObject::tr("ClusterRadius"), voronoi.clusterRadius);
}

void BlastProject::_saveFracture(QDomElement& parentElm, BPPSlice& slice)
{
	QDomElement newElm = parentElm.ownerDocument().createElement(QObject::tr("Slice"));
	parentElm.appendChild(newElm);
	newElm.setAttribute(QObject::tr("NumSlicesX"), slice.numSlicesX);
	newElm.setAttribute(QObject::tr("NumSlicesY"), slice.numSlicesY);
	newElm.setAttribute(QObject::tr("NumSlicesZ"), slice.numSlicesZ);
	newElm.setAttribute(QObject::tr("OffsetVariation"), slice.offsetVariation);
	newElm.setAttribute(QObject::tr("RotationVariation"), slice.rotationVariation);
	newElm.setAttribute(QObject::tr("NoiseAmplitude"), slice.noiseAmplitude);
	newElm.setAttribute(QObject::tr("NoiseFrequency"), slice.noiseFrequency);
	newElm.setAttribute(QObject::tr("NoiseOctaveNumber"), slice.noiseOctaveNumber);
	newElm.setAttribute(QObject::tr("NoiseSeed"), slice.noiseSeed);
	newElm.setAttribute(QObject::tr("SurfaceResolution"), slice.surfaceResolution);
}

void BlastProject::_loadFracturePreset(QDomElement& parentElm, FracturePreset& fracturePreset)
{
	fracturePreset.name = parentElm.attribute(QObject::tr("Name")).toUtf8().data();

	QDomElement elm = parentElm.firstChildElement(QObject::tr("Voronoi"));
	if (!elm.isNull())
	{
		fracturePreset.type = eFractureType_Voronoi;
		_loadFracture(elm, fracturePreset.fracture.voronoi);
	}
	elm = parentElm.firstChildElement(QObject::tr("Slice"));
	if (!elm.isNull())
	{
		fracturePreset.type = eFractureType_Slice;
		_loadFracture(elm, fracturePreset.fracture.slice);
	}

	elm = parentElm.firstChildElement(QObject::tr("Visualization"));
	if (!elm.isNull())
	{
		/*
		std::string str0 = parentElm.attribute(QObject::tr("FracturePreview")).toStdString();
		std::string str1 = parentElm.attribute(QObject::tr("DisplayFractureWidget")).toStdString();
		uint val0 = parentElm.attribute(QObject::tr("FracturePreview")).toUInt();
		uint val1 = parentElm.attribute(QObject::tr("DisplayFractureWidget")).toUInt();
		*/
		fracturePreset.visualization.fracturePreview = elm.attribute(QObject::tr("FracturePreview")).toUInt();
		fracturePreset.visualization.displayFractureWidget = elm.attribute(QObject::tr("DisplayFractureWidget")).toUInt();
	}
}

void BlastProject::_loadFracture(QDomElement& parentElm, BPPVoronoi& voronoi)
{
	voronoi.siteGeneration = parentElm.attribute(QObject::tr("SiteGeneration")).toInt();
	voronoi.numSites = parentElm.attribute(QObject::tr("NumSites")).toUInt();
	voronoi.numberOfClusters = parentElm.attribute(QObject::tr("NumberOfClusters")).toUInt();
	voronoi.sitesPerCluster = parentElm.attribute(QObject::tr("SitesPerCluster")).toFloat();
	voronoi.clusterRadius = parentElm.attribute(QObject::tr("ClusterRadius")).toFloat();
}

void BlastProject::_loadFracture(QDomElement& parentElm, BPPSlice& slice)
{
	slice.numSlicesX = parentElm.attribute(QObject::tr("NumSlicesX")).toUInt();
	slice.numSlicesY = parentElm.attribute(QObject::tr("NumSlicesY")).toUInt();
	slice.numSlicesZ = parentElm.attribute(QObject::tr("NumSlicesZ")).toUInt();
	slice.offsetVariation = parentElm.attribute(QObject::tr("OffsetVariation")).toFloat();
	slice.rotationVariation = parentElm.attribute(QObject::tr("RotationVariation")).toFloat();
	slice.noiseAmplitude = parentElm.attribute(QObject::tr("NoiseAmplitude")).toFloat();
	slice.noiseFrequency = parentElm.attribute(QObject::tr("NoiseFrequency")).toFloat();
	slice.noiseOctaveNumber = parentElm.attribute(QObject::tr("NoiseOctaveNumber")).toUInt();
	slice.noiseSeed = parentElm.attribute(QObject::tr("NoiseSeed")).toUInt();
	slice.surfaceResolution = parentElm.attribute(QObject::tr("SurfaceResolution")).toUInt();
}

void BlastProject::_saveFilterPreset(QDomElement& parentElm, FilterPreset& filterPreset)
{
	QDomElement newElm = parentElm.ownerDocument().createElement(QObject::tr("FilterPreset"));
	parentElm.appendChild(newElm);
	newElm.setAttribute(QObject::tr("Name"), filterPreset.name.c_str());

	for (EFilterRestriction restriction : filterPreset.filters)
		_saveRestriction(newElm, restriction);
}

void BlastProject::_saveRestriction(QDomElement& parentElm, EFilterRestriction& restriction)
{
	QDomElement newElm = parentElm.ownerDocument().createElement(QObject::tr("Restriction"));
	parentElm.appendChild(newElm);
	newElm.setAttribute(QObject::tr("Value"), convertFilterRestrictionToString(restriction));
}

void BlastProject::_loadFilterPreset(QDomElement& parentElm, FilterPreset& filterPreset)
{
	filterPreset.name = parentElm.attribute(QObject::tr("Name")).toUtf8().data();

	QDomNodeList nodeList = parentElm.childNodes();
	for (int i = 0; i < nodeList.count(); ++i)
	{
		QDomNode& node = nodeList.at(i);
		QDomElement elm = node.toElement();
		if (elm.isNull() || elm.nodeName() != "Restriction")
			continue;

		EFilterRestriction restriction;
		_loadRestriction(elm, restriction);
		filterPreset.filters.push_back(restriction);
	}
}

void BlastProject::_loadRestriction(QDomElement& parentElm, EFilterRestriction& restriction)
{
	restriction = (EFilterRestriction)(convertStringToFilterRestriction(parentElm.attribute(QObject::tr("Value")).toUtf8().data()));
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

//	scene->Clear();

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
