#include "FbxFileWriter.h"
#include "fbxsdk.h"
#include "FbxUtils.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include "NvBlastTypes.h"
#include "NvBlastTkFramework.h"
#include "NvBlast.h"
#include "PxVec3.h"
#include "NvBlastAssert.h"
#include <unordered_set>
#include <functional>


FbxFileWriter::FbxFileWriter():
	bOutputFBXAscii(false)
{
	// Wrap in a shared ptr so that when it deallocates we get an auto destroy and all of the other assets created don't leak.
	sdkManager = std::shared_ptr<FbxManager>(FbxManager::Create(), [=](FbxManager* manager)
	{
		std::cout << "Deleting FbxManager" << std::endl;
		manager->Destroy();
	});

	setConvertToUE4(false);
}

/*

	Add the NvBlastAsset as a param
	Walk the NvBlastChunk tree
	-- Get the triangles for each chunk, however we do that
	-- create a skin, clusters and bone node for each chunk, linked to their parent with the proper link mode

*/
bool FbxFileWriter::saveToFile(const NvBlastAsset* asset, std::vector<std::vector<Nv::Blast::Triangle>> chunksGeometry, std::string assetName, std::string outputPath)
{

	FbxIOSettings* ios = FbxIOSettings::Create(sdkManager.get(), IOSROOT);
	// Set some properties on the io settings

//	ios->SetBoolProp(EXP_ASCIIFBX, true);

	sdkManager->SetIOSettings(ios);

	sdkManager->GetIOSettings()->SetBoolProp(EXP_ASCIIFBX, bOutputFBXAscii);

	FbxScene* scene = FbxScene::Create(sdkManager.get(), "Export Scene");

	if (getConvertToUE4())
	{
		FbxAxisSystem::EFrontVector FrontVector = (FbxAxisSystem::EFrontVector) - FbxAxisSystem::eParityOdd;
		const FbxAxisSystem UnrealZUp(FbxAxisSystem::eZAxis, FrontVector, FbxAxisSystem::eRightHanded);

		scene->GetGlobalSettings().SetAxisSystem(UnrealZUp);
	}

	// Otherwise default to Maya defaults

	FbxMesh* mesh = FbxMesh::Create(sdkManager.get(), "meshgeo");

	FbxGeometryElementNormal* geNormal = mesh->CreateElementNormal();
	geNormal->SetMappingMode(FbxGeometryElement::eByControlPoint);
	geNormal->SetReferenceMode(FbxGeometryElement::eDirect);

	FbxGeometryElementUV* geUV = mesh->CreateElementUV("diffuseElement");
	geUV->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
	geUV->SetReferenceMode(FbxGeometryElement::eDirect);

	// Get the triangles count for all of the mesh parts

	size_t triangleCount = 0;
	for (auto triangles : chunksGeometry)
	{
		triangleCount += triangles.size();
	}

	mesh->InitControlPoints((int)triangleCount * 3);
	
	FbxNode* meshNode = FbxNode::Create(scene, "meshnode");
	meshNode->SetNodeAttribute(mesh);
	meshNode->SetShadingMode(FbxNode::eTextureShading);

	FbxNode* lRootNode = scene->GetRootNode();
	lRootNode->AddChild(meshNode);

	FbxSkin* skin = FbxSkin::Create(sdkManager.get(), "Skin of the thing");
	skin->SetGeometry(mesh);

	mesh->AddDeformer(skin);

	// Add a material otherwise UE4 freaks out on import

	FbxGeometryElementMaterial* matElement = mesh->CreateElementMaterial();
	matElement->SetMappingMode(FbxGeometryElement::eByPolygon);
	matElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

	FbxSurfacePhong* material = FbxSurfacePhong::Create(sdkManager.get(), "FirstExportMaterial");

	material->Diffuse.Set(FbxDouble3(1.0, 1.0, 0));
	material->DiffuseFactor.Set(1.0);

	meshNode->AddMaterial(material);

	FbxSurfacePhong* material2 = FbxSurfacePhong::Create(sdkManager.get(), "SecondExportMaterial");

	material2->Diffuse.Set(FbxDouble3(1.0, 0.0, 1.0));
	material2->DiffuseFactor.Set(1.0);

	meshNode->AddMaterial(material2);

	// Now walk the tree and create a skeleton with geometry at the same time
	// Find a "root" chunk and walk the tree from there.
	uint32_t chunkCount = NvBlastAssetGetChunkCount(asset, NvBlastTkFrameworkGet()->getLogFn());

	auto chunks = NvBlastAssetGetChunks(asset, NvBlastTkFrameworkGet()->getLogFn());

	currentDepth = 0;
	uint32_t cpIdx = 0;
	for (uint32_t i = 0; i < chunkCount; i++)
	{
		const NvBlastChunk* chunk = &chunks[i];

		if (chunk->parentChunkIndex == UINT32_MAX)
		{
			uint32_t addedCps = createChunkRecursive(cpIdx, i, meshNode, lRootNode, skin, asset, chunksGeometry);

			cpIdx += addedCps;
		}
	}	

	return finalizeFbxAndSave(scene, skin, outputPath, assetName);}

/*
	Recursive method that creates this chunk and all it's children.

	This creates a FbxNode with an FbxCluster, and all of the geometry for this chunk.

	Returns the number of added control points
*/
uint32_t FbxFileWriter::createChunkRecursive(uint32_t currentCpIdx, uint32_t chunkIndex, FbxNode *meshNode, FbxNode* parentNode, FbxSkin* skin, const NvBlastAsset* asset, std::vector<std::vector<Nv::Blast::Triangle>> chunksGeometry)
{
	currentDepth++;

//  	if (currentDepth >= 4)
//  	{
//  		return 0;
//  	}

	auto chunks = NvBlastAssetGetChunks(asset, NvBlastTkFrameworkGet()->getLogFn());
	const NvBlastChunk* chunk = &chunks[chunkIndex];
	auto triangles = chunksGeometry[chunkIndex];
	physx::PxVec3 centroid = physx::PxVec3(chunk->centroid[0], chunk->centroid[1], chunk->centroid[2]);

	std::ostringstream namestream;

	//mesh->InitTextureUV(triangles.size() * 3);

	std::ostringstream().swap(namestream); // Swap namestream with a default constructed ostringstream
	namestream << "bone_" << chunkIndex;
	std::string boneName = namestream.str();

	FbxSkeleton* skelAttrib;
	if (chunk->parentChunkIndex == UINT32_MAX)
	{
		skelAttrib = FbxSkeleton::Create(sdkManager.get(), "SkelRootAttrib");
		skelAttrib->SetSkeletonType(FbxSkeleton::eRoot);

		// Change the centroid to origin
		centroid = physx::PxVec3(0.0f);
	}
	else
	{
		skelAttrib = FbxSkeleton::Create(sdkManager.get(), boneName.c_str());
		skelAttrib->SetSkeletonType(FbxSkeleton::eLimbNode);
	}

	skelAttrib->Size.Set(1.0); // What's this for?


	FbxNode* boneNode = FbxNode::Create(sdkManager.get(), boneName.c_str());
	boneNode->SetNodeAttribute(skelAttrib);

	auto mat = parentNode->EvaluateGlobalTransform().Inverse();

	FbxVector4 vec(centroid.x, centroid.y, centroid.z, 0);
	FbxVector4 c2 = mat.MultT(vec);

	boneNode->LclTranslation.Set(c2);
	
	parentNode->AddChild(boneNode);

	std::ostringstream().swap(namestream); // Swap namestream with a default constructed ostringstream
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
	for (auto tri : triangles)
	{
		addVert(tri.a, currentCpIdx + cpIdx + 0);
		addVert(tri.b, currentCpIdx + cpIdx + 1);
		addVert(tri.c, currentCpIdx + cpIdx + 2);

		mesh->BeginPolygon();
		mesh->AddPolygon(currentCpIdx + cpIdx + 0);
		mesh->AddPolygon(currentCpIdx + cpIdx + 1);
		mesh->AddPolygon(currentCpIdx + cpIdx + 2);
		mesh->EndPolygon();
		if (tri.userInfo == 0)
		{
			matElement->GetIndexArray().SetAt(polyCount, 0);
		}
		else
		{
			matElement->GetIndexArray().SetAt(polyCount, 1);
		}
		polyCount++;
		cpIdx += 3;
	}
		
	mat = meshNode->EvaluateGlobalTransform();
	cluster->SetTransformMatrix(mat);

	mat = boneNode->EvaluateGlobalTransform();
	cluster->SetTransformLinkMatrix(mat);

	uint32_t addedCps = static_cast<uint32_t>(triangles.size() * 3);

	for (uint32_t i = chunk->firstChildIndex; i < chunk->childIndexStop; i++)
	{
		addedCps += createChunkRecursive(currentCpIdx + addedCps, i, meshNode, boneNode, skin, asset, chunksGeometry);
	}

	return addedCps;
}


uint32_t FbxFileWriter::createChunkRecursive(uint32_t currentCpIdx, uint32_t chunkIndex, FbxNode *meshNode, FbxNode* parentNode, FbxSkin* skin, const NvBlastAsset* asset,
	const std::vector<std::vector<std::vector<int32_t> > >& posIndex,
	const std::vector<std::vector<std::vector<int32_t> > >& normIndex,
	const std::vector<std::vector<std::vector<int32_t> > >& texIndex)
{
		currentDepth++;

		//  	if (currentDepth >= 4)
		//  	{
		//  		return 0;
		//  	}

		auto chunks = NvBlastAssetGetChunks(asset, NvBlastTkFrameworkGet()->getLogFn());
		const NvBlastChunk* chunk = &chunks[chunkIndex];
		physx::PxVec3 centroid = physx::PxVec3(chunk->centroid[0], chunk->centroid[1], chunk->centroid[2]);

		std::ostringstream namestream;

		//mesh->InitTextureUV(triangles.size() * 3);

		std::ostringstream().swap(namestream); // Swap namestream with a default constructed ostringstream
		namestream << "bone_" << chunkIndex;
		std::string boneName = namestream.str();

		FbxSkeleton* skelAttrib;
		if (chunk->parentChunkIndex == UINT32_MAX)
		{
			skelAttrib = FbxSkeleton::Create(sdkManager.get(), "SkelRootAttrib");
			skelAttrib->SetSkeletonType(FbxSkeleton::eRoot);

			// Change the centroid to origin
			centroid = physx::PxVec3(0.0f);
		}
		else
		{
			skelAttrib = FbxSkeleton::Create(sdkManager.get(), boneName.c_str());
			skelAttrib->SetSkeletonType(FbxSkeleton::eLimbNode);
		}

		skelAttrib->Size.Set(1.0); // What's this for?


		FbxNode* boneNode = FbxNode::Create(sdkManager.get(), boneName.c_str());
		boneNode->SetNodeAttribute(skelAttrib);

		auto mat = parentNode->EvaluateGlobalTransform().Inverse();

		FbxVector4 vec(centroid.x, centroid.y, centroid.z, 0);
		FbxVector4 c2 = mat.MultT(vec);

		boneNode->LclTranslation.Set(c2);

		parentNode->AddChild(boneNode);

		std::ostringstream().swap(namestream); // Swap namestream with a default constructed ostringstream
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

		

		auto posIndices = posIndex[chunkIndex];
		auto normIndices = normIndex[chunkIndex];
		auto uvIndices = texIndex[chunkIndex];
		uint32_t cPolygCount = mesh->GetPolygonCount();
		int32_t addedVertices = 0;
			for (uint32_t subMesh = 0; subMesh < posIndices.size(); ++subMesh)
			{
				for (uint32_t tr = 0; tr < posIndices[subMesh].size(); tr += 3)
				{
					mesh->BeginPolygon();
					mesh->AddPolygon(posIndices[subMesh][tr + 0]);
					mesh->AddPolygon(posIndices[subMesh][tr + 1]);
					mesh->AddPolygon(posIndices[subMesh][tr + 2]);
					mesh->EndPolygon();
					geNormal->GetIndexArray().SetAt(currentCpIdx + addedVertices, normIndices[subMesh][tr + 0]);
					geNormal->GetIndexArray().SetAt(currentCpIdx + addedVertices + 1, normIndices[subMesh][tr + 1]);
					geNormal->GetIndexArray().SetAt(currentCpIdx + addedVertices + 2, normIndices[subMesh][tr + 2]);

					geUV->GetIndexArray().SetAt(currentCpIdx + addedVertices, uvIndices[subMesh][tr + 0]);
					geUV->GetIndexArray().SetAt(currentCpIdx + addedVertices + 1, uvIndices[subMesh][tr + 1]);
					geUV->GetIndexArray().SetAt(currentCpIdx + addedVertices + 2, uvIndices[subMesh][tr + 2]);
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

					cluster->AddControlPointIndex(posIndices[subMesh][tr + 0], 1.0);
					cluster->AddControlPointIndex(posIndices[subMesh][tr + 1], 1.0);
					cluster->AddControlPointIndex(posIndices[subMesh][tr + 2], 1.0);
				}
			}
		mat = meshNode->EvaluateGlobalTransform();
		cluster->SetTransformMatrix(mat);

		mat = boneNode->EvaluateGlobalTransform();
		cluster->SetTransformLinkMatrix(mat);


		for (uint32_t i = chunk->firstChildIndex; i < chunk->childIndexStop; i++)
		{
			 addedVertices +=createChunkRecursive(addedVertices, i, meshNode, boneNode, skin, asset, posIndex, normIndex, texIndex);
		}

		return addedVertices;
	
}

void FbxFileWriter::addControlPoints(FbxMesh* mesh, const std::vector<physx::PxVec3>& pos, std::vector<std::vector<std::vector<int32_t> > >& posIndex)
{	
	std::vector<uint32_t> vertices;
	std::cout << "Adding control points" << std::endl;
	std::vector<int32_t> mapping(pos.size(), -1);
	for (uint32_t ch = 0; ch < posIndex.size(); ++ch)
	{
		mapping.assign(pos.size(), -1);
		for (uint32_t sb = 0; sb < posIndex[ch].size(); ++sb)
		{
			for (uint32_t pi = 0; pi < posIndex[ch][sb].size(); ++pi)
			{
				uint32_t p = posIndex[ch][sb][pi];
				if (mapping[p] == -1)
				{
					mapping[p] = (int)vertices.size();
					vertices.push_back(posIndex[ch][sb][pi]);
					posIndex[ch][sb][pi] = mapping[p];
				}
				else
				{
					posIndex[ch][sb][pi] = mapping[p];
				}
			}		
		}
	}
	mesh->InitControlPoints((int)vertices.size());
	FbxVector4* controlPoints = mesh->GetControlPoints();
	for (auto v : vertices)
	{
		*controlPoints = FbxVector4(pos[v].x, pos[v].y, pos[v].z, 0);
		++controlPoints;
	}
	std::cout << "Adding control points: done" << std::endl;
}

bool FbxFileWriter::finalizeFbxAndSave(FbxScene* scene, FbxSkin* skin, const std::string& outputPath, const std::string& name)
{
	// Store the bind pose

	std::unordered_set<FbxNode*> clusterNodes;

	std::function<void(FbxNode*)> addRecursively = [&](FbxNode* node)
	{
		if (node)
		{
			addRecursively(node->GetParent());

			clusterNodes.insert(node);
		}
	};

	for (uint32_t i = 0; i < (uint32_t)skin->GetClusterCount(); i++)
	{
		FbxNode* clusterNode = skin->GetCluster(i)->GetLink();

		addRecursively(clusterNode);
	}

	NVBLAST_ASSERT(clusterNodes.size() > 0);

	FbxPose* pose = FbxPose::Create(sdkManager.get(), "BasePose");
	pose->SetIsBindPose(true);

	for (auto node : clusterNodes)
	{
		FbxMatrix bindMat = node->EvaluateGlobalTransform();

		pose->Add(node, bindMat);
	}

	scene->AddPose(pose);

	FbxExporter* exporter = FbxExporter::Create(sdkManager.get(), "Scene Exporter");

	auto path = outputPath + "\\" + name + ".fbx";

	int lFormat;

	if (bOutputFBXAscii)
	{
		lFormat = sdkManager->GetIOPluginRegistry()->FindWriterIDByDescription("FBX ascii (*.fbx)");
	}
	else
	{
		lFormat = sdkManager->GetIOPluginRegistry()->FindWriterIDByDescription("FBX binary (*.fbx)");
	}

	bool exportStatus = exporter->Initialize(path.c_str(), lFormat, sdkManager->GetIOSettings());

	if (!exportStatus)
	{
		std::cerr << "Call to FbxExporter::Initialize failed" << std::endl;
		std::cerr << "Error returned: " << exporter->GetStatus().GetErrorString() << std::endl;
		return false;
	}

	exportStatus = exporter->Export(scene);

	if (!exportStatus)
	{
		auto fbxStatus = exporter->GetStatus();

		std::cerr << "Call to FbxExporter::Export failed" << std::endl;
		std::cerr << "Error returned: " << fbxStatus.GetErrorString() << std::endl;
		return false;
	}

	return true;
}

bool FbxFileWriter::saveToFile(const NvBlastAsset* asset, const std::string& name, const std::string& outputPath, const std::vector<physx::PxVec3>& pos, const std::vector<physx::PxVec3>& norm,
	const std::vector<physx::PxVec2>& uvs,
	const std::vector<std::vector<std::vector<int32_t> > >& posIndexInp,
	const std::vector<std::vector<std::vector<int32_t> > >& normIndex,
	const std::vector<std::vector<std::vector<int32_t> > >& texIndex,
	const std::vector<std::string>& texPathes, 
	const uint32_t submeshCount)
{
	NV_UNUSED(submeshCount);
	NV_UNUSED(texPathes);

	auto posIndex = posIndexInp;
	/**
		Get polygon count
	*/
	uint32_t polygCount = 0;
	for (uint32_t ch = 0; ch < posIndex.size(); ++ch)
	{
		for (uint32_t sbm = 0; sbm < posIndex[ch].size(); ++sbm)
		{
			polygCount += (uint32_t)posIndex[ch][sbm].size() / 3;
		}
	}

	FbxIOSettings* ios = FbxIOSettings::Create(sdkManager.get(), IOSROOT);
	// Set some properties on the io settings

	//	ios->SetBoolProp(EXP_ASCIIFBX, true);

	sdkManager->SetIOSettings(ios);

	sdkManager->GetIOSettings()->SetBoolProp(EXP_ASCIIFBX, bOutputFBXAscii);

	FbxScene* scene = FbxScene::Create(sdkManager.get(), "Export Scene");

	if (getConvertToUE4())
	{
		FbxAxisSystem::EFrontVector FrontVector = (FbxAxisSystem::EFrontVector) - FbxAxisSystem::eParityOdd;
		const FbxAxisSystem UnrealZUp(FbxAxisSystem::eZAxis, FrontVector, FbxAxisSystem::eRightHanded);

		scene->GetGlobalSettings().SetAxisSystem(UnrealZUp);
	}

	// Otherwise default to Maya defaults

	FbxMesh* mesh = FbxMesh::Create(sdkManager.get(), "meshgeo");

	FbxGeometryElementNormal* geNormal = mesh->CreateElementNormal();
	geNormal->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
	geNormal->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

	FbxGeometryElementUV* geUV = mesh->CreateElementUV("diffuseElement");
	geUV->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
	geUV->SetReferenceMode(FbxGeometryElement::eIndexToDirect);


	FbxNode* meshNode = FbxNode::Create(scene, "meshnode");
	meshNode->SetNodeAttribute(mesh);
	meshNode->SetShadingMode(FbxNode::eTextureShading);

	FbxNode* lRootNode = scene->GetRootNode();
	lRootNode->AddChild(meshNode);

	FbxSkin* skin = FbxSkin::Create(sdkManager.get(), "Skin of the thing");
	skin->SetGeometry(mesh);

	mesh->AddDeformer(skin);

	/**
		Create control points, copy data to buffers
	*/
	addControlPoints(mesh, pos, posIndex);

	auto normalsElem = mesh->GetElementNormal();
	for (uint32_t i = 0; i < norm.size(); ++i)
	{
		normalsElem->GetDirectArray().Add(FbxVector4(norm[i].x, norm[i].y, norm[i].z, 0));
	}
	auto uvsElem = mesh->GetElementUV("diffuseElement");
	for (uint32_t i = 0; i < uvs.size(); ++i)
	{
		uvsElem->GetDirectArray().Add(FbxVector2(uvs[i].x, uvs[i].y));
	}


	// Add a material otherwise UE4 freaks out on import

	FbxGeometryElementMaterial* matElement = mesh->CreateElementMaterial();
	matElement->SetMappingMode(FbxGeometryElement::eByPolygon);
	matElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

	FbxSurfacePhong* material = FbxSurfacePhong::Create(sdkManager.get(), "FirstExportMaterial");

	material->Diffuse.Set(FbxDouble3(1.0, 1.0, 0));
	material->DiffuseFactor.Set(1.0);

	meshNode->AddMaterial(material);

	FbxSurfacePhong* material2 = FbxSurfacePhong::Create(sdkManager.get(), "SecondExportMaterial");

	material2->Diffuse.Set(FbxDouble3(1.0, 0.0, 1.0));
	material2->DiffuseFactor.Set(1.0);

	meshNode->AddMaterial(material2);


	matElement->GetIndexArray().SetCount(polygCount);
	normalsElem->GetIndexArray().SetCount(polygCount * 3);
	uvsElem->GetIndexArray().SetCount(polygCount * 3);


	std::cout << "Create chunks recursive" << std::endl;

	// Now walk the tree and create a skeleton with geometry at the same time
	// Find a "root" chunk and walk the tree from there.
	uint32_t chunkCount = NvBlastAssetGetChunkCount(asset, NvBlastTkFrameworkGet()->getLogFn());
	auto chunks = NvBlastAssetGetChunks(asset, NvBlastTkFrameworkGet()->getLogFn());
	currentDepth = 0;
	uint32_t cpIdx = 0;
	for (uint32_t i = 0; i < chunkCount; i++)
	{
		const NvBlastChunk* chunk = &chunks[i];

		if (chunk->parentChunkIndex == UINT32_MAX)
		{
			uint32_t addedCps = createChunkRecursive(cpIdx, i, meshNode, lRootNode, skin, asset, posIndex, normIndex, texIndex);
			cpIdx += addedCps;
		}
	}

	return finalizeFbxAndSave(scene, skin, outputPath, name);
}

bool FbxFileWriter::saveToFile(const NvBlastAsset* asset, const std::string& name, const std::string& outputPath, const std::vector<physx::PxVec3>& pos, const std::vector<physx::PxVec3>& norm, const std::vector<physx::PxVec2>& uvs,
	const std::vector<std::vector<std::vector<int32_t> > >& indices)
{
	std::vector<std::string> matnames;
	matnames.push_back("");
	return saveToFile(asset, name, outputPath, pos, norm, uvs, indices, indices, indices, matnames, 1);
}