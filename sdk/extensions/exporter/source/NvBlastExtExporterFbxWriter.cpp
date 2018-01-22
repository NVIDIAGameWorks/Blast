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


#include "fbxsdk.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <map>
#include <algorithm>
#include <set>
#include "NvBlastTypes.h"
#include "NvBlastGlobals.h"
#include "NvBlastTkFramework.h"
#include "NvBlast.h"
#include "PxVec3.h"
#include "NvBlastAssert.h"
#include <unordered_set>
#include <functional>
#include "NvBlastExtExporterFbxWriter.h"
#include "NvBlastExtExporterFbxUtils.h"
#include "NvBlastExtAuthoringCollisionBuilder.h"
#include "NvBlastExtAuthoring.h"
#include "NvBlastExtAuthoringMesh.h"

using namespace Nv::Blast;

FbxFileWriter::FbxFileWriter():
	bOutputFBXAscii(false)
{
	// Wrap in a shared ptr so that when it deallocates we get an auto destroy and all of the other assets created don't leak.
	sdkManager = std::shared_ptr<FbxManager>(FbxManager::Create(), [=](FbxManager* manager)
	{
		manager->Destroy();
	});

	mScene = FbxScene::Create(sdkManager.get(), "Export Scene");

	mScene->GetGlobalSettings().SetAxisSystem(FbxUtils::getBlastFBXAxisSystem());
	mScene->GetGlobalSettings().SetSystemUnit(FbxUtils::getBlastFBXUnit());
	mScene->GetGlobalSettings().SetOriginalUpAxis(FbxUtils::getBlastFBXAxisSystem());
	mScene->GetGlobalSettings().SetOriginalSystemUnit(FbxUtils::getBlastFBXUnit());

	//We don't actually check for membership in this layer, but it's useful to show and hide the geo to look at the collision geo
	mRenderLayer = FbxDisplayLayer::Create(mScene, FbxUtils::getRenderGeometryLayerName().c_str());
	mRenderLayer->Show.Set(true);
	mRenderLayer->Color.Set(FbxDouble3(0.0f, 1.0f, 0.0f));

	mInteriorIndex = -1;
}

void FbxFileWriter::release()
{
	//sdkManager->Destroy();
	delete this;
}

FbxScene* FbxFileWriter::getScene()
{
	return mScene;
}


void FbxFileWriter::createMaterials(const ExporterMeshData& aResult)
{
	mMaterials.clear();
	
	for (uint32_t i = 0; i < aResult.submeshCount; ++i)
	{
		FbxSurfacePhong* material = FbxSurfacePhong::Create(sdkManager.get(), aResult.submeshMats[i].name);
		material->Diffuse.Set(FbxDouble3(float(rand()) / RAND_MAX , float(rand()) / RAND_MAX, float(rand()) / RAND_MAX));
		material->DiffuseFactor.Set(1.0);
		mMaterials.push_back(material);
	}
}

void FbxFileWriter::setInteriorIndex(int32_t index)
{
	mInteriorIndex = index;
}


void FbxFileWriter::createMaterials(const AuthoringResult& aResult)
{
	mMaterials.clear();
	for (uint32_t i = 0; i < aResult.materialCount; ++i)
	{
		FbxSurfacePhong* material = FbxSurfacePhong::Create(sdkManager.get(), aResult.materialNames[i]);
		material->Diffuse.Set(FbxDouble3(float(rand()) / RAND_MAX, float(rand()) / RAND_MAX, float(rand()) / RAND_MAX));
		material->DiffuseFactor.Set(1.0);
		mMaterials.push_back(material);
	}
	if (mMaterials.size() == 0)
	{
		FbxSurfacePhong* material = FbxSurfacePhong::Create(sdkManager.get(), "Base_mat");
		material->Diffuse.Set(FbxDouble3(0.3, 1.0, 0));
		material->DiffuseFactor.Set(1.0);
		mMaterials.push_back(material);
	}
	if (mInteriorIndex == -1) // No material setted. Create new one.
	{
		FbxSurfacePhong* interiorMat = FbxSurfacePhong::Create(sdkManager.get(), "Interior_Material");
		interiorMat->Diffuse.Set(FbxDouble3(1.0, 0.0, 0.5));
		interiorMat->DiffuseFactor.Set(1.0);
		mMaterials.push_back(interiorMat);
	}
	else
	{
		if (mInteriorIndex < 0) mInteriorIndex = 0;
		if (static_cast<size_t>(mInteriorIndex) >= mMaterials.size()) mInteriorIndex = 0;
	}

}


bool FbxFileWriter::appendMesh(const AuthoringResult& aResult, const char* assetName, bool nonSkinned)
{
	createMaterials(aResult);

	if (nonSkinned)
	{
		return appendNonSkinnedMesh(aResult, assetName);
	}
	std::string meshName(assetName); meshName.append("_rendermesh");

	FbxMesh* mesh = FbxMesh::Create(sdkManager.get(), meshName.c_str());

	FbxGeometryElementNormal* geNormal = mesh->CreateElementNormal();
	geNormal->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
	geNormal->SetReferenceMode(FbxGeometryElement::eDirect);

	FbxGeometryElementUV* geUV = mesh->CreateElementUV("diffuseElement");
	geUV->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
	geUV->SetReferenceMode(FbxGeometryElement::eDirect);

	FbxGeometryElementSmoothing* smElement = nullptr;
	size_t triangleCount = aResult.geometryOffset[aResult.chunkCount];

	for (size_t triangle = 0; triangle < triangleCount; triangle++)
	{
		if (aResult.geometry[triangle].smoothingGroup >= 0)
		{
			//Found a valid smoothing group
			smElement = mesh->CreateElementSmoothing();
			smElement->SetMappingMode(FbxGeometryElement::eByPolygon);
			smElement->SetReferenceMode(FbxGeometryElement::eDirect);
			break;
		}
	}
	
	mesh->InitControlPoints((int)triangleCount * 3);


	FbxNode* meshNode = FbxNode::Create(mScene, assetName);
	meshNode->SetNodeAttribute(mesh);
	meshNode->SetShadingMode(FbxNode::eTextureShading);

	mRenderLayer->AddMember(meshNode);

	for (uint32_t i = 0; i < mMaterials.size(); ++i)
	{
		meshNode->AddMaterial(mMaterials[i]);
	}

	FbxNode* lRootNode = mScene->GetRootNode();

	//In order for Maya to correctly convert the axis of a skinned model there must be a common root node between the skeleton and the model
	FbxNode* sceneRootNode = FbxNode::Create(sdkManager.get(), "sceneRoot");
	lRootNode->AddChild(sceneRootNode);
	sceneRootNode->AddChild(meshNode);

	//UE4 cannot hide the root bone, so add a dummy chunk so chunk0 is not the root
	FbxNode* skelRootNode = FbxNode::Create(sdkManager.get(), "root");
	FbxSkeleton* skelAttrib = FbxSkeleton::Create(sdkManager.get(), "SkelRootAttrib");
	skelAttrib->SetSkeletonType(FbxSkeleton::eRoot);
	skelRootNode->SetNodeAttribute(skelAttrib);

	sceneRootNode->AddChild(skelRootNode);

	FbxSkin* skin = FbxSkin::Create(sdkManager.get(), "Skin of the thing");
	skin->SetGeometry(mesh);
	mesh->AddDeformer(skin);
	
	// Add a material otherwise UE4 freaks out on import

	FbxGeometryElementMaterial* matElement = mesh->CreateElementMaterial();
	matElement->SetMappingMode(FbxGeometryElement::eByPolygon);
	matElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

	// Now walk the tree and create a skeleton with geometry at the same time
	// Find a "root" chunk and walk the tree from there.
	uint32_t chunkCount = NvBlastAssetGetChunkCount(aResult.asset, Nv::Blast::logLL);
	auto chunks = NvBlastAssetGetChunks(aResult.asset, Nv::Blast::logLL);

	uint32_t cpIdx = 0;
	for (uint32_t i = 0; i < chunkCount; i++)
	{
		auto& chunk = chunks[i];

		if (chunk.parentChunkIndex == UINT32_MAX)
		{
			uint32_t addedCps = createChunkRecursive(cpIdx, i, meshNode, skelRootNode, skin, aResult);
			cpIdx += addedCps;
		}
	}

	if (!smElement)
	{
		//If no smoothing groups, generate them
		generateSmoothingGroups(mesh, skin);
	}

	removeDuplicateControlPoints(mesh, skin);

	if (aResult.collisionHull != nullptr)
	{
		return appendCollisionMesh(chunkCount, aResult.collisionHullOffset, aResult.collisionHull, assetName);
	}

	return true;
};


bool FbxFileWriter::appendNonSkinnedMesh(const AuthoringResult& aResult, const char* assetName)
{
	FbxNode* lRootNode = mScene->GetRootNode();

	//UE4 cannot hide the root bone, so add a dummy chunk so chunk0 is not the root
	FbxNode* skelRootNode = FbxNode::Create(sdkManager.get(), "root");
	//UE4 needs this to be a skeleton node, null node, or mesh node to get used
	FbxNull* nullAttr = FbxNull::Create(sdkManager.get(), "SkelRootAttrib");
	skelRootNode->SetNodeAttribute(nullAttr);
	lRootNode->AddChild(skelRootNode);

	// Now walk the tree and create a skeleton with geometry at the same time
	// Find a "root" chunk and walk the tree from there.
	uint32_t chunkCount = NvBlastAssetGetChunkCount(aResult.asset, Nv::Blast::logLL);
	auto chunks = NvBlastAssetGetChunks(aResult.asset, Nv::Blast::logLL);

	for (uint32_t i = 0; i < chunkCount; i++)
	{
		auto& chunk = chunks[i];

		if (chunk.parentChunkIndex == UINT32_MAX)
		{
			createChunkRecursiveNonSkinned(assetName, i, skelRootNode, mMaterials, aResult);
		}
	}

	if (aResult.collisionHull != nullptr)
	{
		return appendCollisionMesh(chunkCount, aResult.collisionHullOffset, aResult.collisionHull, assetName);
	}

	return true;
}


bool FbxFileWriter::appendNonSkinnedMesh(const ExporterMeshData& meshData, const char* assetName)
{
	FbxNode* lRootNode = mScene->GetRootNode();

	//UE4 cannot hide the root bone, so add a dummy chunk so chunk0 is not the root
	FbxNode* skelRootNode = FbxNode::Create(sdkManager.get(), "root");
	//UE4 needs this to be a skeleton node, null node, or mesh node to get used
	FbxNull* nullAttr = FbxNull::Create(sdkManager.get(), "SkelRootAttrib");
	skelRootNode->SetNodeAttribute(nullAttr);
	lRootNode->AddChild(skelRootNode);

	// Now walk the tree and create a skeleton with geometry at the same time
	// Find a "root" chunk and walk the tree from there.
	uint32_t chunkCount = NvBlastAssetGetChunkCount(meshData.asset, Nv::Blast::logLL);

	auto chunks = NvBlastAssetGetChunks(meshData.asset, Nv::Blast::logLL);

	for (uint32_t i = 0; i < chunkCount; i++)
	{
		const NvBlastChunk* chunk = &chunks[i];

		if (chunk->parentChunkIndex == UINT32_MAX)
		{
			createChunkRecursiveNonSkinned("chunk", i, skelRootNode, mMaterials, meshData);
		}
	}
	if (meshData.hulls != nullptr)
	{
		return appendCollisionMesh(chunkCount, meshData.hullsOffsets, meshData.hulls, assetName);
	}
	return true;
}

bool FbxFileWriter::appendCollisionMesh(uint32_t meshCount, uint32_t* offsets, CollisionHull** hulls, const char* assetName)
{
	FbxDisplayLayer* displayLayer = FbxDisplayLayer::Create(mScene, FbxUtils::getCollisionGeometryLayerName().c_str());
	//Hide by default
	displayLayer->Show.Set(false);
	displayLayer->Color.Set(FbxDouble3(0.0f, 0.0f, 1.0f));

	// Now walk the tree and create a skeleton with geometry at the same time
	// Find a "root" chunk and walk the tree from there.

	for (uint32_t i = 0; i < meshCount; i++)
	{
		auto findIt = chunkNodes.find(i);
		if (findIt == chunkNodes.end())
		{
			std::cerr << "Warning: No chunk node for chunk " << i << ". Ignoring collision geo" << std::endl;
			continue;
		}
		addCollisionHulls(i, displayLayer, findIt->second, offsets[i+1] - offsets[i], hulls + offsets[i]);
	}
	return true;
}

/*
	Recursive method that creates this chunk and all it's children.

	This creates a FbxNode with an FbxCluster, and all of the geometry for this chunk.

	Returns the number of added control points
*/
uint32_t FbxFileWriter::createChunkRecursive(uint32_t currentCpIdx, uint32_t chunkIndex, FbxNode *meshNode, FbxNode* parentNode, FbxSkin* skin, const AuthoringResult& aResult)
{

	auto chunks = NvBlastAssetGetChunks(aResult.asset, Nv::Blast::logLL);
	const NvBlastChunk* chunk = &chunks[chunkIndex];
	physx::PxVec3 centroid = physx::PxVec3(chunk->centroid[0], chunk->centroid[1], chunk->centroid[2]);

	//mesh->InitTextureUV(triangles.size() * 3);

	std::string boneName = FbxUtils::getChunkNodeName(chunkIndex);

	FbxSkeleton* skelAttrib = FbxSkeleton::Create(sdkManager.get(), boneName.c_str());
	if (chunk->parentChunkIndex == UINT32_MAX)
	{
		skelAttrib->SetSkeletonType(FbxSkeleton::eRoot);

		// Change the centroid to origin
		centroid = physx::PxVec3(0.0f);
	}
	else
	{
		skelAttrib->SetSkeletonType(FbxSkeleton::eLimbNode);
		worldChunkPivots[chunkIndex] = centroid;
	}

	skelAttrib->Size.Set(1.0); // What's this for?


	FbxNode* boneNode = FbxNode::Create(sdkManager.get(), boneName.c_str());
	boneNode->SetNodeAttribute(skelAttrib);

	chunkNodes[chunkIndex] = boneNode;

	auto mat = parentNode->EvaluateGlobalTransform().Inverse();

	FbxVector4 vec(0, 0, 0, 0);
	FbxVector4 c2 = mat.MultT(vec);

	boneNode->LclTranslation.Set(c2);
	
	parentNode->AddChild(boneNode);

	std::ostringstream namestream;
	namestream << "cluster_" << std::setw(5) << std::setfill('0') << chunkIndex;
	std::string clusterName = namestream.str();

	FbxCluster* cluster = FbxCluster::Create(sdkManager.get(), clusterName.c_str());
	cluster->SetTransformMatrix(FbxAMatrix());
	cluster->SetLink(boneNode);
	cluster->SetLinkMode(FbxCluster::eTotalOne);

	skin->AddCluster(cluster);

	FbxMesh* mesh = static_cast<FbxMesh*>(meshNode->GetNodeAttribute());

	FbxVector4* controlPoints = mesh->GetControlPoints();
	auto geNormal = mesh->GetElementNormal();
	auto geUV = mesh->GetElementUV("diffuseElement");
	FbxGeometryElementMaterial* matElement = mesh->GetElementMaterial();
	FbxGeometryElementSmoothing* smElement = mesh->GetElementSmoothing();

	auto addVert = [&](Nv::Blast::Vertex vert, int controlPointIdx)
	{
		FbxVector4 vertex;
		FbxVector4 normal;
		FbxVector2 uv;

		FbxUtils::VertexToFbx(vert, vertex, normal, uv);

		controlPoints[controlPointIdx] = vertex;
		geNormal->GetDirectArray().Add(normal);
		geUV->GetDirectArray().Add(uv);
		// Add this control point to the bone with weight 1.0
		cluster->AddControlPointIndex(controlPointIdx, 1.0);
	};

	uint32_t cpIdx = 0;
	uint32_t polyCount = mesh->GetPolygonCount();
	for (uint32_t i = aResult.geometryOffset[chunkIndex]; i < aResult.geometryOffset[chunkIndex + 1]; i++)
	{
		Triangle& tri = aResult.geometry[i];
		addVert(tri.a, currentCpIdx + cpIdx + 0);
		addVert(tri.b, currentCpIdx + cpIdx + 1);
		addVert(tri.c, currentCpIdx + cpIdx + 2);

		mesh->BeginPolygon();
		mesh->AddPolygon(currentCpIdx + cpIdx + 0);
		mesh->AddPolygon(currentCpIdx + cpIdx + 1);
		mesh->AddPolygon(currentCpIdx + cpIdx + 2);
		mesh->EndPolygon();
		int32_t material = (tri.materialId != MATERIAL_INTERIOR) ? ((tri.materialId < int32_t(mMaterials.size())) ? tri.materialId : 0) : ((mInteriorIndex == -1) ? int32_t(mMaterials.size() - 1): mInteriorIndex);
		matElement->GetIndexArray().SetAt(polyCount, material);
		if (smElement)
		{
			if (tri.userData == 0)
			{
				smElement->GetDirectArray().Add(tri.smoothingGroup);
			}
			else
			{
				smElement->GetDirectArray().Add(SMOOTHING_GROUP_INTERIOR);
			}
		}
		
		polyCount++;
		cpIdx += 3;
	}
		
	mat = meshNode->EvaluateGlobalTransform();
	cluster->SetTransformMatrix(mat);

	mat = boneNode->EvaluateGlobalTransform();
	cluster->SetTransformLinkMatrix(mat);

	uint32_t addedCps = static_cast<uint32_t>((aResult.geometryOffset[chunkIndex + 1] - aResult.geometryOffset[chunkIndex]) * 3);

	for (uint32_t i = chunk->firstChildIndex; i < chunk->childIndexStop; i++)
	{
		addedCps += createChunkRecursive(currentCpIdx + addedCps, i, meshNode, boneNode, skin, aResult);
	}

	return addedCps;
}


void FbxFileWriter::createChunkRecursiveNonSkinned(const std::string& meshName, uint32_t chunkIndex, FbxNode* parentNode,
	const std::vector<FbxSurfaceMaterial*>& materials, const ExporterMeshData& meshData)
{
	auto chunks = NvBlastAssetGetChunks(meshData.asset, Nv::Blast::logLL);
	const NvBlastChunk* chunk = &chunks[chunkIndex];
	physx::PxVec3 centroid = physx::PxVec3(chunk->centroid[0], chunk->centroid[1], chunk->centroid[2]);

	std::string chunkName = FbxUtils::getChunkNodeName(chunkIndex);

	FbxMesh* mesh = FbxMesh::Create(sdkManager.get(), (chunkName + "_mesh").c_str());

	FbxNode* meshNode = FbxNode::Create(mScene, chunkName.c_str());
	meshNode->SetNodeAttribute(mesh);
	meshNode->SetShadingMode(FbxNode::eTextureShading);
	mRenderLayer->AddMember(meshNode);

	chunkNodes[chunkIndex] = meshNode;

	auto mat = parentNode->EvaluateGlobalTransform().Inverse();

	FbxVector4 c2 = mat.MultT(FbxVector4(centroid.x, centroid.y, centroid.z, 1.0f));
	if (chunk->parentChunkIndex != UINT32_MAX)
	{
		//Don't mess with the root chunk pivot
		meshNode->LclTranslation.Set(c2);
		worldChunkPivots[chunkIndex] = centroid;
	}
	
	parentNode->AddChild(meshNode);
	FbxAMatrix finalXForm = meshNode->EvaluateGlobalTransform();

	//Set the geo transform to inverse so we can use the world mesh coordinates
	FbxAMatrix invFinalXForm = finalXForm.Inverse();
	meshNode->SetGeometricTranslation(FbxNode::eSourcePivot, invFinalXForm.GetT());
	meshNode->SetGeometricRotation(FbxNode::eSourcePivot, invFinalXForm.GetR());
	meshNode->SetGeometricScaling(FbxNode::eSourcePivot, invFinalXForm.GetS());

	auto geNormal = mesh->CreateElementNormal();
	auto geUV = mesh->CreateElementUV("diffuseElement");
	auto matr = mesh->CreateElementMaterial();

	uint32_t* firstIdx = meshData.submeshOffsets + chunkIndex * meshData.submeshCount;
	uint32_t* lastIdx  = meshData.submeshOffsets + (chunkIndex + 1) * meshData.submeshCount;
	uint32_t cpCount = *lastIdx - *firstIdx;
	mesh->InitControlPoints(cpCount);

	geNormal->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
	geNormal->SetReferenceMode(FbxGeometryElement::eDirect);

	geUV->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
	geUV->SetReferenceMode(FbxGeometryElement::eDirect);

	matr->SetMappingMode(FbxGeometryElement::eByPolygon);
	matr->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

	for (auto m : materials)
	{
		meshNode->AddMaterial(m);
	}

	uint32_t cPolygCount = 0;
	int32_t addedVertices = 0;
	
	for (uint32_t subMesh = 0; subMesh < meshData.submeshCount; ++subMesh)
	{
		for (uint32_t tr = *(firstIdx + subMesh); tr < *(firstIdx + subMesh + 1); tr += 3)
		{
			mesh->BeginPolygon(subMesh);
			for (uint32_t k = 0; k < 3; ++k)
			{
				mesh->AddPolygon(tr - *firstIdx + k);

				FbxVector4 temp;
				FbxUtils::PxVec3ToFbx(meshData.positions[meshData.posIndex[tr + k]], temp);
				mesh->SetControlPointAt(temp, tr - *firstIdx + k);

				FbxUtils::PxVec3ToFbx(meshData.normals[meshData.normIndex[tr + k]], temp);
				geNormal->GetDirectArray().Add(temp);
				
				FbxVector2 temp2;
				FbxUtils::PxVec2ToFbx(meshData.uvs[meshData.texIndex[tr + k]], temp2);
				geUV->GetDirectArray().Add(temp2);
			}
			mesh->EndPolygon();
			cPolygCount++;
			addedVertices += 3;
		}
	}

	if (!mesh->GetElementSmoothing())
	{
		//If no smoothing groups, generate them
		generateSmoothingGroups(mesh, nullptr);
	}

	removeDuplicateControlPoints(mesh, nullptr);

	for (uint32_t i = chunk->firstChildIndex; i < chunk->childIndexStop; i++)
	{
		createChunkRecursiveNonSkinned(meshName, i, meshNode, materials, meshData);
	}
}


void FbxFileWriter::createChunkRecursiveNonSkinned(const std::string& meshName, uint32_t chunkIndex, FbxNode* parentNode, const std::vector<FbxSurfaceMaterial*>& materials, const AuthoringResult& aResult)
{
	auto chunks = NvBlastAssetGetChunks(aResult.asset, Nv::Blast::logLL);
	const NvBlastChunk* chunk = &chunks[chunkIndex];
	physx::PxVec3 centroid = physx::PxVec3(chunk->centroid[0], chunk->centroid[1], chunk->centroid[2]);

	std::string chunkName = FbxUtils::getChunkNodeName(chunkIndex).c_str();

	FbxMesh* mesh = FbxMesh::Create(sdkManager.get(), (chunkName + "_mesh").c_str());

	FbxNode* meshNode = FbxNode::Create(mScene, chunkName.c_str());
	meshNode->SetNodeAttribute(mesh);
	meshNode->SetShadingMode(FbxNode::eTextureShading);
	mRenderLayer->AddMember(meshNode);

	chunkNodes[chunkIndex] = meshNode;

	auto mat = parentNode->EvaluateGlobalTransform().Inverse();

	FbxVector4 c2 = mat.MultT(FbxVector4(centroid.x, centroid.y, centroid.z, 1.0f));

	if (chunk->parentChunkIndex != UINT32_MAX)
	{
		//Don't mess with the root chunk pivot
		meshNode->LclTranslation.Set(c2);
		worldChunkPivots[chunkIndex] = centroid;
	}

	parentNode->AddChild(meshNode);
	FbxAMatrix finalXForm = meshNode->EvaluateGlobalTransform();

	//Set the geo transform to inverse so we can use the world mesh coordinates
	FbxAMatrix invFinalXForm = finalXForm.Inverse();
	meshNode->SetGeometricTranslation(FbxNode::eSourcePivot, invFinalXForm.GetT());
	meshNode->SetGeometricRotation(FbxNode::eSourcePivot, invFinalXForm.GetR());
	meshNode->SetGeometricScaling(FbxNode::eSourcePivot, invFinalXForm.GetS());


	auto geNormal = mesh->CreateElementNormal();
	auto geUV = mesh->CreateElementUV("diffuseElement");
	auto matr = mesh->CreateElementMaterial();

	uint32_t firstIdx = aResult.geometryOffset[chunkIndex];
	uint32_t lastIdx  = aResult.geometryOffset[chunkIndex + 1];

	FbxGeometryElementSmoothing* smElement = nullptr;
	for (uint32_t triangle = firstIdx; triangle < lastIdx; triangle++)
	{
		if (aResult.geometry[triangle].smoothingGroup >= 0)
		{
			//Found a valid smoothing group
			smElement = mesh->CreateElementSmoothing();
			smElement->SetMappingMode(FbxGeometryElement::eByPolygon);
			smElement->SetReferenceMode(FbxGeometryElement::eDirect);
			break;
		}
	}

	mesh->InitControlPoints((int)(lastIdx - firstIdx) * 3);

	geNormal->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
	geNormal->SetReferenceMode(FbxGeometryElement::eDirect);

	geUV->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
	geUV->SetReferenceMode(FbxGeometryElement::eDirect);

	matr->SetMappingMode(FbxGeometryElement::eByPolygon);
	matr->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

	for (auto m : materials)
	{
		meshNode->AddMaterial(m);
	}

	FbxGeometryElementMaterial* matElement = mesh->GetElementMaterial();
	int32_t polyCount = 0;
	for (uint32_t tr = firstIdx; tr < lastIdx; tr++)
	{
		auto& geo = aResult.geometry[tr];
		const Nv::Blast::Vertex triVerts[3] = { geo.a, geo.b, geo.c };
		mesh->BeginPolygon();
		for (uint32_t k = 0; k < 3; ++k)
		{
			mesh->AddPolygon(tr * 3 + k);
			FbxVector4 v, n;
			FbxVector2 uv;
			FbxUtils::VertexToFbx(triVerts[k], v, n, uv);
			mesh->SetControlPointAt(v, tr * 3 + k);

			geNormal->GetDirectArray().Add(n);
			geUV->GetDirectArray().Add(uv);
		}
		mesh->EndPolygon();
		int32_t material = (geo.materialId != MATERIAL_INTERIOR) ? ((geo.materialId < int32_t(mMaterials.size()))? geo.materialId : 0) : ((mInteriorIndex == -1)? int32_t(mMaterials.size() - 1) : mInteriorIndex);
		matElement->GetIndexArray().SetAt(polyCount, material);

		if (smElement)
		{
			if (geo.userData == 0)
			{
				smElement->GetDirectArray().Add(geo.smoothingGroup);
			}
			else
			{
				smElement->GetDirectArray().Add(SMOOTHING_GROUP_INTERIOR);
			}
		}

		polyCount++;		

	}

	if (!smElement)
	{
		//If no smoothing groups, generate them
		generateSmoothingGroups(mesh, nullptr);

	}

	removeDuplicateControlPoints(mesh, nullptr);

	for (uint32_t i = chunk->firstChildIndex; i < chunk->childIndexStop; i++)
	{
		createChunkRecursiveNonSkinned(meshName, i, meshNode, materials, aResult);
	}
}

uint32_t FbxFileWriter::addCollisionHulls(uint32_t chunkIndex, FbxDisplayLayer* displayLayer, FbxNode* parentNode, uint32_t hullsCount, CollisionHull** hulls)
{
	for (uint32_t hullId = 0; hullId < hullsCount; ++hullId)
	{
		std::stringstream namestream;
		namestream.clear();
		namestream << "collisionHull_" << chunkIndex << "_"  << hullId;

		FbxNode* collisionNode = FbxNode::Create(sdkManager.get(), namestream.str().c_str());

		displayLayer->AddMember(collisionNode);

		//TODO: Remove this when tools are converted over
		FbxProperty::Create(collisionNode, FbxIntDT, "ParentalChunkIndex");
		collisionNode->FindProperty("ParentalChunkIndex").Set(chunkIndex);
		//

		namestream.clear();
		namestream << "collisionHullGeom_" << chunkIndex << "_" << hullId;
		FbxMesh* meshAttr = FbxMesh::Create(sdkManager.get(), namestream.str().c_str());
		collisionNode->SetNodeAttribute(meshAttr);
		parentNode->AddChild(collisionNode);
		
		auto mat = parentNode->EvaluateGlobalTransform().Inverse();
		auto centroid = worldChunkPivots.find(chunkIndex);
		
		if (centroid != worldChunkPivots.end())
		{
			FbxVector4 c2 = mat.MultT(FbxVector4(centroid->second.x, centroid->second.y, centroid->second.z, 1.0f));
			//Don't mess with the root chunk pivot
			collisionNode->LclTranslation.Set(c2);
		}
		parentNode->AddChild(collisionNode);
		FbxAMatrix finalXForm = collisionNode->EvaluateGlobalTransform();

		//Set the geo transform to inverse so we can use the world mesh coordinates
		FbxAMatrix invFinalXForm = finalXForm.Inverse();
		collisionNode->SetGeometricTranslation(FbxNode::eSourcePivot, invFinalXForm.GetT());
		collisionNode->SetGeometricRotation(FbxNode::eSourcePivot, invFinalXForm.GetR());
		collisionNode->SetGeometricScaling(FbxNode::eSourcePivot, invFinalXForm.GetS());


		meshAttr->InitControlPoints(hulls[hullId]->pointsCount);
		meshAttr->CreateElementNormal();
		FbxVector4* controlPoints = meshAttr->GetControlPoints();
		auto geNormal = meshAttr->GetElementNormal();
		geNormal->SetMappingMode(FbxGeometryElement::eByPolygon);
		geNormal->SetReferenceMode(FbxGeometryElement::eDirect);
		for (uint32_t i = 0; i < hulls[hullId]->pointsCount; ++i)
		{
			auto& pnts = hulls[hullId]->points[i];
			controlPoints->Set(pnts.x, pnts.y, pnts.z, 0.0);
			controlPoints++;
		}

		for (uint32_t i = 0; i < hulls[hullId]->polygonDataCount; ++i)
		{
			auto& poly = hulls[hullId]->polygonData[i];
			meshAttr->BeginPolygon();
			for (uint32_t j = 0; j < poly.mNbVerts; ++j)
			{				
				meshAttr->AddPolygon(hulls[hullId]->indices[poly.mIndexBase + j]);
			}
			meshAttr->EndPolygon();
			FbxVector4 plane(poly.mPlane[0], poly.mPlane[1], poly.mPlane[2], 0);
			geNormal->GetDirectArray().Add(plane);
		}		
	}	
	return 1;
}

uint32_t FbxFileWriter::createChunkRecursive(uint32_t currentCpIdx, uint32_t chunkIndex, FbxNode *meshNode, FbxNode* parentNode, FbxSkin* skin, const ExporterMeshData& meshData)
{
		auto chunks = NvBlastAssetGetChunks(meshData.asset, Nv::Blast::logLL);
		const NvBlastChunk* chunk = &chunks[chunkIndex];
		physx::PxVec3 centroid = physx::PxVec3(chunk->centroid[0], chunk->centroid[1], chunk->centroid[2]);

		std::string boneName = FbxUtils::getChunkNodeName(chunkIndex).c_str();

		FbxSkeleton* skelAttrib = FbxSkeleton::Create(sdkManager.get(), boneName.c_str());
		if (chunk->parentChunkIndex == UINT32_MAX)
		{
			skelAttrib->SetSkeletonType(FbxSkeleton::eRoot);

			// Change the centroid to origin
			centroid = physx::PxVec3(0.0f);
		}
		else
		{
			skelAttrib->SetSkeletonType(FbxSkeleton::eLimbNode);
			worldChunkPivots[chunkIndex] = centroid;
		}

		FbxNode* boneNode = FbxNode::Create(sdkManager.get(), boneName.c_str());
		boneNode->SetNodeAttribute(skelAttrib);

		chunkNodes[chunkIndex] = boneNode;
		
		auto mat = parentNode->EvaluateGlobalTransform().Inverse();

		FbxVector4 vec(0, 0, 0, 0);
		FbxVector4 c2 = mat.MultT(vec);

		boneNode->LclTranslation.Set(c2);

		parentNode->AddChild(boneNode);

		std::ostringstream namestream;
		namestream << "cluster_" << std::setw(5) << std::setfill('0') << chunkIndex;
		std::string clusterName = namestream.str();

		FbxCluster* cluster = FbxCluster::Create(sdkManager.get(), clusterName.c_str());
		cluster->SetTransformMatrix(FbxAMatrix());
		cluster->SetLink(boneNode);
		cluster->SetLinkMode(FbxCluster::eTotalOne);

		skin->AddCluster(cluster);

		FbxMesh* mesh = static_cast<FbxMesh*>(meshNode->GetNodeAttribute());

		auto geNormal = mesh->GetElementNormal();
		auto geUV = mesh->GetElementUV("diffuseElement");
		auto matr = mesh->GetElementMaterial();

		std::vector<bool> addedVerticesFlag(mesh->GetControlPointsCount(), false);

		uint32_t* firstIdx = meshData.submeshOffsets + chunkIndex * meshData.submeshCount;
		uint32_t cPolygCount = mesh->GetPolygonCount();
		int32_t addedVertices = 0;
		for (uint32_t subMesh = 0; subMesh < meshData.submeshCount; ++subMesh)
		{			
			for (uint32_t tr = *(firstIdx + subMesh); tr < *(firstIdx + subMesh + 1); tr += 3)
			{
				mesh->BeginPolygon(subMesh);
				mesh->AddPolygon(meshData.posIndex[tr + 0]);
				mesh->AddPolygon(meshData.posIndex[tr + 1]);
				mesh->AddPolygon(meshData.posIndex[tr + 2]);
				mesh->EndPolygon();
				for (uint32_t k = 0; k < 3; ++k)
				{
					geNormal->GetIndexArray().SetAt(currentCpIdx + addedVertices + k, meshData.normIndex[tr + k]);
					geUV->GetIndexArray().SetAt(currentCpIdx + addedVertices + k, meshData.texIndex[tr + k]);
				}
				if (subMesh == 0)
				{
					matr->GetIndexArray().SetAt(cPolygCount, 0);
				}
				else
				{
					matr->GetIndexArray().SetAt(cPolygCount, 1);
				}
				cPolygCount++;
				addedVertices += 3;
				for (uint32_t k = 0; k < 3; ++k)
				{
					if (!addedVerticesFlag[meshData.posIndex[tr + k]])
					{
						cluster->AddControlPointIndex(meshData.posIndex[tr + k], 1.0);
						addedVerticesFlag[meshData.posIndex[tr + k]] = true;
					}
				}
			}
		}
		mat = meshNode->EvaluateGlobalTransform();
		cluster->SetTransformMatrix(mat);

		mat = boneNode->EvaluateGlobalTransform();
		cluster->SetTransformLinkMatrix(mat);


		for (uint32_t i = chunk->firstChildIndex; i < chunk->childIndexStop; i++)
		{
			 addedVertices += createChunkRecursive(currentCpIdx + addedVertices, i, meshNode, boneNode, skin, meshData);
		}

		return addedVertices;
	
}

void FbxFileWriter::addControlPoints(FbxMesh* mesh, const ExporterMeshData& meshData)
{	
	std::vector<uint32_t> vertices;
	std::cout << "Adding control points" << std::endl;
	std::vector<int32_t> mapping(meshData.positionsCount, -1);
	for (uint32_t ch = 0; ch < meshData.meshCount; ++ch)
	{
		mapping.assign(meshData.positionsCount, -1);
		for (uint32_t sb = 0; sb < meshData.submeshCount; ++sb)
		{
			uint32_t* first = meshData.submeshOffsets + ch * meshData.submeshCount + sb;
			for (uint32_t pi = *first; pi < *(first+1); ++pi)
			{
				uint32_t p = meshData.posIndex[pi];
				if (mapping[p] == -1)
				{
					mapping[p] = (int)vertices.size();
					vertices.push_back(p);
					meshData.posIndex[pi] = mapping[p];
				}
				else
				{
					meshData.posIndex[pi] = mapping[p];
				}
			}		
		}
	}
	mesh->InitControlPoints((int)vertices.size());
	FbxVector4* controlPoints = mesh->GetControlPoints();
	for (auto v : vertices)
	{
		auto& p = meshData.positions[v];
		*controlPoints = FbxVector4(p.x, p.y, p.z, 0);
		++controlPoints;
	}
	std::cout << "Adding control points: done" << std::endl;
}

void FbxFileWriter::addBindPose()
{
	// Store the bind pose
	//Just add all the nodes, it doesn't seem to do any harm and it stops Maya complaining about incomplete bind poses
	FbxPose* pose = FbxPose::Create(sdkManager.get(), "BindPose");
	pose->SetIsBindPose(true);

	int nodeCount = mScene->GetNodeCount();
	for (int i = 0; i < nodeCount; i++)
	{
		FbxNode* node = mScene->GetNode(i);
		FbxMatrix bindMat = node->EvaluateGlobalTransform();

		pose->Add(node, bindMat);
	}

	mScene->AddPose(pose);
}

bool FbxFileWriter::saveToFile(const char* assetName, const char* outputPath)
{

	addBindPose();

	FbxIOSettings* ios = FbxIOSettings::Create(sdkManager.get(), IOSROOT);
	// Set some properties on the io settings
		
	sdkManager->SetIOSettings(ios);
	
	sdkManager->GetIOSettings()->SetBoolProp(EXP_ASCIIFBX, bOutputFBXAscii);
	

	FbxExporter* exporter = FbxExporter::Create(sdkManager.get(), "Scene Exporter");
	exporter->SetFileExportVersion(FBX_2012_00_COMPATIBLE);

	int lFormat;

	if (bOutputFBXAscii)
	{
		lFormat = sdkManager->GetIOPluginRegistry()->FindWriterIDByDescription("FBX ascii (*.fbx)");
	}
	else
	{
		lFormat = sdkManager->GetIOPluginRegistry()->FindWriterIDByDescription("FBX binary (*.fbx)");
	}

	auto path = std::string(outputPath) + "\\" + assetName + ".fbx";
	bool exportStatus = exporter->Initialize(path.c_str(), lFormat, sdkManager->GetIOSettings());

	if (!exportStatus)
	{
		std::cerr << "Call to FbxExporter::Initialize failed" << std::endl;
		std::cerr << "Error returned: " << exporter->GetStatus().GetErrorString() << std::endl;
		return false;
	}

	exportStatus = exporter->Export(mScene);

	if (!exportStatus)
	{
		auto fbxStatus = exporter->GetStatus();

		std::cerr << "Call to FbxExporter::Export failed" << std::endl;
		std::cerr << "Error returned: " << fbxStatus.GetErrorString() << std::endl;
		return false;
	}
	return true;
}



bool FbxFileWriter::appendMesh(const ExporterMeshData& meshData, const char* assetName, bool nonSkinned)
{
	createMaterials(meshData);

	if (nonSkinned)
	{
		return appendNonSkinnedMesh(meshData, assetName);
	}

	/**
		Get polygon count
	*/
	uint32_t polygCount = meshData.submeshOffsets[meshData.meshCount * meshData.submeshCount] / 3;

	FbxMesh* mesh = FbxMesh::Create(sdkManager.get(), "meshgeo");

	FbxGeometryElementNormal* geNormal = mesh->CreateElementNormal();
	geNormal->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
	geNormal->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

	FbxGeometryElementUV* geUV = mesh->CreateElementUV("diffuseElement");
	geUV->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
	geUV->SetReferenceMode(FbxGeometryElement::eIndexToDirect);


	FbxNode* meshNode = FbxNode::Create(mScene, "meshnode");
	meshNode->SetNodeAttribute(mesh);
	meshNode->SetShadingMode(FbxNode::eTextureShading);

	FbxNode* lRootNode = mScene->GetRootNode();

	mRenderLayer->AddMember(meshNode);

	for (uint32_t i = 0; i < mMaterials.size(); ++i)
	{
		meshNode->AddMaterial(mMaterials[i]);
	}

	FbxSkin* skin = FbxSkin::Create(sdkManager.get(), "Skin of the thing");
	skin->SetGeometry(mesh);

	mesh->AddDeformer(skin);

	/**
		Create control points, copy data to buffers
	*/
	addControlPoints(mesh, meshData);

	auto normalsElem = mesh->GetElementNormal();
	for (uint32_t i = 0; i < meshData.normalsCount; ++i)
	{
		auto& n = meshData.normals[i];
		normalsElem->GetDirectArray().Add(FbxVector4(n.x, n.y, n.z, 0));
	}
	auto uvsElem = mesh->GetElementUV("diffuseElement");
	for (uint32_t i = 0; i < meshData.uvsCount; ++i)
	{
		auto& uvs = meshData.uvs[i];
		uvsElem->GetDirectArray().Add(FbxVector2(uvs.x, uvs.y));
	}
		
	FbxGeometryElementMaterial* matElement = mesh->CreateElementMaterial();
	matElement->SetMappingMode(FbxGeometryElement::eByPolygon);
	matElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);


	matElement->GetIndexArray().SetCount(polygCount);
	normalsElem->GetIndexArray().SetCount(polygCount * 3);
	uvsElem->GetIndexArray().SetCount(polygCount * 3);


	std::cout << "Create chunks recursive" << std::endl;

	//In order for Maya to correctly convert the axis of a skinned model there must be a common root node between the skeleton and the model
	FbxNode* sceneRootNode = FbxNode::Create(sdkManager.get(), "sceneRoot");
	lRootNode->AddChild(sceneRootNode);
	sceneRootNode->AddChild(meshNode);

	//UE4 cannot hide the root bone, so add a dummy chunk so chunk0 is not the root
	FbxNode* skelRootNode = FbxNode::Create(sdkManager.get(), "root");
	FbxSkeleton* skelAttrib = FbxSkeleton::Create(sdkManager.get(), "SkelRootAttrib");
	skelAttrib->SetSkeletonType(FbxSkeleton::eRoot);
	skelRootNode->SetNodeAttribute(skelAttrib);

	sceneRootNode->AddChild(skelRootNode);

	// Now walk the tree and create a skeleton with geometry at the same time
	// Find a "root" chunk and walk the tree from there.
	uint32_t chunkCount = NvBlastAssetGetChunkCount(meshData.asset, Nv::Blast::logLL);
	auto chunks = NvBlastAssetGetChunks(meshData.asset, Nv::Blast::logLL);
	uint32_t cpIdx = 0;
	for (uint32_t i = 0; i < chunkCount; i++)
	{
		const NvBlastChunk* chunk = &chunks[i];

		if (chunk->parentChunkIndex == UINT32_MAX)
		{
			uint32_t addedCps = createChunkRecursive(cpIdx, i, meshNode, skelRootNode, skin, meshData);
			cpIdx += addedCps;
		}
	}

	if (!mesh->GetElementSmoothing())
	{
		//If no smoothing groups, generate them
		generateSmoothingGroups(mesh, skin);
	}

	removeDuplicateControlPoints(mesh, skin);

	if (meshData.hulls != nullptr)
	{
		return appendCollisionMesh(chunkCount, meshData.hullsOffsets, meshData.hulls, assetName);
	}
	return true;
}

void FbxFileWriter::generateSmoothingGroups(fbxsdk::FbxMesh* mesh, FbxSkin* skin)
{
	if (mesh->GetElementSmoothing(0) || !mesh->IsTriangleMesh())
	{
		//they already exist or we can't make it
		return;
	}

	const FbxGeometryElementNormal* geNormal = mesh->GetElementNormal();
	if (!geNormal || geNormal->GetMappingMode() != FbxGeometryElement::eByPolygonVertex || geNormal->GetReferenceMode() != FbxGeometryElement::eDirect)
	{
		//We just set this up, but just incase
		return;
	}

	int clusterCount = 0;
	std::vector<std::vector<int>> cpsPerCluster;
	if (skin)
	{
		clusterCount = skin->GetClusterCount();
		cpsPerCluster.resize(clusterCount);
		for (int c = 0; c < clusterCount; c++)
		{
			FbxCluster* cluster = skin->GetCluster(c);
			int* clusterCPList = cluster->GetControlPointIndices();
			const int clusterCPListLength = cluster->GetControlPointIndicesCount();

			cpsPerCluster[c].resize(clusterCPListLength);
			memcpy(cpsPerCluster[c].data(), clusterCPList, sizeof(int) * clusterCPListLength);
			std::sort(cpsPerCluster[c].begin(), cpsPerCluster[c].end());
		}
	}

	auto smElement = mesh->CreateElementSmoothing();
	smElement->SetMappingMode(FbxGeometryElement::eByPolygon);
	smElement->SetReferenceMode(FbxGeometryElement::eDirect);

	FbxVector4* cpList = mesh->GetControlPoints();
	const int cpCount = mesh->GetControlPointsCount();

	const int triangleCount = mesh->GetPolygonCount();
	const int cornerCount = triangleCount * 3;

	int* polygonCPList = mesh->GetPolygonVertices();
	const auto& normalByCornerList = geNormal->GetDirectArray();

	std::multimap<int, int> overlappingCorners;
	//sort them by z for faster overlap checking
	std::vector<std::pair<double, int>> cornerIndexesByZ(cornerCount);
	for (int c = 0; c < cornerCount; c++)
	{
		cornerIndexesByZ[c] = std::pair<double, int>(cpList[polygonCPList[c]][2], c);
	}
	std::sort(cornerIndexesByZ.begin(), cornerIndexesByZ.end());

	for (int i = 0; i < cornerCount; i++)
	{
		const int cornerA = cornerIndexesByZ[i].second;
		const int cpiA = polygonCPList[cornerA];
		FbxVector4 cpA = cpList[cpiA];
		cpA[3] = 0;

		int clusterIndexA = -1;
		for (int c = 0; c < clusterCount; c++)
		{
			if (std::binary_search(cpsPerCluster[c].begin(), cpsPerCluster[c].end(), cpiA))
			{
				clusterIndexA = c;
				break;
			}
		}

		for (int j = i + 1; j < cornerCount; j++)
		{
			if (std::abs(cornerIndexesByZ[j].first - cornerIndexesByZ[i].first) > FBXSDK_TOLERANCE)
			{
				break; // if the z's don't match other values don't matter
			}
			const int cornerB = cornerIndexesByZ[j].second;
			const int cpiB = polygonCPList[cornerB];
			FbxVector4 cpB = cpList[cpiB];

			cpB[3] = 0;

			//uses FBXSDK_TOLERANCE
			if (cpA == cpB)
			{
				int clusterIndexB = -1;
				for (int c = 0; c < clusterCount; c++)
				{
					if (std::binary_search(cpsPerCluster[c].begin(), cpsPerCluster[c].end(), cpiB))
					{
						clusterIndexB = c;
						break;
					}
				}

				if (clusterIndexA == clusterIndexB)
				{
					overlappingCorners.emplace(cornerA, cornerB);
					overlappingCorners.emplace(cornerB, cornerA);
				}
			}
		}
	}

	auto& smoothingGroupByTri = smElement->GetDirectArray();
	for (int i = 0; i < triangleCount; i++)
	{
		smoothingGroupByTri.Add(0);
	}
	//first one
	smoothingGroupByTri.SetAt(0, 1);

	for (int i = 1; i < triangleCount; i++)
	{
		int sharedMask = 0, unsharedMask = 0;
		for (int c = 0; c < 3; c++)
		{
			int myCorner = i * 3 + c;
			FbxVector4 myNormal = normalByCornerList.GetAt(myCorner);
			myNormal.Normalize();
			myNormal[3] = 0;

			auto otherCornersRangeBegin = overlappingCorners.lower_bound(myCorner);
			auto otherCornersRangeEnd = overlappingCorners.upper_bound(myCorner);
			for (auto it = otherCornersRangeBegin; it != otherCornersRangeEnd; it++)
			{
				int otherCorner = it->second;
				FbxVector4 otherNormal = normalByCornerList.GetAt(otherCorner);
				otherNormal.Normalize();
				otherNormal[3] = 0;
				if (otherNormal == myNormal)
				{
					sharedMask |= smoothingGroupByTri[otherCorner / 3];
				}
				else
				{
					unsharedMask |= smoothingGroupByTri[otherCorner / 3];
				}
			}
		}

		//Easy case, no overlap
		if ((sharedMask & unsharedMask) == 0 && sharedMask != 0)
		{
			smoothingGroupByTri.SetAt(i, sharedMask);
		}
		else
		{
			for (int sm = 0; sm < 32; sm++)
			{
				int val = 1 << sm;
				if (((val & sharedMask) == sharedMask) && !(val & unsharedMask))
				{
					smoothingGroupByTri.SetAt(i, val);
					break;
				}
			}
		}
	}

}

namespace
{
	//These methods have different names for some reason
	inline double* getControlPointBlendWeights(FbxSkin* skin)
	{
		return skin->GetControlPointBlendWeights();
	}

	inline double* getControlPointBlendWeights(FbxCluster* cluster)
	{
		return cluster->GetControlPointWeights();
	}

	template <typename T>
	void remapCPsAndRemoveDuplicates(const int newCPCount, const std::vector<int>& oldToNewCPMapping, T* skinOrCluster)
	{
		//Need to avoid duplicate entires since UE doesn't seem to normalize this correctly
		std::vector<bool> addedCP(newCPCount, false);
		std::vector<std::pair<int, double>> newCPsAndWeights;
		newCPsAndWeights.reserve(newCPCount);

		int* skinCPList = skinOrCluster->GetControlPointIndices();
		double* skinCPWeights = getControlPointBlendWeights(skinOrCluster);
		const int skinCPListLength = skinOrCluster->GetControlPointIndicesCount();

		for (int bw = 0; bw < skinCPListLength; bw++)
		{
			int newCPIdx = oldToNewCPMapping[skinCPList[bw]];
			if (!addedCP[newCPIdx])
			{
				addedCP[newCPIdx] = true;
				newCPsAndWeights.emplace_back(newCPIdx, skinCPWeights[bw]);
			}
		}
		skinOrCluster->SetControlPointIWCount(newCPsAndWeights.size());
		skinCPList = skinOrCluster->GetControlPointIndices();
		skinCPWeights = getControlPointBlendWeights(skinOrCluster);
		for (size_t bw = 0; bw < newCPsAndWeights.size(); bw++)
		{
			skinCPList[bw] = newCPsAndWeights[bw].first;
			skinCPWeights[bw] = newCPsAndWeights[bw].second;
		}
	}
}

//Do this otherwise Maya shows the mesh as faceted due to not being welded
void FbxFileWriter::removeDuplicateControlPoints(fbxsdk::FbxMesh* mesh, FbxSkin* skin)
{
	FbxVector4* cpList = mesh->GetControlPoints();
	const int cpCount = mesh->GetControlPointsCount();

	std::vector<int> oldToNewCPMapping(cpCount, -1);
	//sort them by z for faster overlap checking
	std::vector<std::pair<double, int>> cpIndexesByZ(cpCount);
	for (int cp = 0; cp < cpCount; cp++)
	{
		cpIndexesByZ[cp] = std::pair<double, int>(cpList[cp][2], cp);
	}
	std::sort(cpIndexesByZ.begin(), cpIndexesByZ.end());

	int clusterCount = 0;
	std::vector<std::vector<int>> cpsPerCluster;
	if (skin)
	{
		clusterCount = skin->GetClusterCount();
		cpsPerCluster.resize(clusterCount);
		for (int c = 0; c < clusterCount; c++)
		{
			FbxCluster* cluster = skin->GetCluster(c);
			int* clusterCPList = cluster->GetControlPointIndices();
			const int clusterCPListLength = cluster->GetControlPointIndicesCount();

			cpsPerCluster[c].resize(clusterCPListLength);
			memcpy(cpsPerCluster[c].data(), clusterCPList, sizeof(int) * clusterCPListLength);
			std::sort(cpsPerCluster[c].begin(), cpsPerCluster[c].end());
		}
	}

	std::vector<FbxVector4> uniqueCPs;
	uniqueCPs.reserve(cpCount);

	for (int i = 0; i < cpCount; i++)
	{
		const int cpiA = cpIndexesByZ[i].second;
		FbxVector4 cpA = cpList[cpiA];
		if (!(oldToNewCPMapping[cpiA] < 0))
		{
			//already culled this one
			continue;
		}
		const int newIdx = int(uniqueCPs.size());
		oldToNewCPMapping[cpiA] = newIdx;
		uniqueCPs.push_back(cpA);

		int clusterIndexA = -1;
		for (int c = 0; c < clusterCount; c++)
		{
			if (std::binary_search(cpsPerCluster[c].begin(), cpsPerCluster[c].end(), cpiA))
			{
				clusterIndexA = c;
				break;
			}
		}
		
		for (int j = i + 1; j < cpCount; j++)
		{
			if (std::abs(cpIndexesByZ[j].first - cpIndexesByZ[i].first) > FBXSDK_TOLERANCE)
			{
				break; // if the z's don't match other values don't matter
			}
			
			const int cpiB = cpIndexesByZ[j].second;
			FbxVector4 cpB = cpList[cpiB];

			//uses FBXSDK_TOLERANCE
			if (cpA == cpB)
			{
				int clusterIndexB = -1;
				for (int c = 0; c < clusterCount; c++)
				{
					if (std::binary_search(cpsPerCluster[c].begin(), cpsPerCluster[c].end(), cpiB))
					{
						clusterIndexB = c;
						break;
					}
				}

				//don't merge unless they share the same clusters
				if (clusterIndexA == clusterIndexB)
				{
					oldToNewCPMapping[cpiB] = newIdx;
				}
			}
		}
	}

	const int originalCPCount = cpCount;
	const int newCPCount = int(uniqueCPs.size());

	if (newCPCount == cpCount)
	{
		//don't bother, it will just scramble it for no reason
		return;
	}

	mesh->InitControlPoints(newCPCount);
	cpList = mesh->GetControlPoints();

	for (int cp = 0; cp < newCPCount; cp++)
	{
		cpList[cp] = uniqueCPs[cp];
	}

	int* polygonCPList = mesh->GetPolygonVertices();
	const int polygonCPListLength = mesh->GetPolygonVertexCount();
	for (int pv = 0; pv < polygonCPListLength; pv++)
	{
		polygonCPList[pv] = oldToNewCPMapping[polygonCPList[pv]];
	}

	if (skin)
	{
		remapCPsAndRemoveDuplicates(newCPCount, oldToNewCPMapping, skin);
		for (int c = 0; c < skin->GetClusterCount(); c++)
		{
			FbxCluster* cluster = skin->GetCluster(c);
			remapCPsAndRemoveDuplicates(newCPCount, oldToNewCPMapping, cluster);
		}
		 
	}
}