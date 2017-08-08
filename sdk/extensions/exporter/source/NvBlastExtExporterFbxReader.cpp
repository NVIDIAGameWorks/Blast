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

#include "NvBlastExtExporterFbxReader.h"
#include "NvBlastExtExporterFbxUtils.h"
#include "NvBlastGlobals.h"
#include "fileio/fbxiosettings.h"
#include "fileio/fbxiosettingspath.h"
#include "core/base/fbxstringlist.h"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <sstream>
#include "scene/geometry/fbxmesh.h"

#include "PxVec3.h"
#include "PxVec2.h"
#include "NvBlastExtAuthoringMesh.h"
#include "NvBlastExtAuthoringBondGenerator.h"
#include "NvBlastExtAuthoringCollisionBuilder.h"

using physx::PxVec3;
using physx::PxVec2;

using namespace Nv::Blast;

FbxFileReader::FbxFileReader()
{
	mBoneCount = 0;
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
		std::cout << "Deleting FbxManager" << std::endl;
		manager->Destroy();
	});

	mBoneCount = 0;
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

	// Process just 0, because dumb. Fail out if more than 1?
	if (meshNodes.size() > 1)
	{
		std::cerr << "Can't load more that one graphics mesh." << std::endl;
		return;
	}

	if (meshNodes.empty())
	{
		return;
	}

	FbxNode* meshNode = meshNodes[0];
	FbxMesh* mesh = meshNode->GetMesh();

	int polyCount = mesh->GetPolygonCount();


	bool bAllTriangles = true;
	// Verify that the mesh is triangulated.
	for (int i = 0; i < polyCount; i++)
	{
		if (mesh->GetPolygonSize(i) != 3)
		{
			bAllTriangles = false;
		}
	}

	if (!bAllTriangles)
	{
		std::cerr << "Mesh 0 has " << polyCount << " but not all polygons are triangles. Mesh must be triangulated." << std::endl;
		return;
	}

	FbxStringList uvSetNames;

	mesh->GetUVSetNames(uvSetNames);

	const char * uvSetName = uvSetNames.GetStringAt(0);

	std::vector<PxVec3> positions;
	std::vector<PxVec3> normals;
	std::vector<PxVec2> uv;
	std::vector<uint32_t> indices;

	int* polyVertices = mesh->GetPolygonVertices();

	uint32_t vertIndex = 0;

	FbxAMatrix trans = getTransformForNode(meshNode);
	FbxVector4 rotation = trans.GetR();
	FbxVector4 scale = trans.GetS();
	FbxAMatrix normalTransf;
	normalTransf.SetR(rotation);
	normalTransf.SetS(scale);
	normalTransf = normalTransf.Inverse().Transpose();

	int32_t matElements = mesh->GetElementMaterialCount();
	if (matElements > 1)
	{
		std::cerr << "Mesh has more than 1 material mappings, first one will be used. " << std::endl;
	}
	auto matLayer = mesh->GetElementMaterial(0);
	auto smLayer = mesh->GetElementSmoothing();


	for (int i = 0; i < polyCount; i++)
	{
		for (int vi = 0; vi < 3; vi++)
		{
			int polyCPIdx = polyVertices[i*3+vi];

			FbxVector4 vert = mesh->GetControlPointAt(polyCPIdx);
			FbxVector4 normVec;
			FbxVector2 uvVec;

			bool bUnmapped;
			mesh->GetPolygonVertexNormal(i, vi, normVec);
			mesh->GetPolygonVertexUV(i, vi, uvSetName, uvVec, bUnmapped);			
			vert = trans.MultT(vert);
			normVec = normalTransf.MultT(normVec);

			positions.push_back(PxVec3((float) vert[0], (float)vert[1], (float)vert[2]));
			normals.push_back(PxVec3((float)normVec[0], (float)normVec[1], (float)normVec[2]));
			uv.push_back(PxVec2((float)uvVec[0], (float)uvVec[1]));
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

uint32_t FbxFileReader::getCollision(uint32_t*& hullsOffset, Nv::Blast::CollisionHull** hulls)
{
	if (!isCollisionLoaded())
	{
		return 0;
	}
	hullsOffset = new uint32_t[mMeshCount + 1];
	hulls = new Nv::Blast::CollisionHull*[mMeshCount];
	memcpy(hullsOffset, mHullsOffset.data(), sizeof(uint32_t) * (mMeshCount + 1));
	memcpy(hulls, mHulls.data(), sizeof(Nv::Blast::CollisionHull*) * mMeshCount);
	return mMeshCount;
}

struct CollisionHullImpl : public Nv::Blast::CollisionHull
{
	void release() override
	{

	}
};

bool FbxFileReader::getCollisionInternal()
{
	for (auto hull : mHulls)
	{
		hull->release();
	}
	int32_t maxParentIndex = 0;
	
	for (auto p : mCollisionNodes)
	{
		int32_t parentIndex = p.first;
		maxParentIndex = std::max(maxParentIndex, parentIndex);
	}
	mMeshCount = maxParentIndex + 1;
	mHullsOffset.resize(mMeshCount + 1);
	mHulls.resize(mCollisionNodes.size());
	mHullsOffset[0] = 0;

	for (auto p : mCollisionNodes) // it should be sorted by chunk id
	{		
		int32_t parentIndex = p.first;
		//hulls[parentIndex].push_back(Nv::Blast::CollisionHull());
		mHulls[mHullsOffset[parentIndex]] = new CollisionHullImpl();
		Nv::Blast::CollisionHull& chull = *mHulls[mHullsOffset[parentIndex]];
		FbxMesh* meshNode = p.second->GetMesh();

		FbxAMatrix nodeTransform = getTransformForNode(p.second);
		FbxAMatrix nodeTransformNormal = nodeTransform.Inverse().Transpose();

		chull.points = new PxVec3[meshNode->GetControlPointsCount()];
		FbxVector4* vpos = meshNode->GetControlPoints();
		/**
			Copy control points from FBX.
		*/
		for (int32_t i = 0; i < meshNode->GetControlPointsCount(); ++i)
		{
			FbxVector4 worldVPos = nodeTransform.MultT(*vpos);
			chull.points[i].x = (float)worldVPos[0];
			chull.points[i].y = (float)worldVPos[1];
			chull.points[i].z = (float)worldVPos[2];
			vpos++;
		}

		uint32_t polyCount = meshNode->GetPolygonCount();
		chull.polygonData = new Nv::Blast::CollisionHull::HullPolygon[polyCount];
		FbxGeometryElementNormal* nrm = meshNode->GetElementNormal();
		FbxLayerElementArray& narr = nrm->GetDirectArray();

		for (uint32_t poly = 0; poly < polyCount; ++poly)
		{
			int32_t vInPolyCount = meshNode->GetPolygonSize(poly);
			auto& pd = chull.polygonData[poly];
			pd.mIndexBase = (uint16_t)chull.indicesCount;
			pd.mNbVerts = (uint16_t)vInPolyCount;
			int32_t* ind = &meshNode->GetPolygonVertices()[meshNode->GetPolygonVertexIndex(poly)];
			chull.indices = new uint32_t[vInPolyCount];
			memcpy(chull.indices, ind, sizeof(uint32_t) * vInPolyCount);

			FbxVector4 normal;
			narr.GetAt(poly, &normal);

			normal = nodeTransformNormal.MultT(normal);

			pd.mPlane[0] = (float)normal[0];
			pd.mPlane[1] = (float)normal[1];
			pd.mPlane[2] = (float)normal[2];
			PxVec3 polyLastVertex = chull.points[chull.indices[vInPolyCount - 1]];
			pd.mPlane[3] = -((float)(polyLastVertex.x * normal[0] + polyLastVertex.y * normal[1] + polyLastVertex.z * normal[2]));
		}
		mHullsOffset[parentIndex + 1] = mHullsOffset[parentIndex] + 1;
	}
	

	return false;
}


/**
	To work properly export tool should give bone names as bone_@chunkIndex (e.g. bone_1, bone_2)
**/
bool FbxFileReader::getBoneInfluencesInternal(FbxMesh* meshNode) 
{

	if (meshNode->GetDeformerCount() != 1)
	{
		std::cout << "Can't create bone mapping: There is no mesh deformers...: " << std::endl;
		return false;
	}
	mVertexToParentBoneMap.clear();
	mVertexToParentBoneMap.resize(mVertexPositions.size());
	std::vector<uint32_t> controlToParentBoneMap;
	controlToParentBoneMap.resize(meshNode->GetControlPointsCount());
	FbxSkin* def = (FbxSkin *)meshNode->GetDeformer(0, FbxDeformer::EDeformerType::eSkin);

	if (def->GetClusterCount() == 0)
	{
		std::cout << "Can't create bone mapping: There is no vertex clusters...: " << std::endl;
		return false;
	}	
	mBoneCount = def->GetClusterCount();
	for (int32_t i = 0; i < def->GetClusterCount(); ++i)
	{
		FbxCluster* cls = def->GetCluster(i);
		FbxNode* bone = cls->GetLink();
		int32_t parentChunk = atoi(bone->GetName() + 5);
		int32_t* cpIndx = cls->GetControlPointIndices();
		for (int32_t j = 0; j < cls->GetControlPointIndicesCount(); ++j)
		{
			controlToParentBoneMap[*cpIndx] = parentChunk;
			++cpIndx;
		}
	}
	int* polyVertices = meshNode->GetPolygonVertices();
	uint32_t lv = 0;
	for (int i = 0; i < meshNode->GetPolygonCount(); i++)
	{
		for (int vi = 0; vi < 3; vi++)
		{
			mVertexToParentBoneMap[lv] = controlToParentBoneMap[*polyVertices];
			polyVertices++;
			lv++;
		}
	}
	return true;
};

physx::PxVec3* FbxFileReader::getPositionArray()
{
	return mVertexPositions.data();
};

physx::PxVec3* FbxFileReader::getNormalsArray()
{
	return mVertexNormals.data();
};

physx::PxVec2* FbxFileReader::getUvArray()
{
	return mVertexUv.data();
};

uint32_t* FbxFileReader::getIndexArray()
{
	return mIndices.data();
};

uint32_t FbxFileReader::getBoneInfluences(uint32_t*& out)
{
	out = static_cast<uint32_t*>(NVBLAST_ALLOC(sizeof(uint32_t) * mVertexToParentBoneMap.size()));
	memcpy(out, mVertexToParentBoneMap.data(), sizeof(uint32_t) * mVertexToParentBoneMap.size());
	return mVertexToParentBoneMap.size();
}

uint32_t FbxFileReader::getBoneCount()
{
	return mBoneCount;
}

char* FbxFileReader::getMaterialName(int32_t id)
{
	if (id < int32_t(mMaterialNames.size()) && id >= 0)
	{
		return &mMaterialNames[id][0];		
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