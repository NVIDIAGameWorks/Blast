#include "FbxFileReader.h"
#include "fileio/fbxiosettings.h"
#include "fileio/fbxiosettingspath.h"
#include "core/base/fbxstringlist.h"
#include <iostream>
#include "scene/geometry/fbxmesh.h"

#include "PxVec3.h"
#include "PxVec2.h"

using physx::PxVec3;
using physx::PxVec2;
using Nv::Blast::Mesh;


FbxFileReader::FbxFileReader()
{
	setConvertToUE4(false);
}

FbxAMatrix FbxFileReader::getTransformForNode(FbxNode* node)
{
	return node->EvaluateGlobalTransform();
}

std::shared_ptr<Nv::Blast::Mesh> FbxFileReader::loadFromFile(std::string filename)
{
	// Wrap in a shared ptr so that when it deallocates we get an auto destroy and all of the other assets created don't leak.
	std::shared_ptr<FbxManager> sdkManager = std::shared_ptr<FbxManager>(FbxManager::Create(), [=](FbxManager* manager)
	{
		std::cout << "Deleting FbxManager" << std::endl;
		manager->Destroy();
	});

	FbxIOSettings* ios = FbxIOSettings::Create(sdkManager.get(), IOSROOT);
	// Set some properties on the io settings

	sdkManager->SetIOSettings(ios);

	
	FbxImporter* importer = FbxImporter::Create(sdkManager.get(), "");

	bool importStatus = importer->Initialize(filename.c_str(), -1, sdkManager->GetIOSettings());

	if (!importStatus)
	{
		std::cerr << "Call to FbxImporter::Initialize failed." << std::endl;
		std::cerr << "Error returned: " << importer->GetStatus().GetErrorString() << std::endl;

		return nullptr;
	}

	FbxScene* scene = FbxScene::Create(sdkManager.get(), "importScene");

	importStatus = importer->Import(scene);

	if (!importStatus)
	{
		std::cerr << "Call to FbxImporter::Import failed." << std::endl;
		std::cerr << "Error returned: " << importer->GetStatus().GetErrorString() << std::endl;

		return nullptr;
	}

	if (getConvertToUE4())
	{
		// Convert to UE4
		FbxAxisSystem::EFrontVector FrontVector = (FbxAxisSystem::EFrontVector) - FbxAxisSystem::eParityOdd;
		const FbxAxisSystem UnrealZUp(FbxAxisSystem::eZAxis, FrontVector, FbxAxisSystem::eRightHanded);

		UnrealZUp.ConvertScene(scene);
	}


	// Recurse the fbx tree and find all meshes
	std::vector<FbxNode*> meshNodes;
	getFbxMeshes(scene->GetRootNode(), meshNodes);

	std::cout << "Found " << meshNodes.size() << " meshes." << std::endl;

	// Process just 0, because dumb. Fail out if more than 1?

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
		return nullptr;
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
			normVec = trans.MultT(normVec);

			positions.push_back(PxVec3((float) vert[0], (float)vert[1], (float)vert[2]));
			normals.push_back(PxVec3((float)normVec[0], (float)normVec[1], (float)normVec[2]));
			uv.push_back(PxVec2((float)uvVec[0], (float)uvVec[1]));

			indices.push_back(vertIndex++);
		}

	}

	PxVec3* nr = (!normals.empty()) ? normals.data() : nullptr;
	PxVec2* uvp = (!uv.empty()) ? uv.data() : nullptr;

	return std::make_shared<Mesh>(positions.data(), nr, uvp, static_cast<uint32_t>(positions.size()), indices.data(), static_cast<uint32_t>(indices.size()));
}

void FbxFileReader::getFbxMeshes(FbxNode* node, std::vector<FbxNode*>& meshNodes)
{
	FbxMesh* mesh = node->GetMesh();

	if (mesh != nullptr)
	{
		meshNodes.push_back(node);
	}

	int childCount = node->GetChildCount();

	for (int i = 0; i < childCount; i++)
	{
		FbxNode * childNode = node->GetChild(i);

		getFbxMeshes(childNode, meshNodes);
	}
}
