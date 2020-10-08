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
// Copyright (c) 2020 NVIDIA Corporation. All rights reserved.

#include "NvBlastExtExporterFbxReader.h"
#include "NvBlastExtExporterFbxUtils.h"
#include "NvBlastGlobals.h"
#include <fbxsdk.h>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_map>

#include "NvBlastExtAuthoringMesh.h"
#include "NvBlastExtAuthoringBondGenerator.h"
#include "NvBlastPxSharedHelpers.h"

using physx::PxVec3;
using physx::PxVec2;

using namespace Nv::Blast;

FbxFileReader::FbxFileReader()
{
	mMeshCount = 0;
	mChunkCount = 0;
}

void FbxFileReader::release()
{
	delete this;
}

FbxAMatrix FbxFileReader::getTransformForNode(FbxNode* node)
{
	//The geometry transform contains the information about the pivots in the mesh node relative to the node's transform
	FbxAMatrix geometryTransform(node->GetGeometricTranslation(FbxNode::eSourcePivot),
								 node->GetGeometricRotation(FbxNode::eSourcePivot),
								 node->GetGeometricScaling(FbxNode::eSourcePivot));
	FbxAMatrix nodeTransform = node->EvaluateGlobalTransform();

	return nodeTransform * geometryTransform;
}

void FbxFileReader::loadFromFile(const char* filename)
{
	// Wrap in a shared ptr so that when it deallocates we get an auto destroy and all of the other assets created don't leak.
	std::shared_ptr<FbxManager> sdkManager = std::shared_ptr<FbxManager>(FbxManager::Create(), [=](FbxManager* manager)
	{
		manager->Destroy();
	});

	mChunkCount = 0;
	mCollisionNodes.clear();
	FbxIOSettings* ios = FbxIOSettings::Create(sdkManager.get(), IOSROOT);
	// Set some properties on the io settings

	sdkManager->SetIOSettings(ios);

	
	FbxImporter* importer = FbxImporter::Create(sdkManager.get(), "");

	bool importStatus = importer->Initialize(filename, -1, sdkManager->GetIOSettings());

	if (!importStatus)
	{
		std::cerr << "Call to FbxImporter::Initialize failed." << std::endl;
		std::cerr << "Error returned: " << importer->GetStatus().GetErrorString() << std::endl;

		return;
	}

	FbxScene* scene = FbxScene::Create(sdkManager.get(), "importScene");




	importStatus = importer->Import(scene);

	if (!importStatus)
	{
		std::cerr << "Call to FbxImporter::Import failed." << std::endl;
		std::cerr << "Error returned: " << importer->GetStatus().GetErrorString() << std::endl;

		return;
	}

	int32_t matCount = scene->GetMaterialCount();

	for (int32_t i = 0; i < matCount; ++i)
	{
		mMaterialNames.push_back(std::string(scene->GetMaterial(i)->GetName()));
	}

	//This removes axis and unit conversion nodes so it converts the entire scene to the header specified axis and units
	FbxRootNodeUtility::RemoveAllFbxRoots(scene);

	FbxAxisSystem blastAxisSystem = FbxUtils::getBlastFBXAxisSystem();
	FbxAxisSystem sourceSetup = scene->GetGlobalSettings().GetAxisSystem();
	if (sourceSetup != blastAxisSystem)
	{
		std::cout << "Converting to Blast coordinates" << std::endl;
		std::cout << "Existing axis: " << FbxUtils::FbxAxisSystemToString(sourceSetup) << std::endl;
		blastAxisSystem.ConvertScene(scene);
	}

	FbxSystemUnit blastUnits = FbxUtils::getBlastFBXUnit();
	FbxSystemUnit sourceUnits = scene->GetGlobalSettings().GetSystemUnit();
	if (sourceUnits != blastUnits)
	{
		std::cout << "Converting to Blast units" << std::endl;
		std::cout << "Existing units: " << FbxUtils::FbxSystemUnitToString(sourceUnits) << std::endl;
		blastUnits.ConvertScene(scene);
	}

	FbxGeometryConverter geoConverter(sdkManager.get());
	FbxDisplayLayer* collisionDisplayLayer = scene->FindMember<FbxDisplayLayer>(FbxUtils::getCollisionGeometryLayerName().c_str());

	// Recurse the fbx tree and find all meshes
	std::vector<FbxNode*> meshNodes;
	getFbxMeshes(collisionDisplayLayer, scene->GetRootNode(), meshNodes);

	if (isCollisionLoaded())
	{
		std::cout << "Collision geometry is found.";
		getCollisionInternal();
	}

	std::cout << "Found " << meshNodes.size() << " meshes." << std::endl;

	if (meshNodes.size() > 1)
	{
		FbxArray<FbxNode*> tempMeshArray;
		tempMeshArray.Resize((int)meshNodes.size());
		for (size_t i = 0; i < meshNodes.size(); i++)
		{
			FbxMesh* mesh = meshNodes[i]->GetMesh();
			if (mesh->GetDeformerCount(FbxDeformer::eSkin) != 0)
			{
				std::cerr << "Multi-part mesh " << meshNodes[i]->GetName() << " is already skinned, not sure what to do" << std::endl;
				return;
			}
			//Add a one-bone skin so later when we merge meshes the connection to the chunk transform will stick, this handles the non-skinned layout of the FBX file
			FbxSkin* skin = FbxSkin::Create(scene, (std::string(meshNodes[i]->GetName()) + "_skin").c_str());
			mesh->AddDeformer(skin);
			FbxCluster* cluster = FbxCluster::Create(skin, (std::string(meshNodes[i]->GetName()) + "_cluster").c_str());
			skin->AddCluster(cluster);
			cluster->SetLink(meshNodes[i]);
			const int cpCount = mesh->GetControlPointsCount();
			cluster->SetControlPointIWCount(cpCount);
			//Fully weight to the one bone
			int* cpIdx = cluster->GetControlPointIndices();
			double* cpWeights = cluster->GetControlPointWeights();
			for (int cp = 0; cp < cpCount; cp++)
			{
				cpIdx[cp] = cp;
				cpWeights[cp] = 1.0;
			}
			tempMeshArray.SetAt(int(i), meshNodes[i]);
		}
		meshNodes.resize(1);
		meshNodes[0] = geoConverter.MergeMeshes(tempMeshArray, "MergedMesh", scene);
	}

	if (meshNodes.empty())
	{
		return;
	}

	FbxNode* meshNode = meshNodes[0];
	FbxMesh* mesh = meshNode->GetMesh();

	// Verify that the mesh is triangulated.
	bool bAllTriangles = mesh->IsTriangleMesh();
	if (!bAllTriangles)
	{
		//try letting the FBX SDK triangulate it
		mesh = FbxCast<FbxMesh>(geoConverter.Triangulate(mesh, true));
		bAllTriangles = mesh->IsTriangleMesh();
	}

	int polyCount = mesh->GetPolygonCount();
	if (!bAllTriangles)
	{
		std::cerr << "Mesh 0 has " << polyCount << " polygons but not all of them are triangles. Mesh must be triangulated." << std::endl;
		return;
	}

	FbxStringList uvSetNames;

	mesh->GetUVSetNames(uvSetNames);

	const char * uvSetName = uvSetNames.GetStringAt(0);

	std::vector<NvcVec3> positions;
	std::vector<NvcVec3> normals;
	std::vector<NvcVec2> uv;
	std::vector<uint32_t> indices;

	int* polyVertices = mesh->GetPolygonVertices();

	uint32_t vertIndex = 0;

	FbxAMatrix trans = getTransformForNode(meshNode);
	FbxAMatrix normalTransf = trans.Inverse().Transpose();

	int32_t matElements = mesh->GetElementMaterialCount();
	if (matElements > 1)
	{
		std::cerr << "Mesh has more than 1 material mappings, first one will be used. " << std::endl;
	}
	auto matLayer = mesh->GetElementMaterial(0);
	auto smLayer = mesh->GetElementSmoothing();

	const int triangleIndexMappingUnflipped[3] = { 0, 1, 2 };
	const int triangleIndexMappingFlipped[3] = { 2, 1, 0 };
	const int* triangleIndexMapping = trans.Determinant() < 0 ? triangleIndexMappingFlipped : triangleIndexMappingUnflipped;

	for (int i = 0; i < polyCount; i++)
	{
		for (int vi = 0; vi < 3; vi++)
		{
			int polyCPIdx = polyVertices[i*3+ triangleIndexMapping[vi]];

			FbxVector4 vert = mesh->GetControlPointAt(polyCPIdx);
			FbxVector4 normVec;
			FbxVector2 uvVec;

			bool bUnmapped;
			mesh->GetPolygonVertexNormal(i, vi, normVec);
			mesh->GetPolygonVertexUV(i, vi, uvSetName, uvVec, bUnmapped);			
			vert = trans.MultT(vert);
			normVec = normalTransf.MultT(normVec);

			positions.push_back({(float) vert[0], (float)vert[1], (float)vert[2]});
			normals.push_back({(float)normVec[0], (float)normVec[1], (float)normVec[2]});
			uv.push_back({(float)uvVec[0], (float)uvVec[1]});
			indices.push_back(vertIndex++);
		}
		if (matLayer != nullptr)
		{
			mMaterialIds.push_back(matLayer->GetIndexArray().GetAt(i));
		}
		if (smLayer != nullptr)
		{
			mSmoothingGroups.push_back(smLayer->GetDirectArray().GetAt(i));
		}
	}
	
	mVertexPositions = positions;
	mVertexNormals = normals;
	mVertexUv = uv;
	mIndices = indices;

	getBoneInfluencesInternal(mesh);
}

int32_t* FbxFileReader::getSmoothingGroups()
{
	if (!mSmoothingGroups.empty())
	{
		return mSmoothingGroups.data();
	}
	else
	{
		return nullptr;
	}
}

int32_t FbxFileReader::getMaterialCount()
{
	return mMaterialNames.size();
}

void FbxFileReader::getFbxMeshes(FbxDisplayLayer* collisionDisplayLayer, FbxNode* node, std::vector<FbxNode*>& meshNodes)
{
	FbxMesh* mesh = node->GetMesh();

	if (mesh != nullptr)
	{
		if (collisionDisplayLayer == nullptr && node->FindProperty("ParentalChunkIndex").IsValid())
		{
			//Old-style file
			uint32_t chunkIndex = node->FindProperty("ParentalChunkIndex").Get<int32_t>();
			mCollisionNodes.emplace(chunkIndex, node);
		}
		else if (collisionDisplayLayer != nullptr && collisionDisplayLayer->IsMember(node))
		{
			uint32_t chunkIndex = FbxUtils::getChunkIndexForNode(node);
			if (chunkIndex != UINT32_MAX)
			{
				mCollisionNodes.emplace(chunkIndex, node);
			}
			else
			{
				std::cerr << "Warning: Not sure what to do about collision geo " << node->GetName() << ". No corresponding chunk." << std::endl;
			}
		}
		else
		{
			meshNodes.push_back(node);
		}
	}
	int childCount = node->GetChildCount();

	for (int i = 0; i < childCount; i++)
	{
		FbxNode * childNode = node->GetChild(i);

		getFbxMeshes(collisionDisplayLayer, childNode, meshNodes);
	}
}

bool FbxFileReader::isCollisionLoaded()
{
	return !mCollisionNodes.empty();
}

uint32_t FbxFileReader::getCollision(uint32_t*& hullsOffset, Nv::Blast::CollisionHull**& hulls)
{
	if (!isCollisionLoaded())
	{
		hullsOffset = nullptr;
		hulls = nullptr;
		return 0;
	}
	hullsOffset = (uint32_t*)NVBLAST_ALLOC(sizeof(uint32_t) * mHullsOffset.size());
	memcpy(hullsOffset, mHullsOffset.data(), sizeof(uint32_t) * mHullsOffset.size());
	
	
	hulls = (Nv::Blast::CollisionHull**)NVBLAST_ALLOC(sizeof(Nv::Blast::CollisionHull*) * mHulls.size());
	for (size_t i = 0; i < mHulls.size(); i++)
	{
		//This deep-copies the data inside
		hulls[i] = new CollisionHullImpl(mHulls[i]);
	}
	return mMeshCount;
}

bool FbxFileReader::getCollisionInternal()
{
	mHulls.clear();
	int32_t maxParentIndex = 0;
	
	for (auto p : mCollisionNodes)
	{
		int32_t parentIndex = p.first;
		maxParentIndex = std::max(maxParentIndex, parentIndex);
	}
	mMeshCount = maxParentIndex + 1;
	mHullsOffset.resize(mMeshCount + 1);
	mHulls.resize(mCollisionNodes.size());

	uint32_t currentHullCount = 0;
	uint32_t prevParentIndex = 0;
	mHullsOffset[0] = currentHullCount;

	for (auto p : mCollisionNodes) // it should be sorted by chunk id
	{		
		uint32_t parentIndex = p.first;
		if (prevParentIndex != parentIndex)
		{
			for (uint32_t m = prevParentIndex + 1; m < parentIndex; m++)
			{
				//copy these if there were no collision meshes
				mHullsOffset[m] = mHullsOffset[prevParentIndex];
			}
			mHullsOffset[parentIndex] = currentHullCount;
			prevParentIndex = parentIndex;
		}
		Nv::Blast::CollisionHull& chull = mHulls[currentHullCount];
		currentHullCount++;

		FbxMesh* meshNode = p.second->GetMesh();

		FbxAMatrix nodeTransform = getTransformForNode(p.second);
		FbxAMatrix nodeTransformNormal = nodeTransform.Inverse().Transpose();

		//PhysX seems to care about having welding verticies.
		//Probably doing a dumb search is fast enough since how big could the convex hulls possibly be?
		std::vector<FbxVector4> uniqueCPValues;
		uniqueCPValues.reserve(meshNode->GetControlPointsCount());
		std::vector<uint32_t> originalToNewCPMapping(meshNode->GetControlPointsCount(), ~0U);

		FbxVector4* vpos = meshNode->GetControlPoints();
		for (int32_t i = 0; i < meshNode->GetControlPointsCount(); ++i)
		{
			FbxVector4 worldVPos = nodeTransform.MultT(*vpos);
			bool found = false;
			for (size_t j = 0; j < uniqueCPValues.size(); j++)
			{
				if (uniqueCPValues[j] == worldVPos)
				{
					originalToNewCPMapping[i] = uint32_t(j);
					found = true;
					break;
				}
			}
			if (!found)
			{
				originalToNewCPMapping[i] = uint32_t(uniqueCPValues.size());
				uniqueCPValues.push_back(worldVPos);
			}
			vpos++;
		}

		chull.points = new NvcVec3[uniqueCPValues.size()];
		chull.pointsCount = uint32_t(uniqueCPValues.size());
	
		physx::PxVec3 hullCentroid(0.0f);

		for (uint32_t i = 0; i < chull.pointsCount; ++i)
		{
			const FbxVector4& worldVPos = uniqueCPValues[i];
			chull.points[i].x = (float)worldVPos[0];
			chull.points[i].y = (float)worldVPos[1];
			chull.points[i].z = (float)worldVPos[2];
			hullCentroid += toPxShared(chull.points[i]);
		}

		if (chull.pointsCount)
		{
			hullCentroid /= (float)chull.pointsCount;
		}

		uint32_t polyCount = meshNode->GetPolygonCount();
		chull.polygonData = new Nv::Blast::HullPolygon[polyCount];
		chull.polygonDataCount = polyCount;

		chull.indicesCount = meshNode->GetPolygonVertexCount();
		chull.indices = new uint32_t[chull.indicesCount];
		uint32_t curIndexCount = 0;

		for (uint32_t poly = 0; poly < polyCount; ++poly)
		{
			int32_t vInPolyCount = meshNode->GetPolygonSize(poly);
			auto& pd = chull.polygonData[poly];
			pd.indexBase = (uint16_t)curIndexCount;
			pd.vertexCount = (uint16_t)vInPolyCount;
			int32_t* ind = &meshNode->GetPolygonVertices()[meshNode->GetPolygonVertexIndex(poly)];
			uint32_t* destInd = chull.indices + curIndexCount;
			for (int32_t v = 0; v < vInPolyCount; v++)
			{
				destInd[v] = originalToNewCPMapping[ind[v]];
			}
			curIndexCount += vInPolyCount;

			//Don't depend on the normals to create the plane normal, they could be wrong
			PxVec3 lastThreeVerts[3] = {
				toPxShared(chull.points[chull.indices[curIndexCount - 1]]),
				toPxShared(chull.points[chull.indices[curIndexCount - 2]]),
				toPxShared(chull.points[chull.indices[curIndexCount - 3]])
			};

			physx::PxPlane plane(lastThreeVerts[0], lastThreeVerts[1], lastThreeVerts[2]);
			plane.normalize();

			const float s = plane.n.dot(lastThreeVerts[0] - hullCentroid) >= 0.0f ? 1.0f : -1.0f;

			pd.plane[0] = s*plane.n.x;
			pd.plane[1] = s*plane.n.y;
			pd.plane[2] = s*plane.n.z;
			pd.plane[3] = s*plane.d;
		}
	}

	//Set the end marker
	for (uint32_t m = prevParentIndex + 1; m <= mMeshCount; m++)
	{
		//copy these if there were no collision meshes
		mHullsOffset[m] = currentHullCount;
	}

	return false;
}


/**
	To work properly export tool should give bone names as bone_@chunkIndex (e.g. bone_1, bone_2)
**/
bool FbxFileReader::getBoneInfluencesInternal(FbxMesh* meshNode) 
{
	std::unordered_map<FbxNode*, uint32_t> boneToChunkIndex;

	if (meshNode->GetDeformerCount() != 1)
	{
		std::cout << "Can't create bone mapping: There is no mesh deformers...: " << std::endl;
		return false;
	}
	mVertexToContainingChunkMap.clear();
	mVertexToContainingChunkMap.resize(mVertexPositions.size());
	std::vector<uint32_t> controlToParentChunkMap;
	controlToParentChunkMap.resize(meshNode->GetControlPointsCount());
	FbxSkin* def = (FbxSkin *)meshNode->GetDeformer(0, FbxDeformer::EDeformerType::eSkin);

	if (def->GetClusterCount() == 0)
	{
		std::cout << "Can't create bone mapping: There is no vertex clusters...: " << std::endl;
		return false;
	}
	//We want the number of chunks not the bones in the FBX file
	mChunkCount = 0;

	for (int32_t i = 0; i < def->GetClusterCount(); ++i)
	{
		FbxCluster* cls = def->GetCluster(i);
		FbxNode* bone = cls->GetLink();

		uint32_t myChunkIndex;
		auto findIt = boneToChunkIndex.find(bone);
		if (findIt != boneToChunkIndex.end())
		{
			myChunkIndex = findIt->second;
		}
		else
		{
			myChunkIndex = FbxUtils::getChunkIndexForNode(bone);
			if (myChunkIndex == UINT32_MAX)
			{
				//maybe an old file?
				myChunkIndex = FbxUtils::getChunkIndexForNodeBackwardsCompatible(bone);
			}

			if (myChunkIndex == UINT32_MAX)
			{
				std::cerr << "Not sure what to do with node " << bone->GetName() << ". is this a chunk?" << std::endl;
			}

			boneToChunkIndex.emplace(bone, myChunkIndex);
			if (myChunkIndex >= mChunkCount)
			{
				mChunkCount = myChunkIndex + 1;
			}
		}

		int32_t* cpIndx = cls->GetControlPointIndices();
		for (int32_t j = 0; j < cls->GetControlPointIndicesCount(); ++j)
		{
			controlToParentChunkMap[*cpIndx] = myChunkIndex;
			++cpIndx;
		}
	}

	int* polyVertices = meshNode->GetPolygonVertices();
	uint32_t lv = 0;
	for (int i = 0; i < meshNode->GetPolygonCount(); i++)
	{
		for (int vi = 0; vi < 3; vi++)
		{
			mVertexToContainingChunkMap[lv] = controlToParentChunkMap[*polyVertices];
			polyVertices++;
			lv++;
		}
	}
	return true;
};

NvcVec3* FbxFileReader::getPositionArray()
{
	return mVertexPositions.data();
};

NvcVec3* FbxFileReader::getNormalsArray()
{
	return mVertexNormals.data();
};

NvcVec2* FbxFileReader::getUvArray()
{
	return mVertexUv.data();
};

uint32_t* FbxFileReader::getIndexArray()
{
	return mIndices.data();
};

uint32_t FbxFileReader::getBoneInfluences(uint32_t*& out)
{
	out = static_cast<uint32_t*>(NVBLAST_ALLOC(sizeof(uint32_t) * mVertexToContainingChunkMap.size()));
	memcpy(out, mVertexToContainingChunkMap.data(), sizeof(uint32_t) * mVertexToContainingChunkMap.size());
	return mVertexToContainingChunkMap.size();
}

uint32_t FbxFileReader::getBoneCount()
{
	return mChunkCount;
}

const char* FbxFileReader::getMaterialName(int32_t id)
{
	if (id < int32_t(mMaterialNames.size()) && id >= 0)
	{
		return mMaterialNames[id].c_str();		
	}
	else
	{
		return nullptr;
	}
}

int32_t* FbxFileReader::getMaterialIds()
{
	if (mMaterialIds.empty())
	{
		return nullptr;
	}
	return mMaterialIds.data();
}
