// This code contains NVIDIA Confidential Information and is disclosed 
// under the Mutual Non-Disclosure Agreement. 
// 
// Notice 
// ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES 
// NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO 
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT, 
// MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 
// 
// NVIDIA Corporation assumes no responsibility for the consequences of use of such 
// information or for any infringement of patents or other rights of third parties that may 
// result from its use. No license is granted by implication or otherwise under any patent 
// or patent rights of NVIDIA Corporation. No third party distribution is allowed unless 
// expressly authorized by NVIDIA.  Details are subject to change without notice. 
// This code supersedes and replaces all information previously supplied. 
// NVIDIA Corporation products are not authorized for use as critical 
// components in life support devices or systems without express written approval of 
// NVIDIA Corporation. 
// 
// Copyright (c) 2013 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This file contains wrapper functions to make hair lib easy to setup and use
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "fbxsdk.h"
#include <vector>
#include <string>

#include "FbxUtil.h"
#include "MathUtil.h"

//#include <Nv/Blast/NvHairSdk.h>

// local functions used only in this file
namespace {

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Get the geometry offset to a node. It is never inherited by the children.
	FbxAMatrix GetGeometry(FbxNode* pNode)
	{
		const FbxVector4 lT = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
		const FbxVector4 lR = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
		const FbxVector4 lS = pNode->GetGeometricScaling(FbxNode::eSourcePivot);

		return FbxAMatrix(lT, lR, lS);
	}

	////////////////////////////////////////////////////////////////////////////////////////
	void convertFromFbxMatrix(atcore_float4x4& transform, FbxAMatrix& tmatrix)
	{
		float* data = (float*)&transform;

		// update skinning matrix
		for (int row = 0; row < 4; row++)
			for (int col = 0; col < 4; col++)
			{
				data[ row * 4 + col] = float(tmatrix.Get(row,col));
			}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool checkMesh(FbxNode* pNode)
	{
		FbxMesh* pMesh = pNode->GetMesh();
		if (!pMesh)
			return false;

		return true;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool checkSkinned(FbxNode* pNode)
	{
		FbxGeometry* pFbxGeometry = pNode->GetGeometry();
		if (!pFbxGeometry)
			return false;
	
		FbxSkin* pFbxSkin = (FbxSkin*)pFbxGeometry->GetDeformer(0, FbxDeformer::eSkin);
		if (!pFbxSkin)
			return false;

		return true;
	}
}

namespace
{
	FbxManager*			s_FbxManager = nullptr;
	FbxScene*			s_FbxScene = nullptr;
	

	/////////////////////////////////////////////////////////////////////////////
	// find node by name
	FbxNode* 
	FindNodeByName(FbxScene* scene, const char* nodeName)
	{
		if (!scene)
			return 0;

		if (!nodeName)
			return 0;

		for (int i = 0; i < scene->GetNodeCount(); i++)
		{
			FbxNode* node = scene->GetNode(i);
			const char* name = node->GetName();
			if (!strcmp(nodeName, name))
				return node;
		}
		return 0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool
FbxUtil::Release(void)
{
	if (s_FbxScene)
	{
		s_FbxScene->Destroy();
		s_FbxScene = nullptr;
	}

	if (s_FbxManager)
	{
		s_FbxManager->Destroy();
		s_FbxManager = nullptr;
	}
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FbxUtil::Initialize(const char* fbxFileName, float sceneUnit)
{
	if (fbxFileName)
	{
		FbxImporter*		s_FbxImporter = nullptr;

		if (!s_FbxManager)
			s_FbxManager = FbxManager::Create();

		if (!s_FbxImporter)
			s_FbxImporter = FbxImporter::Create(s_FbxManager, "");

		if (!s_FbxScene)
			s_FbxScene = FbxScene::Create(s_FbxManager, "");
	
		if (!s_FbxImporter->Initialize(fbxFileName, -1))
			return false;

		if (!s_FbxImporter->Import(s_FbxScene))
			return false;

		FbxGlobalSettings& settings = s_FbxScene->GetGlobalSettings();

		FbxSystemUnit fbxUnit = settings.GetSystemUnit();

		if ((sceneUnit > 0.0f) && (sceneUnit != fbxUnit.GetScaleFactor()))
		 {
	 		 FbxSystemUnit convertUnit(sceneUnit);
			 convertUnit.ConvertScene(s_FbxScene);
		 }

		s_FbxImporter->Destroy();

		// get time info
		FbxTimeSpan timespan;
		settings.GetTimelineDefaultTimeSpan(timespan);

		FbxTime startTime = timespan.GetStart();
		FbxTime endTime = timespan.GetStop();

	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FbxUtil::GetGlobalSettings(float* pStartFrame, float* pEndFrame, float *pFps, int *upAxis, char* rootBoneName)
{
	return false;

	/*
	if ( !s_FbxScene)
		return false;

	FbxGlobalSettings& settings = s_FbxScene->GetGlobalSettings();

	FbxTimeSpan timespan;
	settings.GetTimelineDefaultTimeSpan(timespan);

	FbxTime startTime = timespan.GetStart();
	FbxTime endTime = timespan.GetStop();
	
	if (pStartFrame) 
		*pStartFrame = (float)startTime.GetFrameCount();

	if (pEndFrame) 
		*pEndFrame = (float)endTime.GetFrameCount();

	if (pFps)
		*pFps = (float)endTime.GetFrameRate(FbxTime::GetGlobalTimeMode());

	if (upAxis)
	{
		int upSign = 0;
		FbxAxisSystem axisSystem = settings.GetAxisSystem();
		FbxAxisSystem::EUpVector upVector = axisSystem.GetUpVector(upSign);

		enum FbxAxisSystem::ECoordSystem coordSystem = axisSystem.GetCoorSystem();

		switch (upVector)
		{
		case FbxAxisSystem::eXAxis: *upAxis = 0; break;
		case FbxAxisSystem::eYAxis: *upAxis = 1; break;
		case FbxAxisSystem::eZAxis: *upAxis = 2; break;
		}
	}

	if (rootBoneName)
	{
		FbxNode* pRoot = s_FbxScene->GetRootNode();
		if (pRoot)
		{
			strncpy(rootBoneName, pRoot->GetName(), NV_HAIR_MAX_STRING);
		}
		else
			strcpy(rootBoneName, "");
	}

	return true;
	*/
}

/////////////////////////////////////////////////////////////////////////////

bool
FbxUtil::InitializeAnimationCache(AnimationCache& animCache)
{
	return false;

	/*
	if (!s_FbxScene)
		return false;

	float startFrame, endFrame, fps;
	FbxUtil::GetGlobalSettings(&startFrame, &endFrame, &fps);

	int numNodes = s_FbxScene->GetNodeCount();

	////////////////////////////////////////////
	animCache.Initialize(numNodes, (NvInt32)startFrame, (NvInt32)endFrame);

	for (int i = 0; i < numNodes; i++)
	{
		FbxNode* node = s_FbxScene->GetNode(i);
		const char* nodeName = node->GetName();
		animCache.SetBoneName(i, nodeName);
	}

	////////////////////////////////////////////
	for (NvInt frame = animCache.m_frameStart; frame <= animCache.m_frameEnd; frame++)
	{
		FbxTime lTime;
		lTime.SetSecondDouble( frame / fps);

		int index = frame - animCache.m_frameStart;
		atcore_float4x4 *pNodeMatrices = animCache.GetNodeMatrices(index);

		for (int i = 0; i < numNodes; i++)
		{
			FbxNode* node = s_FbxScene->GetNode(i);
			FbxAMatrix nodeMatrix = node->EvaluateGlobalTransform(lTime);
			convertFromFbxMatrix(pNodeMatrices[i], nodeMatrix);
		}
	}

	return true;
	*/
}

////////////////////////////////////////////////////////////////////////////////////////
static FbxSkin* GetFbxSkin( FbxNode* pFbxNode )
{
	if (!pFbxNode)
		return 0;
	
	FbxMesh* pFbxMesh = pFbxNode->GetMesh();
	if (!pFbxMesh)
		return 0;

	FbxGeometry* pFbxGeometry = pFbxNode->GetGeometry();
	if (!pFbxGeometry)
		return 0;

	int deformerCount = pFbxGeometry->GetDeformerCount(FbxDeformer::eSkin);

	FbxSkin *pFbxSkin = 0;
	if (deformerCount > 0)
		pFbxSkin = (FbxSkin*)pFbxGeometry->GetDeformer(0, FbxDeformer::eSkin);

	return pFbxSkin;
}

////////////////////////////////////////////////////////////////////////////////////////
static int GetNumMeshPoints( FbxNode* pFbxNode)
{
	if (!pFbxNode)
		return 0;

	FbxMesh* pFbxMesh = pFbxNode->GetMesh();
	if (!pFbxMesh)
		return 0;

	return pFbxMesh->GetControlPointsCount();
}

////////////////////////////////////////////////////////////////////////////////////////
static bool GetBoneData(FbxSkin *pFbxSkin, FbxNode* pFbxNode, NvChar* pBoneNames, atcore_float4x4 *pBindPoses)
{
	return false;

	/*
	if (!pFbxSkin || !pFbxNode)
		return false;

	int numBones = (int)pFbxSkin->GetClusterCount();

	for (int i = 0; i < numBones; i++)
	{
		FbxCluster* pFbxCluster = pFbxSkin->GetCluster(i);
			
		FbxNode* pLink = pFbxCluster->GetLink();

		// copy bone names
		const char* boneName = pLink->GetName();
		char* str = pBoneNames + i * NV_HAIR_MAX_STRING;
		strcpy_s(str, NV_HAIR_MAX_STRING, boneName);

		// write pose matrix
		{
			FbxAMatrix lReferenceGlobalInitPosition;
			pFbxCluster->GetTransformMatrix(lReferenceGlobalInitPosition);

			FbxAMatrix lReferenceGeometry = GetGeometry(pFbxNode);
			lReferenceGlobalInitPosition *= lReferenceGeometry;

			FbxAMatrix lClusterGlobalInitPosition;
			pFbxCluster->GetTransformLinkMatrix(lClusterGlobalInitPosition);

			FbxAMatrix lClusterMatrix = lClusterGlobalInitPosition.Inverse() * lReferenceGlobalInitPosition;
			convertFromFbxMatrix(pBindPoses[i], lClusterMatrix.Inverse());
		}
	}
	return true;
	*/
}

////////////////////////////////////////////////////////////////////////////////////////
static bool GetSkinningWeights(FbxSkin* pFbxSkin, atcore_float4* boneIndices, atcore_float4* boneWeights, int numPoints)
{
	int numClusters = (int)pFbxSkin->GetClusterCount();
	if (numClusters == 0)
		return false;

	// copy skinning weights and indices
	int* pIndexCounts = new int[numPoints];
	memset(pIndexCounts, 0, sizeof(int) * numPoints);

	for (int i = 0; i < numClusters; i++)
	{
		if (!pFbxSkin) continue;

		FbxCluster* pFbxCluster = pFbxSkin->GetCluster(i);

		if (!pFbxCluster) continue;

		int		cpCount = pFbxCluster->GetControlPointIndicesCount();
		int*		pPointIndices = pFbxCluster->GetControlPointIndices();
		double*		pWeights = pFbxCluster->GetControlPointWeights();

		for (int j = 0; j < cpCount; j++)
		{
			if (pWeights[j] == 0.0f)
				continue;

			int pointIndex = pPointIndices[j];
			int& cnt = pIndexCounts[pointIndex];

			float* pBoneIndex = (float*)&boneIndices[pointIndex];
			float* pBoneWeights = (float*)&boneWeights[pointIndex];

			if (cnt < 4)
			{
				pBoneIndex[cnt] = float(i);
				pBoneWeights[cnt] = float(pWeights[j]);
				cnt++;
			}
			else // over 4, so we just bind to largest weights
			{
				float minWeight = (float)pWeights[j];
				int minIndex = -1;
				for (int b = 0; b < 4; b++) 
				{
					float w = pBoneWeights[b];

					if (w < minWeight)
					{
						minWeight = w;
						minIndex = b;
					}
				}

				if (minIndex >= 0)
				{
					pBoneIndex[minIndex] = float(i);
					pBoneWeights[minIndex] = float(pWeights[j]);
				}

				cnt++;
			}
		}
	}

	for (int i = 0; i < numPoints; i++)
	{
		float* pBoneWeights = (float*)&boneWeights[i];

		float weightsum = 0;
		for (int i = 0; i < 4; i++)
		{
			weightsum += pBoneWeights[i];
		}

		if (weightsum == 0)
			continue;

		for (int i = 0; i < 4; i++)
			pBoneWeights[i] /= weightsum;
	}

	if (pIndexCounts) delete []pIndexCounts;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////
bool FbxUtil::InitializeSkinData( const char* meshName, SkinData& skinData)
{
	return false;

	/*
	FbxNode* pFbxNode = FindNodeByName(s_FbxScene, meshName);
	if (!pFbxNode)
		return false;
	
	FbxSkin* pFbxSkin = GetFbxSkin(pFbxNode);
	int numPoints = GetNumMeshPoints(pFbxNode);

	BoneData& boneData = skinData.m_boneData;

	if (pFbxSkin)
	{
		int numBones = (int)pFbxSkin->GetClusterCount();
		skinData.Allocate(numBones, numPoints);

		// get bone data (bind pose, bone names, ..)
		GetBoneData(pFbxSkin, pFbxNode, boneData.m_pBoneNames, boneData.m_pPoseMatrices);

		// get skinning weights 
		atcore_float4* boneIndices = skinData.m_pBoneIndices;
		atcore_float4* boneWeights = skinData.m_pBoneWeights;

		GetSkinningWeights(pFbxSkin, boneIndices, boneWeights, numPoints);
	}
	else // no skinning, use model matrix
	{
		int numBones = 1;

		skinData.Allocate(numBones, numPoints);

		// copy bone name
		const char* boneName = pFbxNode->GetName();
		boneData.setBoneName(0, boneName);

		// make pose matrix
		gfsdk_makeIdentity(boneData.m_pPoseMatrices[0]);

		// all the vertices map to the model matrix
		atcore_float4* boneIndices = skinData.m_pBoneIndices;
		atcore_float4* boneWeights = skinData.m_pBoneWeights;

		for (int i = 0; i < numPoints; i++)
		{
			boneIndices[i] = gfsdk_makeFloat4(0, 0, 0, 0);
			boneWeights[i] = gfsdk_makeFloat4(1, 0, 0, 0);
		}
	}

	return true;
	*/
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FbxUtil::GetMeshInfo(int* numMeshes, char** meshNames, char** skinned)
{
	int cnt = 0;

	for (int i = 0; i < s_FbxScene->GetNodeCount(); i++)
	{
		FbxNode* pNode = s_FbxScene->GetNode(i);
		if (checkMesh(pNode))
			cnt++;
	}

	char *pNameBuffer = new char[cnt * 128];
	char *pSkinnedBuffer = new char[cnt];

	cnt = 0;
	for (int i = 0; i < s_FbxScene->GetNodeCount(); i++)
	{
		FbxNode* pNode = s_FbxScene->GetNode(i);

		if (!checkMesh(pNode))
			continue;

		strcpy(pNameBuffer + cnt * 128, pNode->GetName());

		pSkinnedBuffer[cnt] = checkSkinned(pNode);

		cnt++;
	}

	*numMeshes = cnt;
	if (skinned)
	{
		*skinned = pSkinnedBuffer;
	}
	else
		delete []pSkinnedBuffer;

	*meshNames = pNameBuffer;

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FbxUtil::GetMeshMaterials( const NvChar* meshName, int *numMaterials, MeshMaterial** materials)
{
	if (!s_FbxScene)
		return false;

	FbxNode* pNode = FindNodeByName(s_FbxScene, meshName);
	if (!pNode)
		return false;

	int numMats = pNode->GetMaterialCount();

	*materials = new MeshMaterial[numMats];

	int matCnt = 0;

	for (int i = 0; i < numMats; i++)
	{
		FbxSurfaceMaterial *mat = pNode->GetMaterial(i);
		if (!mat) continue;
		if (mat->GetUserDataPtr()) continue;

		MeshMaterial& material = (*materials)[matCnt];
			
		strcpy(material.m_name, mat->GetName());
		strcpy(material.m_diffuseTexture, "");
		strcpy(material.m_bumpTexture, "");
		strcpy(material.m_transparentTexture, "");
		strcpy(material.m_normalTexture, "");

		(atcore_float3&)material.m_ambientColor = gfsdk_makeFloat3(1.0f, 1.0f, 1.0f);
		(atcore_float3&)material.m_diffuseColor = gfsdk_makeFloat3(1.0f, 1.0f, 1.0f);
	
		FbxProperty prop;
		FbxProperty fact;
		FbxDouble3 d3;

		// get ambient
		{
			fact = mat->FindProperty(FbxSurfaceMaterial::sAmbientFactor);
			material.m_ambientFactor = fact.IsValid() ? (float)fact.Get<FbxDouble>() : 0.1f;

			prop = mat->FindProperty(FbxSurfaceMaterial::sAmbient);
			if (prop.IsValid())
			{
				FbxDouble3 d3 = prop.Get<FbxDouble3>();
				material.m_ambientColor = gfsdk_makeFloat3((float)d3[0], (float)d3[1], (float)d3[2]);
			}
		}

		// get specular
		{
			fact = mat->FindProperty(FbxSurfaceMaterial::sSpecularFactor);
			material.m_specularFactor = fact.IsValid() ? (float)fact.Get<FbxDouble>() : 0.1f;

			fact = mat->FindProperty(FbxSurfaceMaterial::sShininess);
			material.m_shininess = fact.IsValid() ? (float)fact.Get<FbxDouble>() : 20.0f;
		}

		// get diffuse
		{
			fact = mat->FindProperty(FbxSurfaceMaterial::sDiffuseFactor);
			material.m_diffuseFactor = fact.IsValid() ? (float)fact.Get<FbxDouble>() : 0.5f;

			prop = mat->FindProperty(FbxSurfaceMaterial::sDiffuse);
			if (prop.IsValid())
			{
				FbxDouble3 d3 = prop.Get<FbxDouble3>();
				material.m_diffuseColor = gfsdk_makeFloat3((float)d3[0], (float)d3[1], (float)d3[2]);

				const FbxFileTexture* tex = prop.GetSrcObject<FbxFileTexture>();
				if (tex) 
					strcpy(material.m_diffuseTexture, tex->GetFileName());
			}
		}

		// get normal map
		{
			prop = mat->FindProperty(FbxSurfaceMaterial::sNormalMap);
			if (prop.IsValid())
			{
				const FbxFileTexture* tex = prop.GetSrcObject<FbxFileTexture>();
				if (tex) 
					strcpy(material.m_normalTexture, tex->GetFileName());
			}
		}

		matCnt++;
	}

	*numMaterials = matCnt;

	return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FbxUtil::CreateMeshDescriptor(const char* meshName, MeshDesc &desc)
{
	if (!s_FbxScene)
		return false;

	FbxNode* pNode = FindNodeByName(s_FbxScene, meshName);
	if (!pNode)
		return false;

	// read growth mesh data
	FbxMesh* pMesh = pNode->GetMesh();
	if (!pMesh)
		return false;

	int cpCount = pMesh->GetControlPointsCount();

	int triCount = 0;
	for (int i = 0; i < pMesh->GetPolygonCount(); i++)
	{
		int psize = pMesh->GetPolygonSize(i);
		triCount += (psize - 2);
	}

	// allocate buffers
	desc.Allocate(cpCount, triCount);

	// read positions
	FbxVector4* points = pMesh->GetControlPoints();
	for (int i = 0; i < desc.m_NumVertices; i++)
		desc.m_pVertices[i] = gfsdk_makeFloat3(float(points[i].mData[0]), float(points[i].mData[1]), float(points[i].mData[2]));

	memset(desc.m_pTexCoords, 0, sizeof(atcore_float2) * desc.m_NumTriangles * 3);
	
	// read uvs 
	FbxLayerElementUV* leUV = pMesh->GetLayer(0)->GetUVs();

	FbxGeometryElementNormal *lNormals = pMesh->GetElementNormal();
	FbxGeometryElementTangent* lTangents = pMesh->GetElementTangent();

	int tcnt = 0;
	int vertexBase = 0;
	for (int i = 0; i < pMesh->GetPolygonCount(); i++)
	{
		int vcnt = pMesh->GetPolygonSize(i);
		for (int j = 0; j < (vcnt-2); j++)
		{	
			int pIndex[3] = { 0, j+1, j+2 };

			// get polygon index
			for (int p = 0; p < 3; ++p)
				desc.m_pIndices[tcnt*3+p] = pMesh->GetPolygonVertex(i, pIndex[p]);	
			
			// get face normal
			if (lNormals)
			{
				FbxLayerElement::EMappingMode mode = lNormals->GetMappingMode();

				for (int p = 0; p < 3; ++p)
				{
					int vertexId = vertexBase + pIndex[p];
					int normalId = vertexId;
					if (mode == FbxGeometryElement::eByPolygonVertex) 
					{
						if (lNormals->GetReferenceMode() == FbxGeometryElement::eIndexToDirect) 
							normalId = lNormals->GetIndexArray().GetAt(vertexId);
					}
					else if (mode == FbxGeometryElement::eByControlPoint)
						normalId = pMesh->GetPolygonVertex(i, pIndex[p]);

					const FbxVector4 &n = lNormals->GetDirectArray().GetAt(normalId);
					desc.m_pVertexNormals[tcnt*3+p] = gfsdk_makeFloat3((float)n[0], (float)n[1], (float)n[2]);
				}
			}

			// get face normal
			if (lTangents)
			{
				FbxLayerElement::EMappingMode mode = lTangents->GetMappingMode();

				for (int p = 0; p < 3; ++p)
				{
					int vertexId = vertexBase + pIndex[p];
					int tangentId = vertexId;
					if (mode == FbxGeometryElement::eByPolygonVertex) 
					{
						if (lTangents->GetReferenceMode() == FbxGeometryElement::eIndexToDirect) 
							tangentId = lTangents->GetIndexArray().GetAt(vertexId);
					}
					else if (mode == FbxGeometryElement::eByControlPoint)
						tangentId = pMesh->GetPolygonVertex(i, pIndex[p]);

					const FbxVector4 &n = lTangents->GetDirectArray().GetAt(tangentId);
					desc.m_pTangents[tcnt*3+p] = gfsdk_makeFloat3((float)n[0], (float)n[1], (float)n[2]);
				}
			}

			if (leUV)
			{
				int i0 = pMesh->GetTextureUVIndex(i, 0);
				int i1 = pMesh->GetTextureUVIndex(i, j+1);
				int i2 = pMesh->GetTextureUVIndex(i, j+2);

				FbxVector2 texCoord0 = leUV->GetDirectArray().GetAt(i0);
				FbxVector2 texCoord1 = leUV->GetDirectArray().GetAt(i1);
				FbxVector2 texCoord2 = leUV->GetDirectArray().GetAt(i2);

				desc.m_pTexCoords[tcnt*3+0] = gfsdk_makeFloat2(1.0f * float(texCoord0[0]), 1.0f - float(texCoord0[1]));
				desc.m_pTexCoords[tcnt*3+1] = gfsdk_makeFloat2(1.0f * float(texCoord1[0]), 1.0f - float(texCoord1[1]));
				desc.m_pTexCoords[tcnt*3+2] = gfsdk_makeFloat2(1.0f * float(texCoord2[0]), 1.0f - float(texCoord2[1]));
			}
			
			tcnt++;			
		}
		vertexBase += vcnt;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
bool FbxUtil::CreateHairFromFbx(const char* guideName, const char* growthMeshName, NvHair::AssetDescriptor& hairAsset)
{
	if (!s_FbxScene)
		return false;

	// read growth mesh data
	MeshDesc meshDesc;
	if (false == FbxUtil::CreateMeshDescriptor(growthMeshName, meshDesc))
		return false;

	FbxNode* pHairNode = FindNodeByName(s_FbxScene, guideName);
	if (!pHairNode)
		return false;

	FbxNode* pMeshNode = FindNodeByName(s_FbxScene, growthMeshName);
	if (!pMeshNode)
		return false;

	FbxSkin* pFbxSkin = GetFbxSkin(pMeshNode);
	int numBones = 1;

	if (pFbxSkin)
		numBones = (int)pFbxSkin->GetClusterCount();

	// get skin data from mesh
	int numPoints = meshDesc.m_NumVertices;

	atcore_float4* pMeshBoneIndices = new atcore_float4[numPoints];
	atcore_float4* pMeshBoneWeights = new atcore_float4[numPoints];

	GetSkinningWeights(pFbxSkin, pMeshBoneIndices, pMeshBoneWeights, numPoints);

	// create raw hair array
	FbxLine* pLine = pHairNode->GetLine();
	if (!pLine)
		return false;

	int cpCount = pLine->GetControlPointsCount();
	int curveCount = pLine->GetEndPointCount();

	atcore_float3*	pVertices	= new atcore_float3[cpCount];
	NvUInt32*		pEndIndices = new NvUInt32[curveCount];

	FbxVector4*		pFbxPoints		= pLine->GetControlPoints();
	FbxArray<int>*	pFbxEndIndices	= pLine->GetEndPointArray();

	for (int i = 0; i < cpCount; i++)
		pVertices[i] = gfsdk_makeFloat3(float(pFbxPoints[i][0]), float(pFbxPoints[i][1]), float(pFbxPoints[i][2]));

	for (int i = 0; i < curveCount; i++)
		pEndIndices[i] = (*pFbxEndIndices)[i];

	// check against closest growth mesh points
	const float distThreshold = 25.0;

	int numValidHairs = 0;
	int numValidHairVertices = 0;

	int* pMeshToHairMap = new int[meshDesc.m_NumVertices];
	int* pHairToMeshMap = new int[curveCount];
	int* pHairToHairMap = new int[curveCount];
	float* pMinDistances = new float[meshDesc.m_NumVertices];
	atcore_float3* pRootVertices = new atcore_float3[curveCount];
	atcore_float3* pTriangleCenters = new atcore_float3[meshDesc.m_NumTriangles];

	// initialize triangle centers
	for (int i = 0; i < meshDesc.m_NumTriangles; i++)
	{
		NvUInt32 v0 = meshDesc.m_pIndices[i*3 + 0];
		NvUInt32 v1 = meshDesc.m_pIndices[i * 3 + 1];
		NvUInt32 v2 = meshDesc.m_pIndices[i * 3 + 2];

		atcore_float3 p0 = meshDesc.m_pVertices[v0];
		atcore_float3 p1 = meshDesc.m_pVertices[v1];
		atcore_float3 p2 = meshDesc.m_pVertices[v2];

		pTriangleCenters[i] = 1.0f / 3.0f * (p0 + p1 + p2);
	}

	// initialize mesh to hair map
	for (int i = 0; i < meshDesc.m_NumVertices; i++)
	{
		pMeshToHairMap[i] = -1;
		pMinDistances[i] = FLT_MAX;
	}

	// copy root vertices of input hairs

	for (int i = 0; i < curveCount; i++)
	{
		int root = (i == 0) ? 0 : pEndIndices[i-1]+1;
		pRootVertices[i] = pVertices[root];
		pHairToMeshMap[i] = -1;
	}

	// for each input hair curve, find the closest mesh vertex
	for (int i = 0; i < curveCount; i++)
	{
		atcore_float3 hp = pRootVertices[i];

		float minDist = FLT_MAX;
		int closestTriangle = -1;
		for (int j = 0; j < meshDesc.m_NumTriangles; j++)
		{
			atcore_float3 c = pTriangleCenters[j];

			float distSquared = gfsdk_lengthSquared(hp - c);
			if (distSquared < minDist) 
			{
				minDist = distSquared;
				closestTriangle = j;
			}
		}

		if (closestTriangle >= 0)
		{
			for (int k = 0; k < 3; k++)
			{
				NvUInt32 v = meshDesc.m_pIndices[closestTriangle * 3 + k];
				atcore_float3 p = meshDesc.m_pVertices[v];

				float distSquared = gfsdk_lengthSquared(hp - p);
				if (distSquared < pMinDistances[v])
				{
					pMinDistances[v] = distSquared;
					pMeshToHairMap[v] = i;
				}
			}
		}
	}

	// prepare mapping from new hair set to mesh and old hairs
	for (int i = 0; i < meshDesc.m_NumVertices; i++)
	{
		int closestHair = pMeshToHairMap[i];
		if (closestHair < 0)
			continue;

		pHairToMeshMap[numValidHairs] = i;
		pHairToHairMap[numValidHairs] = closestHair;
		pMeshToHairMap[i] = numValidHairs; // update hair with new hair index

		int root	= (closestHair == 0) ? 0 : pEndIndices[closestHair-1]+1;
		int tip		= pEndIndices[closestHair];

		numValidHairVertices += (tip - root + 1);
		numValidHairs++;
	}

	// allocate new hairs
	hairAsset.m_numGuideHairs	= numValidHairs;
	hairAsset.m_numVertices		= numValidHairVertices;

	hairAsset.m_boneIndices	= new atcore_float4[hairAsset.m_numGuideHairs];
	hairAsset.m_boneWeights	= new atcore_float4[hairAsset.m_numGuideHairs];

	hairAsset.m_vertices		= new atcore_float3[hairAsset.m_numVertices];
	hairAsset.m_endIndices = new NvUInt32[hairAsset.m_numGuideHairs];
	hairAsset.m_faceIndices = new NvUInt32[meshDesc.m_NumTriangles * 3];
	hairAsset.m_faceUvs		= new atcore_float2[meshDesc.m_NumTriangles * 3];

	hairAsset.m_numBones		= numBones;
	hairAsset.m_boneNames		= new NvChar[NV_HAIR_MAX_STRING * hairAsset.m_numBones];
	hairAsset.m_bindPoses		= new atcore_float4x4[hairAsset.m_numBones];
	hairAsset.m_boneParents	= new NvInt32[hairAsset.m_numBones];

	if (pFbxSkin)
		GetBoneData(pFbxSkin, pMeshNode, hairAsset.m_boneNames, hairAsset.m_bindPoses);
	else
	{
		strcpy(hairAsset.m_boneNames, "Root");
		gfsdk_makeIdentity(hairAsset.m_bindPoses[0]);
		hairAsset.m_boneParents[0] = -1;
	}

	// copy vertex data
	int vertexCnt = 0;

	for (int i = 0; i < hairAsset.m_numGuideHairs; i++)
	{
		int oldHair = pHairToHairMap[i];

		int oldRoot = (oldHair == 0) ? 0 : pEndIndices[oldHair-1]+1;
		int oldTip	= pEndIndices[oldHair];

		int offset = oldTip - oldRoot;

		int newRoot = vertexCnt;
		int newTip	= vertexCnt + offset;

		vertexCnt += (offset + 1);

		hairAsset.m_endIndices[i] = newTip;

		int meshVertex = pHairToMeshMap[i];
		atcore_float3 meshRoot = meshDesc.m_pVertices[meshVertex];
		atcore_float3 hairRoot = pVertices[oldRoot];

		atcore_float3 rootOffset = meshRoot - hairRoot;

		for (int j = 0; j <= offset; j++)
			hairAsset.m_vertices[newRoot + j] = pVertices[oldRoot + j] + rootOffset;

		hairAsset.m_boneIndices[i] = gfsdk_makeFloat4(0,0,0,0);
		hairAsset.m_boneWeights[i] = gfsdk_makeFloat4(1,0,0,0);

		if (pFbxSkin)
		{
			hairAsset.m_boneIndices[i] = pMeshBoneIndices[meshVertex];
			hairAsset.m_boneWeights[i] = pMeshBoneWeights[meshVertex];
		}
	}

	// copy face indices and texture uvs
	int faceCnt = 0;
	for (int i = 0; i < meshDesc.m_NumTriangles; i++)
	{
		NvUInt32 i0 = meshDesc.m_pIndices[i * 3 + 0];
		NvUInt32 i1 = meshDesc.m_pIndices[i * 3 + 1];
		NvUInt32 i2 = meshDesc.m_pIndices[i * 3 + 2];

		int h0 = pMeshToHairMap[i0];
		int h1 = pMeshToHairMap[i1];
		int h2 = pMeshToHairMap[i2];

		if ((h0 < 0) || (h1 < 0) || (h2 < 0))
			continue; // invalid face

		hairAsset.m_faceIndices[ faceCnt * 3 + 0] = h0;
		hairAsset.m_faceIndices[ faceCnt * 3 + 1] = h1;
		hairAsset.m_faceIndices[ faceCnt * 3 + 2] = h2;

		hairAsset.m_faceUvs[ faceCnt * 3 + 0 ] = meshDesc.m_pTexCoords[i * 3 + 0];
		hairAsset.m_faceUvs[ faceCnt * 3 + 1 ] = meshDesc.m_pTexCoords[i * 3 + 1];
		hairAsset.m_faceUvs[ faceCnt * 3 + 2 ] = meshDesc.m_pTexCoords[i * 3 + 2];

		faceCnt++;
	}

	hairAsset.m_numFaces = faceCnt;

	delete []pMeshBoneIndices;
	delete []pMeshBoneWeights;

	delete []pMeshToHairMap;
	delete []pHairToMeshMap;
	delete []pHairToHairMap;

	return true;
}
*/
