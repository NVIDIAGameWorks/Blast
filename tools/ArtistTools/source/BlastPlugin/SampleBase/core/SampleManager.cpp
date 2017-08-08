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
// Copyright (c) 2008-2017 NVIDIA Corporation. All rights reserved.


#include "AppMainWindow.h"
#include "GlobalSettings.h"
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include "NvBlastExtAuthoringTypes.h"
#include "NvBlastExtAuthoringFractureTool.h"
#include "NvBlastExtAuthoringBondGenerator.h"
#include "NvBlastIndexFns.h"

#include "SampleManager.h"

#include "Utils.h"

#include "Renderer.h"
#include "PhysXController.h"
#include "BlastController.h"
#include "CommonUIController.h"
#include "DamageToolController.h"
#include "SelectionToolController.h"
#include "ExplodeToolController.h"
#include "GizmoToolController.h"
#include "EditionToolController.h"
#include "SceneController.h"
#include "SampleController.h"

#include "tclap/CmdLine.h"
#include "PxPhysics.h"
#include "PsFileBuffer.h"
#include "NvBlast.h"
#include "NvBlastExtAuthoringCollisionBuilder.h"
#include "NvBlastExtPxAsset.h"
#include "BlastFractureTool.h"
#include <set>
#include "MaterialLibraryPanel.h"
#include "MaterialAssignmentsPanel.h"
#include "ViewerOutput.h"

#include "NvBlastTkAsset.h"
#include "BlastAssetModelSimple.h"
#include "CorelibUtils.h"
#include "BlastAssetModel.h"
#include "SimpleScene.h"
#include "FileReferencesPanel.h"
#include "BlastPlugin.h"
#include "BlastToolBar.h"
#include "NvBlastExtAuthoring.h"
#include "NvBlastExtExporter.h"
using namespace physx;

const uint32_t DEFAULT_VORONOI_UNIFORM_SITES_NUMBER = 5;
physx::PxFoundation* foundation = nullptr;
physx::PxPhysics* physics = nullptr;
physx::PxCooking* cooking = nullptr;
Nv::Blast::ExtPxManager* physicsManager = nullptr;
SampleManager* sSampleManager = nullptr;

class SimpleRandomGenerator : public RandomGeneratorBase
{
public:
	SimpleRandomGenerator() {
		remember = false;
	};

	virtual float getRandomValue()
	{
		float r = (float)rand();
		r = r / RAND_MAX;
		return r;
	}

	virtual float getExponential(float lambda)
	{
		return -1.0f / lambda * log(1 - getRandomValue());
	}

	virtual void seed(int32_t seed)
	{
		srand(seed);
	}

	virtual ~SimpleRandomGenerator() {};

private:
	bool remember;
};
static SimpleRandomGenerator sRandomGenerator;

void loggingCallback(int type, const char* msg, const char* file, int line)
{
	(void)type;

	std::cout << msg << " FILE:" << file << " Line: " << line << "\n";
}

void buildPxChunks(const std::vector<std::vector<Triangle>>& chunkGeometry, std::vector<ExtPxAssetDesc::ChunkDesc>& pxChunks,
	std::vector<ExtPxAssetDesc::SubchunkDesc>& pxSubchunks, std::vector<bool>& statics)
{
	std::shared_ptr<Nv::Blast::ConvexMeshBuilder> collisionBuilder(
		NvBlastExtAuthoringCreateConvexMeshBuilder(cooking, &physics->getPhysicsInsertionCallback()),
		[](Nv::Blast::ConvexMeshBuilder* cmb) {cmb->release(); });

	pxChunks.resize(chunkGeometry.size());
	pxSubchunks.resize(chunkGeometry.size());

	for (uint32_t i = 0; i < chunkGeometry.size(); ++i)
	{
		std::vector<physx::PxVec3> vertices;
		for (uint32_t p = 0; p < chunkGeometry[i].size(); ++p)
		{
			vertices.push_back(chunkGeometry[i][p].a.p);
			vertices.push_back(chunkGeometry[i][p].b.p);
			vertices.push_back(chunkGeometry[i][p].c.p);
		}
		pxSubchunks[i].transform = physx::PxTransform(physx::PxIdentity);
		pxSubchunks[i].geometry = physx::PxConvexMeshGeometry(
			collisionBuilder.get()->buildConvexMesh(*collisionBuilder.get()->buildCollisionGeometry((uint32_t)vertices.size(), vertices.data())));
		pxChunks[i].isStatic = statics.size() == 0 ? false : statics[i];
		pxChunks[i].subchunkCount = 1;
		pxChunks[i].subchunks = &pxSubchunks[i];
	}

	// only effect when chunk is support
	pxChunks[0].isStatic = true;
}

void saveFractureToObj(std::vector<std::vector<Triangle> > chunksGeometry, std::string name, std::string path)
{
	std::vector<std::string> materialNames(2);
	std::vector<std::string> materialPaths(2);
	float diffuseColor[2][4];

	SampleManager* pSampleManager = SampleManager::ins();
	BlastAsset* pCurBlastAsset = nullptr;
	int nCurIndex = -1;
	pSampleManager->getCurrentSelectedInstance(&pCurBlastAsset, nCurIndex);
	if (pCurBlastAsset != nullptr && nCurIndex != -1)
	{
		pSampleManager->getMaterialForCurrentFamily(materialNames[0], true);
		pSampleManager->getMaterialForCurrentFamily(materialNames[1], false);

		BPPGraphicsMaterial* pMaterialEx = BlastProject::ins().getGraphicsMaterial(materialNames[0].c_str());
		BPPGraphicsMaterial* pMaterialIn = BlastProject::ins().getGraphicsMaterial(materialNames[1].c_str());

		diffuseColor[0][0] = pMaterialEx->diffuseColor.x;
		diffuseColor[0][1] = pMaterialEx->diffuseColor.y;
		diffuseColor[0][2] = pMaterialEx->diffuseColor.z;
		diffuseColor[0][3] = pMaterialEx->diffuseColor.w;
		if (pMaterialEx->diffuseTextureFilePath != nullptr)
		{
			materialPaths[0] = pMaterialEx->diffuseTextureFilePath;
		}

		if (pMaterialIn == nullptr)
		{
			pMaterialIn = pMaterialEx;
		}

		diffuseColor[1][0] = pMaterialIn->diffuseColor.x;
		diffuseColor[1][1] = pMaterialIn->diffuseColor.y;
		diffuseColor[1][2] = pMaterialIn->diffuseColor.z;
		diffuseColor[1][3] = pMaterialIn->diffuseColor.w;
		if (pMaterialIn->diffuseTextureFilePath != nullptr)
		{
			materialPaths[1] = pMaterialIn->diffuseTextureFilePath;
		}
	}
	else
	{
		MaterialAssignmentsPanel* pMaterialAssignmentsPanel = MaterialAssignmentsPanel::ins();
		pMaterialAssignmentsPanel->getMaterialNameAndPaths(materialNames, materialPaths);

		if (materialPaths[0] == "")
		{
			RenderMaterial* pMaterialEx = RenderMaterial::getDefaultRenderMaterial();
			pMaterialEx->getDiffuseColor(diffuseColor[0][0], diffuseColor[0][1], diffuseColor[0][2], diffuseColor[0][3]);
		}
		if (materialPaths[1] == "")
		{
			RenderMaterial* pMaterialIn = RenderMaterial::getDefaultRenderMaterial();
			pMaterialIn->getDiffuseColor(diffuseColor[1][0], diffuseColor[1][1], diffuseColor[1][2], diffuseColor[1][3]);
		}
	}

	uint32_t submeshCount = 2;
	// export materials (mtl file)
	{
		std::string mtlFilePath = GlobalSettings::MakeFileName(path.c_str(), std::string(name + ".mtl").c_str());
		FILE* f = fopen(mtlFilePath.c_str(), "w");
		if (!f)
			return;

		for (uint32_t submeshIndex = 0; submeshIndex < submeshCount; ++submeshIndex)
		{
			// Add By Lixu Begin
			std::string& matName = materialNames[submeshIndex];
			fprintf(f, "newmtl %s\n", matName.size()? matName.c_str() : "neverMat123XABCnever");  // this speical string is also used in another BlastModel.cpp.
			// Add By Lixu End
			fprintf(f, "\tmap_Kd %s\n", materialPaths[submeshIndex].c_str());
			fprintf(f, "\tKd %f %f %f\n", diffuseColor[submeshIndex][0], diffuseColor[submeshIndex][1], diffuseColor[submeshIndex][2]);
			fprintf(f, "\n");
		}

		fclose(f);
	}

	{
		std::string objFilePath = GlobalSettings::MakeFileName(path.c_str(), std::string(name + ".obj").c_str());
		FILE* outStream = fopen(objFilePath.c_str(), "w");

		fprintf(outStream, "mtllib %s.mtl\n", name.c_str());
		fprintf(outStream, "o frac \n");


		for (uint32_t vc = 0; vc < chunksGeometry.size(); ++vc)
		{
			std::vector<Triangle>& chunk = chunksGeometry[vc];
			for (uint32_t i = 0; i < chunk.size(); ++i)
			{
				fprintf(outStream, "v %lf %lf %lf\n", chunk[i].a.p.x, chunk[i].a.p.y, chunk[i].a.p.z);
				fprintf(outStream, "v %lf %lf %lf\n", chunk[i].b.p.x, chunk[i].b.p.y, chunk[i].b.p.z);
				fprintf(outStream, "v %lf %lf %lf\n", chunk[i].c.p.x, chunk[i].c.p.y, chunk[i].c.p.z);
			}

			for (uint32_t i = 0; i < chunk.size(); ++i)
			{
				fprintf(outStream, "vt %lf %lf \n", chunk[i].a.uv[0].x, chunk[i].a.uv[0].y);
				fprintf(outStream, "vt %lf %lf \n", chunk[i].b.uv[0].x, chunk[i].b.uv[0].y);
				fprintf(outStream, "vt %lf %lf \n", chunk[i].c.uv[0].x, chunk[i].c.uv[0].y);
			}

			for (uint32_t i = 0; i < chunk.size(); ++i)
			{
				fprintf(outStream, "vn %lf %lf %lf\n", chunk[i].a.n.x, chunk[i].a.n.y, chunk[i].a.n.z);
				fprintf(outStream, "vn %lf %lf %lf\n", chunk[i].b.n.x, chunk[i].b.n.y, chunk[i].b.n.z);
				fprintf(outStream, "vn %lf %lf %lf\n", chunk[i].c.n.x, chunk[i].c.n.y, chunk[i].c.n.z);
			}
		}
		int indx = 1;
		for (uint32_t vc = 0; vc < chunksGeometry.size(); ++vc)
		{
			fprintf(outStream, "g %d_%d \n", vc, 0);
			fprintf(outStream, "usemtl %s\n", materialNames[0].c_str());
			int totalSize = chunksGeometry[vc].size();
			std::vector<int> internalSurfaces;
			for (uint32_t i = 0; i < totalSize; ++i)
			{
				if (chunksGeometry[vc][i].materialId != 0)
				{
					internalSurfaces.push_back(indx++);
					internalSurfaces.push_back(indx++);
					internalSurfaces.push_back(indx++);
					continue;
				}
				fprintf(outStream, "f %d/%d/%d  ", indx, indx, indx);
				indx++;
				fprintf(outStream, "%d/%d/%d  ", indx, indx, indx);
				indx++;
				fprintf(outStream, "%d/%d/%d \n", indx, indx, indx);
				indx++;
			}
			int internalSize = internalSurfaces.size();
			if (internalSize > 0)
			{
				fprintf(outStream, "g %d_%d \n", vc, 1);
				fprintf(outStream, "usemtl %s\n", materialNames[1].c_str());
				int isIndex;
				for (uint32_t is = 0; is < internalSize;)
				{
					isIndex = internalSurfaces[is++];
					fprintf(outStream, "f %d/%d/%d  ", isIndex, isIndex, isIndex);
					isIndex = internalSurfaces[is++];
					fprintf(outStream, "%d/%d/%d  ", isIndex, isIndex, isIndex);
					isIndex = internalSurfaces[is++];
					fprintf(outStream, "%d/%d/%d \n", isIndex, isIndex, isIndex);
				}
			}
		}
		fclose(outStream);
	}
}

#include "NvBlastExtLlSerialization.h"
#include "NvBlastExtTkSerialization.h"
#include "NvBlastExtPxSerialization.h"
#include "NvBlastExtSerialization.h"

bool saveBlastObject(const std::string& outputDir, const std::string& objectName, const void* object, uint32_t objectTypeID)
{
	ExtSerialization* mSerialization = SampleManager::ins()->getBlastController().getExtSerialization();

	void* buffer;
	const uint64_t bufferSize = mSerialization->serializeIntoBuffer(buffer, object, objectTypeID);
	if (bufferSize == 0)
	{
		std::cerr << "saveBlastObject: Serialization failed.\n";
		return false;
	}

	// Add By Lixu Begin
	physx::PsFileBuffer fileBuf((outputDir + "/" + objectName).c_str(), physx::PxFileBuf::OPEN_WRITE_ONLY);
	// Add By Lixu End

	bool result = fileBuf.isOpen();

	if (!result)
	{
		std::cerr << "Can't open output buffer.\n";
	}
	else
	{
		result = (bufferSize == (size_t)fileBuf.write(buffer, (uint32_t)bufferSize));
		if (!result)
		{
			std::cerr << "Buffer write failed.\n";
		}
		fileBuf.close();
	}

	NVBLAST_FREE(buffer);

	return result;
}

bool saveLlAsset(const std::string& outputDir, const std::string& objectName, const NvBlastAsset* assetLL)
{
	return saveBlastObject(outputDir, objectName, assetLL, LlObjectTypeID::Asset);
}

bool saveTkAsset(const std::string& outputDir, const std::string& objectName, const TkAsset* tkAsset)
{
	return saveBlastObject(outputDir, objectName, tkAsset, TkObjectTypeID::Asset);
}

bool saveExtAsset(const std::string& outputDir, const std::string& objectName, const ExtPxAsset* pxAsset)
{
	return saveBlastObject(outputDir, objectName, pxAsset, ExtPxObjectTypeID::Asset);
}

void FractureExecutor::setSourceAsset(BlastAsset* blastAsset)
{
	assert(m_fractureTool);
	m_fractureTool->setSourceAsset(blastAsset);
	m_pCurBlastAsset = blastAsset;
}

VoronoiFractureExecutor::VoronoiFractureExecutor()
: m_voronoi(nullptr)
{
	if (sSampleManager)
		m_fractureTool = sSampleManager->m_fTool;
}

bool VoronoiFractureExecutor::execute()
{
	std::vector<uint32_t>::iterator it;
	for (it = m_chunkIds.begin(); it != m_chunkIds.end(); it++)
	{
		Nv::Blast::Mesh* mesh = m_fractureTool->getSourceMesh(*it);
		if (mesh == nullptr)
			continue;

		VoronoiSitesGenerator* siteGenerator = NvBlastExtAuthoringCreateVoronoiSitesGenerator(mesh, m_randomGenerator == nullptr ? &sRandomGenerator : m_randomGenerator);
		if (m_voronoi)
		{
//			siteGenerator = new VoronoiSitesGenerator(mesh, m_randomGenerator == nullptr ? &sRandomGenerator : m_randomGenerator);
			if (0 == m_voronoi->siteGeneration)
			{
				siteGenerator->uniformlyGenerateSitesInMesh(m_voronoi->numSites);
			}
			else if (1 == m_voronoi->siteGeneration)
			{
				siteGenerator->clusteredSitesGeneration(m_voronoi->numberOfClusters, m_voronoi->sitesPerCluster, m_voronoi->clusterRadius);
			}
		}
		else
		{
//			siteGenerator = new VoronoiSitesGenerator(mesh, m_randomGenerator == nullptr ? &sRandomGenerator : m_randomGenerator);
			siteGenerator->uniformlyGenerateSitesInMesh(DEFAULT_VORONOI_UNIFORM_SITES_NUMBER);
		}

		const physx::PxVec3* sites = nullptr;
		uint32_t sitesCount = siteGenerator->getVoronoiSites(sites);
		m_fractureTool->voronoiFracturing(*it, sitesCount, sites, false);
		delete siteGenerator;
	}
	m_fractureTool->finalizeFracturing();

	std::vector<bool> supports;
	std::vector<bool> statics;
	std::vector<uint8_t> joints;
	std::vector<uint32_t> worlds;
	BlastAsset* pNewBlastAsset = sSampleManager->_replaceAsset(m_pCurBlastAsset, supports, statics, joints, worlds);
	if (nullptr == pNewBlastAsset)
	{
		return false;
	}

	std::vector<uint32_t> NewChunkIndexes;
	for (uint32_t ci = 0; ci < m_fractureTool->getChunkCount(); ci++)
	{
		for (uint32_t chunkId : m_chunkIds)
		{
			if (m_fractureTool->getChunkInfo(ci).parent == chunkId)
			{
				NewChunkIndexes.push_back(ci);
			}
		}
	}

	sSampleManager->ApplyAutoSelectNewChunks(pNewBlastAsset, NewChunkIndexes);

	return true;
}

SliceFractureExecutor::SliceFractureExecutor()
: m_slice(nullptr)
{
	if (sSampleManager)
		m_fractureTool = sSampleManager->m_fTool;
}

bool SliceFractureExecutor::execute()
{
	SlicingConfiguration config;
	if (m_slice)
	{
		config.x_slices = m_slice->numSlicesX;
		config.y_slices = m_slice->numSlicesY;
		config.z_slices = m_slice->numSlicesZ;
		config.offset_variations = m_slice->offsetVariation;
		config.angle_variations = m_slice->rotationVariation;
		config.noiseAmplitude = m_slice->noiseAmplitude;
		config.noiseFrequency = m_slice->noiseFrequency;
		config.noiseOctaveNumber = m_slice->noiseOctaveNumber;
		config.surfaceResolution = m_slice->surfaceResolution;
	}

	if (m_randomGenerator == nullptr)
	{
		sRandomGenerator.seed(m_slice->noiseSeed);
	}
	else
	{
		m_randomGenerator->seed(m_slice->noiseSeed);
	}

	std::vector<uint32_t>::iterator it;
	for (it = m_chunkIds.begin(); it != m_chunkIds.end(); it++)
	{
		m_fractureTool->slicing(*it, config, false, (m_randomGenerator == nullptr ? &sRandomGenerator : m_randomGenerator));
	}
	m_fractureTool->finalizeFracturing();

	std::vector<bool> supports;
	std::vector<bool> statics;
	std::vector<uint8_t> joints;
	std::vector<uint32_t> worlds;
	BlastAsset* pNewBlastAsset = sSampleManager->_replaceAsset(m_pCurBlastAsset, supports, statics, joints, worlds);
	if (nullptr == pNewBlastAsset)
	{
		return false;
	}

	std::vector<uint32_t> NewChunkIndexes;
	for (uint32_t ci = 0; ci < m_fractureTool->getChunkCount(); ci++)
	{
		for (uint32_t chunkId : m_chunkIds)
		{
			if (m_fractureTool->getChunkInfo(ci).parent == chunkId)
			{
				NewChunkIndexes.push_back(ci);
			}
		}
	}

	sSampleManager->ApplyAutoSelectNewChunks(pNewBlastAsset, NewChunkIndexes);

	return true;
}

static VoronoiFractureExecutor sVoronoiFracture;

SampleManager* SampleManager::ins()
{
	return sSampleManager;
}

SampleManager::SampleManager(DeviceManager* pDeviceManager)
{
	sSampleManager = this;
	m_bNeedRefreshTree = false;

	m_renderer = new Renderer();
	m_physXController = new PhysXController(ExtImpactDamageManager::FilterShader);
	m_blastController = new BlastController();
	m_sceneController = new SceneController();
	m_damageToolController = new DamageToolController();
	m_selectionToolController = new SelectionToolController();
	m_explodeToolController = new ExplodeToolController();
	m_gizmoToolController = new GizmoToolController();
	m_editionToolController = new EditionToolController();
	m_sampleController = new SampleController();
	m_commonUIController = nullptr; // new CommonUIController();

	m_pApplication = new Application(pDeviceManager);

	Application& app = *m_pApplication;

	app.addControllerToFront(m_renderer);
	app.addControllerToFront(m_physXController);
	app.addControllerToFront(m_blastController);
	app.addControllerToFront(m_sceneController);
	app.addControllerToFront(m_damageToolController);
	app.addControllerToFront(m_selectionToolController);
	app.addControllerToFront(m_explodeToolController);
	app.addControllerToFront(m_gizmoToolController);
//	app.addControllerToFront(m_editionToolController);
	app.addControllerToFront(m_sampleController);
//	app.addControllerToFront(m_commonUIController);

	for (IApplicationController* c : app.getControllers())
	{
		(static_cast<ISampleController*>(c))->setManager(this);
	}

	m_fTool = new BlastFractureTool();
	m_fractureExecutor = nullptr;

	setFractureExecutor(&sVoronoiFracture);

	m_pCurBlastAsset = nullptr;
	m_nCurFamilyIndex = -1;
	EnableSimulating(false);
}

SampleManager::~SampleManager()
{
	delete m_renderer;
	delete m_physXController;
	delete m_blastController;
	delete m_sceneController;
	delete m_damageToolController;
	delete m_selectionToolController;
	delete m_explodeToolController;
	delete m_gizmoToolController;
	delete m_editionToolController;
	delete m_sampleController;
	delete m_fTool;
//	delete m_commonUIController;
}

int SampleManager::init()
{
	Application& app = *m_pApplication;
	app.init();

	m_damageToolController->DisableController();
	m_selectionToolController->EnableController();
	m_explodeToolController->DisableController();
	m_gizmoToolController->DisableController();
	BlastPlugin::Inst().GetMainToolbar()->updateCheckIconsStates();

	EnableSimulating(false);

	return 0;
}

int SampleManager::run()
{
	m_physXController->setPlaneVisible(AppMainWindow::Inst().m_bShowPlane);

	Application& app = *m_pApplication;
	app.run();

	std::vector<std::string>::iterator itStr;
	std::vector<Renderable*>::iterator itRenderable;
	std::map<std::string, RenderMaterial*>::iterator itRenderMaterial;
	for (itStr = m_NeedDeleteRenderMaterials.begin(); itStr != m_NeedDeleteRenderMaterials.end(); itStr++)
	{
		itRenderMaterial = m_RenderMaterialMap.find(*itStr);
		if (itRenderMaterial == m_RenderMaterialMap.end())
		{
			continue;
		}
		RenderMaterial* pRenderMaterial = itRenderMaterial->second;

		std::vector<Renderable*>& renderables = pRenderMaterial->getRelatedRenderables();
		for (itRenderable = renderables.begin(); itRenderable != renderables.end(); itRenderable++)
		{
			Renderable* pRenderable = *itRenderable;
			pRenderable->setMaterial(*RenderMaterial::getDefaultRenderMaterial());
		}

		delete pRenderMaterial;
		pRenderMaterial = nullptr;
		m_RenderMaterialMap.erase(itRenderMaterial);
	}
	m_NeedDeleteRenderMaterials.clear();

	return 0;
}

int SampleManager::free()
{
	std::map<BlastAsset*, std::vector<BlastFamily*>>::iterator it;
	for (it = m_AssetFamiliesMap.begin(); it != m_AssetFamiliesMap.end(); it++)
	{
		std::vector<BlastFamily*>& fs = it->second;
		fs.clear();
	}
	m_AssetFamiliesMap.clear();
	m_AssetDescMap.clear();

	Application& app = *m_pApplication;
	app.free();

	return 0;
}

bool SampleManager::createAsset(
	BlastAssetModelSimple** ppBlastAsset,
	std::vector<Nv::Blast::Mesh*>& meshes,
	std::vector<int32_t>& parentIds,
	std::vector<bool>& supports,
	std::vector<bool>& statics,
	std::vector<uint8_t>& joints,
	std::vector<uint32_t>& worlds)
{
	m_fTool->setSourceMeshes(meshes, parentIds);
	m_fTool->finalizeFracturing();

	_createAsset(ppBlastAsset, supports, statics, joints, worlds);

	return true;
}

bool SampleManager::saveAsset(BlastAsset* pBlastAsset)
{
	if (pBlastAsset == nullptr)
	{
		return false;
	}

	AssetList::ModelAsset& desc = m_AssetDescMap[pBlastAsset];
	
	PhysXController& pc = getPhysXController();
	BlastController& bc = getBlastController();
	physics = &pc.getPhysics();
	foundation = &physics->getFoundation();
	cooking = &pc.getCooking();
	physicsManager = &bc.getExtPxManager();

	std::string outDir = GlobalSettings::Inst().m_projectFileDir;

	std::string outBlastFilePath = GlobalSettings::MakeFileName(outDir.c_str(), std::string(desc.name + ".blast").c_str());
	const ExtPxAsset* asset = pBlastAsset->getPxAsset();
	if (asset == nullptr)
	{
		return false;
	}
	saveExtAsset(outDir, std::string(desc.name + ".blast"), asset);

	m_fTool->setSourceAsset(pBlastAsset);
	m_fTool->finalizeFracturing();

	size_t nChunkListSize = m_fTool->getChunkCount();
	std::vector<Triangle*> chunkMeshes(nChunkListSize);
	std::vector<uint32_t> chunkMeshesTriangleCount(nChunkListSize);
	std::shared_ptr<bool> isSupport(new bool[nChunkListSize] { false }, [](bool* b) {delete[] b; });
	for (uint32_t i = 0; i < nChunkListSize; ++i)
	{
		chunkMeshesTriangleCount[i] = m_fTool->getBaseMesh(i, chunkMeshes[i]);
		isSupport.get()[i] = m_fTool->getChunkInfo(i).isLeaf;
	}

	std::shared_ptr<Nv::Blast::BlastBondGenerator> bondGenerator(
		NvBlastExtAuthoringCreateBondGenerator(cooking, &physics->getPhysicsInsertionCallback()),
		[](Nv::Blast::BlastBondGenerator* bg) {bg->release(); });
	BondGenerationConfig cnf;
	cnf.bondMode = BondGenerationConfig::AVERAGE;
	NvBlastChunkDesc* chunkDesc;
	NvBlastBondDesc* bondDescs;
	const uint32_t bondCount = bondGenerator.get()->buildDescFromInternalFracture(m_fTool, isSupport.get(), bondDescs, chunkDesc);
	const uint32_t chunkCount = nChunkListSize;
	if (bondCount == 0)
	{
		std::cout << "Can't create bonds descriptors..." << std::endl;
	}

	std::vector<uint32_t> chunkReorderInvMap;
	{
		std::vector<uint32_t> chunkReorderMap(chunkCount);
		std::vector<char> scratch(chunkCount * sizeof(NvBlastChunkDesc));
		NvBlastEnsureAssetExactSupportCoverage(chunkDesc, chunkCount, scratch.data(), loggingCallback);
		NvBlastBuildAssetDescChunkReorderMap(chunkReorderMap.data(), chunkDesc, chunkCount, scratch.data(), loggingCallback);
		NvBlastApplyAssetDescChunkReorderMapInPlace(chunkDesc, chunkCount, bondDescs, bondCount, chunkReorderMap.data(), true, scratch.data(), loggingCallback);
		chunkReorderInvMap.resize(chunkReorderMap.size());
		Nv::Blast::invertMap(chunkReorderInvMap.data(), chunkReorderMap.data(), static_cast<unsigned int>(chunkReorderMap.size()));
	}

	std::vector<std::vector<Triangle>> resultGeometry(nChunkListSize);
	for (uint32_t i = 0; i < nChunkListSize; ++i)
	{
		uint32_t chunkIndex = chunkReorderInvMap[i];
		resultGeometry[chunkIndex].resize(chunkMeshesTriangleCount[i]);
		memcpy(resultGeometry[chunkIndex].data(), chunkMeshes[i], chunkMeshesTriangleCount[i] * sizeof(Triangle));
	}

	saveFractureToObj(resultGeometry, desc.name, outDir);

	char message[MAX_PATH];
	sprintf(message, "Blast file %s was saved.", outBlastFilePath.c_str());
	output(message);

	return true;
}

#include "fbxsdk.h"

uint32_t currentDepth;
bool bOutputFBXAscii = true;

void PxVec3ToFbx(physx::PxVec3& inVector, FbxVector4& outVector)
{
	outVector[0] = inVector.x;
	outVector[1] = inVector.y;
	outVector[2] = inVector.z;
	outVector[3] = 0;
}

void PxVec2ToFbx(physx::PxVec2& inVector, FbxVector2& outVector)
{
	outVector[0] = inVector.x;
	outVector[1] = inVector.y;
}

void VertexToFbx(Nv::Blast::Vertex& vert, FbxVector4& outVertex, FbxVector4& outNormal, FbxVector2& outUV)
{
	PxVec3ToFbx(vert.p, outVertex);
	PxVec3ToFbx(vert.n, outNormal);
	PxVec2ToFbx(vert.uv[0], outUV);
}

uint32_t createChunkRecursive(FbxManager* sdkManager, uint32_t currentCpIdx, uint32_t chunkIndex, FbxNode *meshNode, FbxNode* parentNode, FbxSkin* skin, const NvBlastAsset* asset, std::vector<std::vector<Nv::Blast::Triangle>> chunksGeometry)
{
	currentDepth++;

	auto chunks = NvBlastAssetGetChunks(asset, nullptr);
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
		skelAttrib = FbxSkeleton::Create(sdkManager, "SkelRootAttrib");
		skelAttrib->SetSkeletonType(FbxSkeleton::eRoot);

		// Change the centroid to origin
		centroid = physx::PxVec3(0.0f);
	}
	else
	{
		skelAttrib = FbxSkeleton::Create(sdkManager, boneName.c_str());
		skelAttrib->SetSkeletonType(FbxSkeleton::eLimbNode);
	}

	skelAttrib->Size.Set(1.0); // What's this for?


	FbxNode* boneNode = FbxNode::Create(sdkManager, boneName.c_str());
	boneNode->SetNodeAttribute(skelAttrib);

	auto mat = parentNode->EvaluateGlobalTransform().Inverse();

	FbxVector4 vec(centroid.x, centroid.y, centroid.z, 0);
	FbxVector4 c2 = mat.MultT(vec);

	boneNode->LclTranslation.Set(c2);

	parentNode->AddChild(boneNode);

	std::ostringstream().swap(namestream); // Swap namestream with a default constructed ostringstream
	namestream << "cluster_" << std::setw(5) << std::setfill('0') << chunkIndex;
	std::string clusterName = namestream.str();

	FbxCluster* cluster = FbxCluster::Create(sdkManager, clusterName.c_str());
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

		VertexToFbx(vert, vertex, normal, uv);

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
		if (tri.materialId == 0)
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
		addedCps += createChunkRecursive(sdkManager, currentCpIdx + addedCps, i, meshNode, boneNode, skin, asset, chunksGeometry);
	}

	return addedCps;
}

bool finalizeFbxAndSave(FbxManager* sdkManager, FbxScene* scene, FbxSkin* skin, const std::string& outputFilePath)
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

	assert(clusterNodes.size() > 0);

	FbxPose* pose = FbxPose::Create(sdkManager, "BasePose");
	pose->SetIsBindPose(true);

	for (auto node : clusterNodes)
	{
		FbxMatrix bindMat = node->EvaluateGlobalTransform();

		pose->Add(node, bindMat);
	}

	scene->AddPose(pose);

	FbxExporter* exporter = FbxExporter::Create(sdkManager, "Scene Exporter");

	int lFormat;

	if (bOutputFBXAscii)
	{
		lFormat = sdkManager->GetIOPluginRegistry()->FindWriterIDByDescription("FBX ascii (*.fbx)");
	}
	else
	{
		lFormat = sdkManager->GetIOPluginRegistry()->FindWriterIDByDescription("FBX binary (*.fbx)");
	}

	bool exportStatus = exporter->Initialize(outputFilePath.c_str(), lFormat, sdkManager->GetIOSettings());

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

bool SampleManager::exportAsset()
{
	if (m_pCurBlastAsset == nullptr)
	{
		viewer_err("Please select one asset instance before saving!");
		return false;
	}

	std::map<BlastAsset*, AssetList::ModelAsset>::iterator itADM = m_AssetDescMap.find(m_pCurBlastAsset);
	if (itADM == m_AssetDescMap.end())
	{
		viewer_err("Fails to find out the selected asset instance in current project!");
		return false;
	}

	BPParams& projectParams = BlastProject::ins().getParams();

	AssetList::ModelAsset& desc = itADM->second;

	BPPAssetArray& assetArray = projectParams.blast.blastAssets;
	BPPAsset asset;
	int aaas = 0;
	for (; aaas < assetArray.arraySizes[0]; aaas++)
	{
		asset = assetArray.buf[aaas];
		std::string assetname = asset.name;
		if (assetname == desc.name)
			break;
	}
	if (aaas == assetArray.arraySizes[0])
	{
		return false;
	}

	PhysXController& pc = getPhysXController();
	BlastController& bc = getBlastController();
	physics = &pc.getPhysics();
	foundation = &physics->getFoundation();
	cooking = &pc.getCooking();
	physicsManager = &bc.getExtPxManager();

	std::string outDir = GlobalSettings::Inst().m_projectFileDir;

	m_fTool->setSourceAsset(m_pCurBlastAsset);
	m_fTool->finalizeFracturing();

	size_t nChunkListSize = m_fTool->getChunkCount();
	std::vector<Triangle*> chunkMeshes(nChunkListSize);
	std::vector<uint32_t> chunkMeshesTriangleCount(nChunkListSize);
	std::shared_ptr<bool> isSupport(new bool[nChunkListSize] { false }, [](bool* b) {delete[] b; });
	for (uint32_t i = 0; i < nChunkListSize; ++i)
	{
		chunkMeshesTriangleCount[i] = m_fTool->getBaseMesh(i, chunkMeshes[i]);
		isSupport.get()[i] = m_fTool->getChunkInfo(i).isLeaf;
	}

	std::shared_ptr<Nv::Blast::BlastBondGenerator> bondGenerator(
		NvBlastExtAuthoringCreateBondGenerator(cooking, &physics->getPhysicsInsertionCallback()),
		[](Nv::Blast::BlastBondGenerator* bg) {bg->release(); });
	BondGenerationConfig cnf;
	cnf.bondMode = BondGenerationConfig::AVERAGE;
	NvBlastChunkDesc* chunkDesc;
	NvBlastBondDesc* bondDescs;
	const uint32_t bondCount = bondGenerator.get()->buildDescFromInternalFracture(m_fTool, isSupport.get(), bondDescs, chunkDesc);
	const uint32_t chunkCount = nChunkListSize;
	if (bondCount == 0)
	{
		std::cout << "Can't create bonds descriptors..." << std::endl;
	}

	std::vector<uint32_t> chunkReorderInvMap;
	{
		std::vector<uint32_t> chunkReorderMap(chunkCount);
		std::vector<char> scratch(chunkCount * sizeof(NvBlastChunkDesc));
		NvBlastEnsureAssetExactSupportCoverage(chunkDesc, chunkCount, scratch.data(), loggingCallback);
		NvBlastBuildAssetDescChunkReorderMap(chunkReorderMap.data(), chunkDesc, chunkCount, scratch.data(), loggingCallback);
		NvBlastApplyAssetDescChunkReorderMapInPlace(chunkDesc, chunkCount, bondDescs, bondCount, chunkReorderMap.data(), true, scratch.data(), loggingCallback);
		chunkReorderInvMap.resize(chunkReorderMap.size());
		Nv::Blast::invertMap(chunkReorderInvMap.data(), chunkReorderMap.data(), static_cast<unsigned int>(chunkReorderMap.size()));
	}

	std::vector<std::vector<Triangle>> resultGeometry(nChunkListSize);
	for (uint32_t i = 0; i < nChunkListSize; ++i)
	{
		uint32_t chunkIndex = chunkReorderInvMap[i];
		resultGeometry[chunkIndex].resize(chunkMeshesTriangleCount[i]);
		memcpy(resultGeometry[chunkIndex].data(), chunkMeshes[i], chunkMeshesTriangleCount[i] * sizeof(Triangle));
	}

	if (asset.exportFBX)
	{
		std::string outputFilePath = GlobalSettings::MakeFileName(outDir.c_str(), asset.fbx.buf);

		Nv::Blast::ConvexMeshBuilder* collisionBuilder = NvBlastExtAuthoringCreateConvexMeshBuilder(cooking, &physics->getPhysicsInsertionCallback());
		Nv::Blast::AuthoringResult* result = NvBlastExtAuthoringProcessFracture(*m_fTool, *bondGenerator, *collisionBuilder);

		if (!asset.embedFBXCollision)
		{
			result->releaseCollisionHulls();
		}

		std::shared_ptr<IMeshFileWriter> fileWriter(NvBlastExtExporterCreateFbxFileWriter(bOutputFBXAscii), [](IMeshFileWriter* p) {p->release(); });
		fileWriter->appendMesh(*result, asset.name.buf);
		if (!fileWriter->saveToFile(asset.fbx.buf, outDir.c_str()))
		{
			std::cerr << "Can't write geometry to FBX file." << std::endl;
			return false;
		}

#if (0)
		FbxManager* sdkManager = FbxManager::Create();

		FbxIOSettings* ios = FbxIOSettings::Create(sdkManager, IOSROOT);
		// Set some properties on the io settings
		
		sdkManager->SetIOSettings(ios);

		sdkManager->GetIOSettings()->SetBoolProp(EXP_ASCIIFBX, bOutputFBXAscii);

		FbxScene* scene = FbxScene::Create(sdkManager, "Export Scene");
		/*
		if (getConvertToUE4())
		{
			FbxAxisSystem::EFrontVector FrontVector = (FbxAxisSystem::EFrontVector) - FbxAxisSystem::eParityOdd;
			const FbxAxisSystem UnrealZUp(FbxAxisSystem::eZAxis, FrontVector, FbxAxisSystem::eRightHanded);

			scene->GetGlobalSettings().SetAxisSystem(UnrealZUp);
		}
		*/
		// Otherwise default to Maya defaults

		FbxMesh* mesh = FbxMesh::Create(sdkManager, "meshgeo");

		FbxGeometryElementNormal* geNormal = mesh->CreateElementNormal();
		geNormal->SetMappingMode(FbxGeometryElement::eByControlPoint);
		geNormal->SetReferenceMode(FbxGeometryElement::eDirect);

		FbxGeometryElementUV* geUV = mesh->CreateElementUV("diffuseElement");
		geUV->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
		geUV->SetReferenceMode(FbxGeometryElement::eDirect);

		// Get the triangles count for all of the mesh parts

		size_t triangleCount = 0;
		for (auto triangles : resultGeometry)
		{
			triangleCount += triangles.size();
		}

		mesh->InitControlPoints((int)triangleCount * 3);

		FbxNode* meshNode = FbxNode::Create(scene, "meshnode");
		meshNode->SetNodeAttribute(mesh);
		meshNode->SetShadingMode(FbxNode::eTextureShading);

		FbxNode* lRootNode = scene->GetRootNode();
		lRootNode->AddChild(meshNode);

		FbxSkin* skin = FbxSkin::Create(sdkManager, "Skin of the thing");
		skin->SetGeometry(mesh);

		mesh->AddDeformer(skin);

		// Add a material otherwise UE4 freaks out on import

		FbxGeometryElementMaterial* matElement = mesh->CreateElementMaterial();
		matElement->SetMappingMode(FbxGeometryElement::eByPolygon);
		matElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

		FbxSurfacePhong* material = FbxSurfacePhong::Create(sdkManager, "FirstExportMaterial");

		material->Diffuse.Set(FbxDouble3(1.0, 1.0, 0));
		material->DiffuseFactor.Set(1.0);

		meshNode->AddMaterial(material);

		FbxSurfacePhong* material2 = FbxSurfacePhong::Create(sdkManager, "SecondExportMaterial");

		material2->Diffuse.Set(FbxDouble3(1.0, 0.0, 1.0));
		material2->DiffuseFactor.Set(1.0);

		meshNode->AddMaterial(material2);

		const ExtPxAsset* pExtPxAsset = m_pCurBlastAsset->getPxAsset();
		if (pExtPxAsset == nullptr)
		{
			return false;
		}

		const TkAsset& tkAsset = pExtPxAsset->getTkAsset();
		const NvBlastAsset* pAssetLL = tkAsset.getAssetLL();
		uint32_t chunkCount = NvBlastAssetGetChunkCount(pAssetLL, nullptr);

		auto chunks = NvBlastAssetGetChunks(pAssetLL, nullptr);

		currentDepth = 0;
		uint32_t cpIdx = 0;
		for (uint32_t i = 0; i < chunkCount; i++)
		{
			const NvBlastChunk* chunk = &chunks[i];

			if (chunk->parentChunkIndex == UINT32_MAX)
			{
				uint32_t addedCps = createChunkRecursive(sdkManager, cpIdx, i, meshNode, lRootNode, skin, pAssetLL, resultGeometry);

				cpIdx += addedCps;
			}
		}

		std::string outputFilePath = GlobalSettings::MakeFileName(outDir.c_str(), asset.fbx.buf);
		finalizeFbxAndSave(sdkManager, scene, skin, outputFilePath);

		sdkManager->Destroy();
		sdkManager = nullptr;
#endif

		std::string info = outputFilePath + " is saved.";
		viewer_info(info.c_str());
	}

	if (asset.exportOBJ)
	{
		std::string filename = asset.obj.buf;
		filename = filename.substr(0, filename.find_last_of('.'));
		saveFractureToObj(resultGeometry, filename, outDir);

		std::string outputFilePath = GlobalSettings::MakeFileName(outDir.c_str(), asset.obj.buf);
		std::string info = outputFilePath + " is saved.";
		viewer_info(info.c_str());
	}

	if (asset.exportBPXA)
	{
		std::string outputFilePath = GlobalSettings::MakeFileName(outDir.c_str(), asset.bpxa.buf);
		const ExtPxAsset* pExtPxAsset = m_pCurBlastAsset->getPxAsset();
		if (pExtPxAsset == nullptr)
		{
			return false;
		}
		saveExtAsset(outDir, std::string(asset.bpxa.buf), pExtPxAsset);

		std::string info = outputFilePath + " is saved.";
		viewer_info(info.c_str());
	}

	if (asset.exportTKAsset)
	{
		std::string outputFilePath = GlobalSettings::MakeFileName(outDir.c_str(), asset.tkasset.buf);
		const ExtPxAsset* pExtPxAsset = m_pCurBlastAsset->getPxAsset();
		if (pExtPxAsset == nullptr)
		{
			return false;
		}

		const TkAsset& tkAsset = pExtPxAsset->getTkAsset();

		saveTkAsset(outDir, std::string(asset.tkasset.buf), &tkAsset);

		std::string info = outputFilePath + " is saved.";
		viewer_info(info.c_str());
	}

	if (asset.exportLLAsset)
	{
		std::string outputFilePath = GlobalSettings::MakeFileName(outDir.c_str(), asset.llasset.buf);
		const ExtPxAsset* pExtPxAsset = m_pCurBlastAsset->getPxAsset();
		if (pExtPxAsset == nullptr)
		{
			return false;
		}

		const TkAsset& tkAsset = pExtPxAsset->getTkAsset();
		const NvBlastAsset* pAssetLL = tkAsset.getAssetLL();

		saveLlAsset(outDir, std::string(asset.llasset.buf), pAssetLL);

		std::string info = outputFilePath + " is saved.";
		viewer_info(info.c_str());
	}

	return true;
}

void SampleManager::_createAsset(BlastAssetModelSimple** ppBlastAsset, 
	std::vector<bool>& supports,
	std::vector<bool>& statics,
	std::vector<uint8_t>& joints,
	std::vector<uint32_t>& worlds)
{
	PhysXController& pc = getPhysXController();
	BlastController& bc = getBlastController();

	physics = &pc.getPhysics();
	foundation = &physics->getFoundation();
	cooking = &pc.getCooking();
	physicsManager = &bc.getExtPxManager();
	TkFramework& tk = bc.getTkFramework();

	size_t nChunkListSize = m_fTool->getChunkCount();

	std::vector<Triangle*> chunkMeshes;
	std::vector<uint32_t> chunkMeshesTriangleCount(nChunkListSize);
	std::shared_ptr<bool> isSupport(new bool[nChunkListSize] { false }, [](bool* b) {delete[] b; });

	chunkMeshes.resize(nChunkListSize);
	for (uint32_t i = 0; i < nChunkListSize; ++i)
	{
		chunkMeshesTriangleCount[i] = m_fTool->getBaseMesh(i, chunkMeshes[i]);
		isSupport.get()[i] = supports.size() == 0 ? m_fTool->getChunkInfo(i).isLeaf : supports[i];
	}

	std::shared_ptr<Nv::Blast::BlastBondGenerator> bondGenerator(
		NvBlastExtAuthoringCreateBondGenerator(cooking, &physics->getPhysicsInsertionCallback()),
		[](Nv::Blast::BlastBondGenerator* bg) {bg->release(); });
	BondGenerationConfig cnf;
	cnf.bondMode = BondGenerationConfig::AVERAGE;
	NvBlastChunkDesc* chunkDesc;
	NvBlastBondDesc* bondDescs;
	const uint32_t bondCount = bondGenerator.get()->buildDescFromInternalFracture(m_fTool, isSupport.get(), bondDescs, chunkDesc);
	const uint32_t chunkCount = nChunkListSize;
	int bondDescsSize = bondCount;
	if (bondDescsSize == worlds.size())
	{		
		for (int bds = 0; bds < bondDescsSize; bds++)
		{
			if (worlds[bds] == 0xFFFFFFFF)
			{
				bondDescs[bds].chunkIndices[1] = worlds[bds];
			}
		}
	}

	if (bondCount == 0)
	{
		std::cout << "Can't create bonds descriptors..." << std::endl;
	}

	// order chunks, build map
	std::vector<uint32_t> chunkReorderInvMap;
	{
		std::vector<uint32_t> chunkReorderMap(chunkCount);
		std::vector<char> scratch(chunkCount * sizeof(NvBlastChunkDesc));
		NvBlastEnsureAssetExactSupportCoverage(chunkDesc, chunkCount, scratch.data(), loggingCallback);
		NvBlastBuildAssetDescChunkReorderMap(chunkReorderMap.data(), chunkDesc, chunkCount, scratch.data(), loggingCallback);
		NvBlastApplyAssetDescChunkReorderMapInPlace(chunkDesc, chunkCount, bondDescs, bondCount, chunkReorderMap.data(), true, scratch.data(), loggingCallback);
		chunkReorderInvMap.resize(chunkReorderMap.size());
		Nv::Blast::invertMap(chunkReorderInvMap.data(), chunkReorderMap.data(), static_cast<unsigned int>(chunkReorderMap.size()));
	}

	// get result geometry
	std::vector<std::vector<Triangle>> resultGeometry(nChunkListSize);
	for (uint32_t i = 0; i < nChunkListSize; ++i)
	{
		uint32_t chunkIndex = chunkReorderInvMap[i];
		resultGeometry[chunkIndex].resize(chunkMeshesTriangleCount[i]);
		memcpy(resultGeometry[chunkIndex].data(), chunkMeshes[i], chunkMeshesTriangleCount[i] * sizeof(Triangle));
	}

	// prepare physics data (convexes)
	std::vector<ExtPxAssetDesc::ChunkDesc> pxChunks(chunkCount);
	std::vector<ExtPxAssetDesc::SubchunkDesc> pxSubchunks;
	buildPxChunks(resultGeometry, pxChunks, pxSubchunks, statics);

	// build and serialize ExtPhysicsAsset
	ExtPxAssetDesc	descriptor;
	descriptor.bondCount = bondCount;
	descriptor.bondDescs = bondDescs;
	descriptor.chunkCount = chunkCount;
	descriptor.chunkDescs = chunkDesc;
	descriptor.bondFlags = joints.data();
	descriptor.pxChunks = pxChunks.data();
	ExtPxAsset* asset = ExtPxAsset::create(descriptor, tk);
	if (asset == nullptr)
	{
		return;
	}

	std::string tempFilePath = utils::GetTempFilePath();
	QFileInfo tempFileInfo(tempFilePath.c_str());
	std::string tempdir = QDir::toNativeSeparators(tempFileInfo.absoluteDir().absolutePath()).toLocal8Bit();
	std::string tempfile = tempFileInfo.fileName().toLocal8Bit();
	saveFractureToObj(resultGeometry, tempfile, tempdir);
	std::string objFilePath = tempFilePath + ".obj";
	std::string mtlFilePath = tempFilePath + ".mtl";
	BlastModel* pBlastModel = BlastModel::loadFromFileTinyLoader(objFilePath.c_str());
	DeleteFileA(tempFilePath.c_str());
	DeleteFileA(objFilePath.c_str());
	DeleteFileA(mtlFilePath.c_str());

	*ppBlastAsset = new BlastAssetModelSimple(asset, pBlastModel, getRenderer());
}

BlastAsset* SampleManager::_replaceAsset(BlastAsset* pBlastAsset,
	std::vector<bool>& supports,
	std::vector<bool>& statics,
	std::vector<uint8_t>& joints,
	std::vector<uint32_t>& worlds)
{
	if (pBlastAsset == nullptr)
	{
		return false;
	}

	BlastAsset* pCurBlastAsset = nullptr;
	int nFamilyIndex = -1;
	getCurrentSelectedInstance(&pCurBlastAsset, nFamilyIndex);

	std::vector<BlastFamily*> familiesOld = m_AssetFamiliesMap[pBlastAsset];
	int familiesSize = familiesOld.size();
	std::vector<physx::PxTransform> transforms(familiesSize);
	std::vector<std::string> extMaterials(familiesSize);
	std::vector<std::string> intMaterials(familiesSize);
	for (int fs = 0; fs < familiesSize; fs++)
	{
		transforms[fs] = familiesOld[fs]->getSettings().transform;

		setCurrentSelectedInstance(pBlastAsset, fs);
		getMaterialForCurrentFamily(extMaterials[fs], true);
		getMaterialForCurrentFamily(intMaterials[fs], false);
	}

	BlastAssetModelSimple* pBlastAssetNew;
	_createAsset(&pBlastAssetNew, supports, statics, joints, worlds);

	BlastAssetModelSimple* pBlastAssetOld = (BlastAssetModelSimple*)pBlastAsset;	
	AssetList::ModelAsset desc = m_AssetDescMap[pBlastAsset];
	removeBlastAsset(pBlastAssetOld);
	addBlastAsset(pBlastAssetNew, desc);

	for (int fs = 0; fs < familiesSize; fs++)
	{
		addBlastFamily(pBlastAssetNew, transforms[fs]);

		setCurrentSelectedInstance(pBlastAssetNew, fs);

		setMaterialForCurrentFamily(extMaterials[fs], true);
		setMaterialForCurrentFamily(intMaterials[fs], false);
	}

	if (pCurBlastAsset == pBlastAsset)
	{
		pCurBlastAsset = pBlastAssetNew;
	}
	setCurrentSelectedInstance(pCurBlastAsset, nFamilyIndex);

	return pBlastAssetNew;
}

std::vector<uint32_t> SampleManager::getCurrentSelectedChunks()
{
	std::vector<uint32_t> selectedChunks;
	std::vector<BlastFamilyPtr>& spFamilies = m_blastController->getFamilies();
	if (spFamilies.size() > 0)
	{
		return spFamilies.back()->getSelectedChunks();
	}
	return selectedChunks;
}

std::map<BlastAsset*, std::vector<uint32_t>> SampleManager::getSelectedChunks()
{
	std::map<BlastAsset*, std::vector<uint32_t>> selectedChunks;
	std::map<BlastAsset*, std::vector<BlastFamily*>>::iterator itrBlast = m_AssetFamiliesMap.begin();
	for (; itrBlast != m_AssetFamiliesMap.end(); ++itrBlast)
	{
		std::set<uint32_t> assetChunks;
		std::vector<BlastFamily*>& families = itrBlast->second;
		std::vector<BlastFamily*>::iterator itrFamily = families.begin();
		for (; itrFamily != families.end(); ++itrFamily)
		{
			std::vector<uint32_t> familyChunks = (*itrFamily)->getSelectedChunks();
			for (std::vector<uint32_t>::iterator itrChunk = familyChunks.begin(); itrChunk != familyChunks.end(); ++itrChunk)
			{
				assetChunks.insert(*itrChunk);
			}
		}

		if (assetChunks.size() == 0)
			continue;

		std::vector<uint32_t> vecAssetChunks;
		for (std::set<uint32_t>::iterator itrChunk = assetChunks.begin(); itrChunk != assetChunks.end(); ++itrChunk)
		{
			vecAssetChunks.push_back(*itrChunk);
		}

		selectedChunks.insert(std::make_pair(itrBlast->first, vecAssetChunks));
	}

	return selectedChunks;
}

void SampleManager::clearChunksSelected()
{
	std::map<BlastAsset*, std::vector<BlastFamily*>>::iterator itr = m_AssetFamiliesMap.begin();
	for (; itr != m_AssetFamiliesMap.end(); ++itr)
	{
		BlastAsset* pBlastAsset = itr->first;
		std::vector<BlastFamily*>& fs = itr->second;

		for (BlastFamily* f : fs)
		{
			f->clearChunksSelected();
		}
	}
}

void SampleManager::setChunkSelected(std::vector<uint32_t> depths, bool selected)
{
	std::map<BlastAsset*, std::vector<BlastFamily*>>::iterator itrBlast = m_AssetFamiliesMap.begin();
	for (; itrBlast != m_AssetFamiliesMap.end(); ++itrBlast)
	{
		std::vector<BlastFamily*>& families = itrBlast->second;
		std::vector<BlastFamily*>::iterator itrFamily = families.begin();
		for (; itrFamily != families.end(); ++itrFamily)
		{
			(*itrFamily)->setChunkSelected(depths, selected);
		}
	}
}

void SampleManager::setChunkVisible(std::vector<uint32_t> depths, bool bVisible)
{
	std::map<BlastAsset*, std::vector<BlastFamily*>>::iterator itrBlast = m_AssetFamiliesMap.begin();
	for (; itrBlast != m_AssetFamiliesMap.end(); ++itrBlast)
	{
		std::vector<BlastFamily*>& families = itrBlast->second;
		std::vector<BlastFamily*>::iterator itrFamily = families.begin();
		for (; itrFamily != families.end(); ++itrFamily)
		{
			(*itrFamily)->setChunkVisible(depths, bVisible);
		}
	}
}

void SampleManager::setFractureExecutor(FractureExecutor* executor)
{
	m_fractureExecutor = executor;
	if (executor)
	{
		executor->m_fractureTool = m_fTool;
		if (executor->m_randomGenerator == nullptr)
			executor->m_randomGenerator = &sRandomGenerator;
	}
}

void SampleManager::EnableStepforward(bool bStepforward)
{
	m_stepforward = bStepforward;
}

void SampleManager::EnableSimulating(bool bSimulating)
{
	m_simulating = bSimulating;
	m_stepforward = false;
	m_physXController->setPaused(!m_simulating);

	if (!m_simulating)
	{
		m_damageToolController->DisableController();
#if 0
		BlastSceneTree* pBlastSceneTree = BlastSceneTree::ins();
		if (pBlastSceneTree)
		{
			pBlastSceneTree->hideAllChunks();
			// make sure chunk0 shows.
			std::vector<uint32_t> depths(1, 0);
			pBlastSceneTree->setChunkVisible(depths, true);
			// refresh in scene tree and viewport
			//pBlastSceneTree->updateValues(false);
			SampleManager::ins()->m_bNeedRefreshTree = true;
		}
#endif
	}
}

#include <ViewerOutput.h>
void SampleManager::output(const char* str)
{
	viewer_msg("%s", str);
}

void SampleManager::output(float value)
{
	viewer_msg("%f", value);
}

void SampleManager::output(physx::PxVec3& vec)
{
	viewer_msg("%f,%f,%f", vec.x, vec.y, vec.z);
}

void SampleManager::clearScene()
{
	m_gizmoToolController->resetPos();
	/*
	BPPAssetArray& assets = BlastProject::ins().getParams().blast.blastAssets;
	int assetSize = assets.arraySizes[0];
	for (int as = 0; as < assetSize; as++)
	{
		BPPAsset& asset = assets.buf[as];
		BlastSceneTree::ins()->removeBlastInstances(asset);
		BlastSceneTree::ins()->removeBlastAsset(asset);
	}
	BlastSceneTree::ins()->clearProjectile();
	*/
	m_sceneController->ClearScene();

	EnableSimulating(false);

	std::map<std::string, RenderMaterial*>::iterator itRenderMaterial;
	for (itRenderMaterial = m_RenderMaterialMap.begin();
		itRenderMaterial != m_RenderMaterialMap.end(); itRenderMaterial++)
	{
		RenderMaterial* pRenderMaterial = itRenderMaterial->second;
		delete pRenderMaterial;
		pRenderMaterial = nullptr;
	}
	m_RenderMaterialMap.clear();

	std::map<BlastAsset*, std::vector<BlastFamily*>>::iterator itAssetFamilies;
	for (itAssetFamilies = m_AssetFamiliesMap.begin(); 
		itAssetFamilies != m_AssetFamiliesMap.end(); itAssetFamilies++)
	{
		std::vector<BlastFamily*>& fs = itAssetFamilies->second;
		fs.clear();
	}
	m_AssetFamiliesMap.clear();
	m_AssetDescMap.clear();
	m_instanceFamilyMap.clear();

	physx::PxVec3 zero(0.0f, 0.0f, 0.0f);
	m_assetExtents = zero;

	m_bNeedRefreshTree = true;

	m_pCurBlastAsset = nullptr;
	m_nCurFamilyIndex = -1;

	SimpleScene::Inst()->m_pCamera->SetDefaults();
}

void SampleManager::resetScene()
{
	std::map<BPPAssetInstance*, std::set<uint32_t>> selectChunks;

	if (m_selectionToolController->IsEnabled())
	{
		std::set<PxActor*> actors = m_selectionToolController->getTargetActors();
		for (PxActor* actor : actors)
		{
			BlastFamily* pBlastFamily = m_blastController->getFamilyByPxActor(*actor);
			if (pBlastFamily)
			{
				BPPAssetInstance* assetInstance = getInstanceByFamily(pBlastFamily);
				uint32_t chunkIndex = pBlastFamily->getChunkIndexByPxActor(*actor);
				selectChunks[assetInstance].insert(chunkIndex);
			}
		}
	}
	else if (m_gizmoToolController->IsEnabled())
	{
		PxActor* actor = m_gizmoToolController->getTargetActor();

		if (actor)
		{
			BlastFamily* pBlastFamily = m_blastController->getFamilyByPxActor(*actor);
			if (pBlastFamily)
			{
				BPPAssetInstance* assetInstance = getInstanceByFamily(pBlastFamily);
				uint32_t chunkIndex = pBlastFamily->getChunkIndexByPxActor(*actor);
				selectChunks[assetInstance].insert(chunkIndex);
			}
		}
	}

	m_selectionToolController->clearSelect();
	/*
	std::map<BPPAssetInstance*, BlastFamily*>::iterator itIFM;
	for (itIFM = m_instanceFamilyMap.begin(); itIFM != m_instanceFamilyMap.end(); itIFM++)
	{
		BPPAssetInstance* pInstance = itIFM->first;
		BlastSceneTree::ins()->removeBlastInstance(*pInstance);
	}
	BlastSceneTree::ins()->clearProjectile();
	*/
	getSceneController().ResetScene();
	EnableSimulating(false);
	/*
	for (itIFM = m_instanceFamilyMap.begin(); itIFM != m_instanceFamilyMap.end(); itIFM++)
	{
		BPPAssetInstance* pInstance = itIFM->first;
		BlastSceneTree::ins()->addBlastInstance(*pInstance);
	}
	*/
	std::set<PxActor*> actors;
	for (std::map<BPPAssetInstance*, std::set<uint32_t>>::iterator itr = selectChunks.begin(); itr != selectChunks.end(); ++itr)
	{
		BlastFamily* family = getFamilyByInstance(itr->first);
		std::set<uint32_t>& chunkIndexes = itr->second;

		if (nullptr != family)
		{
			for (uint32_t chunkIndex : chunkIndexes)
			{
				PxActor* actor = nullptr;
				family->getPxActorByChunkIndex(chunkIndex, &actor);

				if (actor)
					actors.insert(actor);
			}
		}
	}

	if (m_selectionToolController->IsEnabled())
	{
		m_selectionToolController->setTargetActors(actors);
	}
	else if (m_gizmoToolController->IsEnabled())
	{
		if (actors.size() > 0)
		 m_gizmoToolController->setTargetActor(*actors.begin());
	}

	// reset scene should not restore camera
	//SimpleScene::Inst()->m_pCamera->SetDefaults();
}

bool isChunkVisible(std::vector<BlastFamily*>& fs, uint32_t chunkIndex)
{
	int fsSize = fs.size();
	if (fsSize == 0)
	{
		return true;
	}

	bool visible = false;
	for (int i = 0; i < fsSize; i++)
	{
		if (fs[i]->isChunkVisible(chunkIndex))
		{
			visible = true;
			break;
		}
	}
	return visible;
}

void SampleManager::_setSourceAsset()
{
	std::vector<BlastFamilyPtr>& families = m_blastController->getFamilies();
	if (families.size() > 0)
	{
		BlastFamilyPtr spLastFamily = families.back();

		m_fTool->setSourceAsset(&(spLastFamily->getBlastAsset()));
	}
}

BlastFamily* SampleManager::getFamilyByInstance(BPPAssetInstance* instance)
{
	if (instance)
	{
		if (m_instanceFamilyMap.find(instance) != m_instanceFamilyMap.end())
		{
			return m_instanceFamilyMap[instance];
		}
	}
	return nullptr;
}

BPPAssetInstance* SampleManager::getInstanceByFamily(BlastFamily* family)
{
	if (family)
	{
		std::map<BPPAssetInstance*, BlastFamily*>::iterator itr = m_instanceFamilyMap.begin();
		for (; itr != m_instanceFamilyMap.end(); ++itr)
		{
			if (itr->second == family)
			{
				return itr->first;
			}
		}
	}
	return nullptr;
}

void SampleManager::updateFamily(BlastFamily* oldFamily, BlastFamily* newFamily)
{
	if (oldFamily)
	{
		BPPAssetInstance* instance = getInstanceByFamily(oldFamily);
		if (instance)
		{
			m_instanceFamilyMap[instance] = newFamily;
		}
	}
}

void SampleManager::removeRenderMaterial(std::string name)
{
	if (name.empty())
	{
		return;
	}

	std::map<std::string, RenderMaterial*>::iterator it = m_RenderMaterialMap.find(name);
	if (it != m_RenderMaterialMap.end())
	{
		m_NeedDeleteRenderMaterials.push_back(name);
	}
}

void SampleManager::renameRenderMaterial(std::string oldName, std::string newName)
{
	if (oldName.empty() || newName.empty())
	{
		return;
	}

	std::map<std::string, RenderMaterial*>::iterator it = m_RenderMaterialMap.find(oldName);
	if (it != m_RenderMaterialMap.end())
	{
		RenderMaterial* pRenderMaterial = it->second;
		m_RenderMaterialMap.erase(it);
		pRenderMaterial->setMaterialName(newName);
		m_RenderMaterialMap[newName] = pRenderMaterial;
	}
}

void SampleManager::reloadRenderMaterial(std::string name, float r, float g, float b, bool diffuse)
{
	if (name.empty())
	{
		return;
	}

	std::map<std::string, RenderMaterial*>::iterator it = m_RenderMaterialMap.find(name);
	if (it != m_RenderMaterialMap.end())
	{
		RenderMaterial* pRenderMaterial = it->second;
		if (diffuse)
		{
			pRenderMaterial->setDiffuseColor(r, g, b);
		}
		else
		{
			pRenderMaterial->setSpecularColor(r, g, b);
		}
	}
}

void SampleManager::reloadRenderMaterial(std::string name, std::string texture, RenderMaterial::TextureType tt)
{
	if (name.empty())
	{
		return;
	}

	std::map<std::string, RenderMaterial*>::iterator it = m_RenderMaterialMap.find(name);
	if (it != m_RenderMaterialMap.end())
	{
		RenderMaterial* pRenderMaterial = it->second;
		pRenderMaterial->setTextureFileName(texture, tt);
	}
}

void SampleManager::reloadRenderMaterial(std::string name, float specularShininess)
{
	if (name.empty())
	{
		return;
	}

	std::map<std::string, RenderMaterial*>::iterator it = m_RenderMaterialMap.find(name);
	if (it != m_RenderMaterialMap.end())
	{
		RenderMaterial* pRenderMaterial = it->second;
		pRenderMaterial->setSpecularShininess(specularShininess);
	}
}

RenderMaterial* SampleManager::getRenderMaterial(std::string name, bool create)
{
	if (name == "" || name == "None")
	{
		return RenderMaterial::getDefaultRenderMaterial();
	}

	std::map<std::string, RenderMaterial*>::iterator itRenderMaterial = m_RenderMaterialMap.find(name);
	RenderMaterial* pRenderMaterial = nullptr;
	if (itRenderMaterial != m_RenderMaterialMap.end())
	{
		pRenderMaterial = itRenderMaterial->second;
	}
	else if(create)
	{
		ResourceManager* pResourceManager = ResourceManager::ins();

		BPPGraphicsMaterial* pBPPGraphicsMaterial = BlastProject::ins().getGraphicsMaterial(name.c_str());
		
		if (pBPPGraphicsMaterial == nullptr)
		{
			return RenderMaterial::getDefaultRenderMaterial();
		}
		else if (pBPPGraphicsMaterial->diffuseTextureFilePath.buf != nullptr)
		{
			pRenderMaterial = new RenderMaterial(name.c_str(), *pResourceManager,
				"model_simple_textured_ex",
				pBPPGraphicsMaterial->diffuseTextureFilePath.buf);
			pRenderMaterial->setDiffuseColor(
				pBPPGraphicsMaterial->diffuseColor[0],
				pBPPGraphicsMaterial->diffuseColor[1],
				pBPPGraphicsMaterial->diffuseColor[2],
				pBPPGraphicsMaterial->diffuseColor[3]);
		}
		else
		{
			pRenderMaterial = new RenderMaterial(name.c_str(), *pResourceManager,
				"model_simple_textured_ex",
				pBPPGraphicsMaterial->diffuseColor[0],
				pBPPGraphicsMaterial->diffuseColor[1],
				pBPPGraphicsMaterial->diffuseColor[2],
				pBPPGraphicsMaterial->diffuseColor[3]);
		}

		m_RenderMaterialMap[name] = pRenderMaterial;
	}
	return pRenderMaterial;
}

void SampleManager::getCurrentSelectedInstance(BlastAsset** ppBlastAsset, int& index)
{
	*ppBlastAsset = m_pCurBlastAsset;
	index = m_nCurFamilyIndex;
}

void SampleManager::setCurrentSelectedInstance(BlastAsset* pBlastAsset, int index)
{
	m_pCurBlastAsset = pBlastAsset;
	m_nCurFamilyIndex = index;

	MaterialAssignmentsPanel::ins()->updateValues();
	FileReferencesPanel::ins()->updateValues();
}

void SampleManager::getMaterialForCurrentFamily(std::string& name, bool externalSurface)
{
	name = "";

	if (m_pCurBlastAsset == nullptr || m_nCurFamilyIndex < 0)
	{
		return;
	}

	std::map<BlastAsset*, std::vector<BlastFamily*>>::iterator it = m_AssetFamiliesMap.find(m_pCurBlastAsset);
	if (it == m_AssetFamiliesMap.end())
	{
		return;
	}

	std::vector<BlastFamily*>& fs = it->second;
	int fsSize = fs.size();
	if (fsSize == 0 || fsSize <= m_nCurFamilyIndex)
	{
		return;
	}

	BlastFamily* pBlastFamily = fs[m_nCurFamilyIndex];
	RenderMaterial* pRenderMaterial = nullptr;
	pBlastFamily->getMaterial(&pRenderMaterial, externalSurface);
	if (pRenderMaterial != nullptr)
	{
		name = pRenderMaterial->getMaterialName();
		if (name != "")
		{
			return;
		}
	}

	AssetList::ModelAsset modelAsset = m_AssetDescMap[m_pCurBlastAsset];
	int assetID = BlastProject::ins().getAssetIDByName(modelAsset.name.c_str());
	BPPAssetInstance* instance = BlastProject::ins().getAssetInstance(assetID, m_nCurFamilyIndex);
	if (externalSurface && instance->exMaterial.buf != nullptr)
	{
		name = instance->exMaterial.buf;
	}
	else if (!externalSurface && instance->inMaterial.buf != nullptr)
	{
		name = instance->inMaterial.buf;
	}
}

void SampleManager::setMaterialForCurrentFamily(std::string name, bool externalSurface)
{
	if (m_pCurBlastAsset == nullptr || m_nCurFamilyIndex < 0)
	{
		return;
	}

	std::map<BlastAsset*, std::vector<BlastFamily*>>::iterator it = m_AssetFamiliesMap.find(m_pCurBlastAsset);
	if (it == m_AssetFamiliesMap.end())
	{
		return;
	}

	std::vector<BlastFamily*>& fs = it->second;
	int fsSize = fs.size();
	if (fsSize == 0 || fsSize <= m_nCurFamilyIndex)
	{
		return;
	}

	RenderMaterial* pRenderMaterial = getRenderMaterial(name);

	BlastFamily* pBlastFamily = fs[m_nCurFamilyIndex];
	pBlastFamily->setMaterial(pRenderMaterial, externalSurface);

	AssetList::ModelAsset modelAsset = m_AssetDescMap[m_pCurBlastAsset];
	int assetID = BlastProject::ins().getAssetIDByName(modelAsset.name.c_str());
	BPPAssetInstance* instance = BlastProject::ins().getAssetInstance(assetID, m_nCurFamilyIndex);
	if (externalSurface)
	{
		copy(instance->exMaterial, name.c_str());
	}
	else
	{
		copy(instance->inMaterial, name.c_str());
	}
}

#if 0
void SampleManager::applyAssetToProjectParam(BlastAsset* pBlastAsset, bool addTo)
{
	BlastAssetModel* assetModel = dynamic_cast<BlastAssetModel*>(pBlastAsset);

	BPPBlast& blast = BlastProject::ins().getParams().blast;

	std::map<BlastAsset*, std::vector<BlastFamily*>>& AssetFamiliesMap = getAssetFamiliesMap();
	std::map<BlastAsset*, AssetList::ModelAsset>& AssetDescMap = getAssetDescMap();

	AssetList::ModelAsset desc = AssetDescMap[pBlastAsset];
	std::vector<BlastFamily*>& fs = AssetFamiliesMap[pBlastAsset];

	BlastController& blastController = getBlastController();
	SceneController& sceneController = getSceneController();

	char str[MAX_PATH];

	// asset array
	BPPAssetArray assetArray;
	assetArray.arraySizes[0] = 1;
	assetArray.buf = new BPPAsset[1];
	BPPAsset& asset = assetArray.buf[0];
	::init(asset);
	copy(asset.name, desc.name.c_str());
	if (addTo)
	{
		asset.ID = BlastProject::ins().generateNewAssetID();
		merge(blast.blastAssets, assetArray);
	}
	else
	{
		asset.ID = BlastProject::ins().getAssetIDByName(asset.name.buf);
		apart(blast.blastAssets, assetArray);
	}

	const ExtPxAsset* pExtPxAsset = pBlastAsset->getPxAsset();
	const ExtPxChunk* pExtPxChunk = pExtPxAsset->getChunks();

	const TkAsset& tkAsset = pExtPxAsset->getTkAsset();
	uint32_t chunkCount = tkAsset.getChunkCount();
	const NvBlastChunk* pNvBlastChunk = tkAsset.getChunks();
	uint32_t bondCount = tkAsset.getBondCount();
	const NvBlastBond* pNvBlastBond = tkAsset.getBonds();

	const NvBlastSupportGraph supportGraph = tkAsset.getGraph();
	uint32_t* chunkIndices = supportGraph.chunkIndices;
	uint32_t* adjacencyPartition = supportGraph.adjacencyPartition;
	uint32_t* adjacentNodeIndices = supportGraph.adjacentNodeIndices;
	uint32_t* adjacentBondIndices = supportGraph.adjacentBondIndices;

	std::vector<bool> isSupports(chunkCount);
	isSupports.assign(chunkCount, false);
	std::vector<uint32_t> fromIDs(bondCount);
	std::vector<uint32_t> toIDs(bondCount);
	fromIDs.assign(bondCount, -1);
	toIDs.assign(bondCount, -1);

	for (uint32_t node0 = 0; node0 < supportGraph.nodeCount; ++node0)
	{
		const uint32_t chunkIndex0 = supportGraph.chunkIndices[node0];
		if (chunkIndex0 >= chunkCount)
		{
			continue;
		}

		isSupports[chunkIndex0] = true;

		for (uint32_t adjacencyIndex = adjacencyPartition[node0]; adjacencyIndex < adjacencyPartition[node0 + 1]; adjacencyIndex++)
		{
			uint32_t node1 = supportGraph.adjacentNodeIndices[adjacencyIndex];

			// add this condition if you don't want to iterate all bonds twice
			if (node0 > node1)
				continue;

			const uint32_t chunkIndex1 = supportGraph.chunkIndices[node1];

			uint32_t bondIndex = supportGraph.adjacentBondIndices[adjacencyIndex];

			if (chunkIndex0 < chunkIndex1)
			{
				fromIDs[bondIndex] = chunkIndex0;
				toIDs[bondIndex] = chunkIndex1;
			}
			else
			{
				fromIDs[bondIndex] = chunkIndex1;
				toIDs[bondIndex] = chunkIndex0;
			}
		}
	}

	// chunks
	BPPChunkArray chunkArray;
	{
		chunkArray.buf = new BPPChunk[chunkCount];
		chunkArray.arraySizes[0] = chunkCount;
		char chunkname[10];
		for (int cc = 0; cc < chunkCount; ++cc)
		{
			BPPChunk& chunk = chunkArray.buf[cc];
			::init(chunk);

			std::vector<uint32_t> parentChunkIndexes;
			parentChunkIndexes.push_back(cc);
			uint32_t parentChunkIndex = cc;
			while ((parentChunkIndex = pNvBlastChunk[parentChunkIndex].parentChunkIndex) != -1)
			{
				parentChunkIndexes.push_back(parentChunkIndex);
			}

			std::string strChunkName = "Chunk";
			for (int pcIndex = parentChunkIndexes.size() - 1; pcIndex >= 0; pcIndex--)
			{
				sprintf(chunkname, "_%d", parentChunkIndexes[pcIndex]);
				strChunkName += chunkname;
			}
			copy(chunk.name, strChunkName.c_str());

			chunk.asset = asset.ID;
			chunk.ID = cc;
			chunk.parentID = pNvBlastChunk[cc].parentChunkIndex;
			chunk.staticFlag = pExtPxChunk[cc].isStatic;
			chunk.visible = isChunkVisible(fs, cc);
			chunk.support = isSupports[cc];

			if (assetModel != nullptr)
			{
				const BlastModel& model = assetModel->getModel();

				BPPGraphicsMesh& graphicsMesh = chunk.graphicsMesh;
				::init(graphicsMesh);

				const BlastModel::Chunk& chunk = model.chunks[cc];

				const std::vector<BlastModel::Chunk::Mesh>& meshes = chunk.meshes;
				int meshSize = meshes.size();

				if (meshSize == 0)
				{
					continue;
				}

				std::vector<physx::PxVec3> positions;
				std::vector<physx::PxVec3> normals;
				std::vector<physx::PxVec2> uv;
				std::vector<uint32_t>  ind;
				std::vector<int>  faceBreakPoint;
				std::vector<uint32_t>  materialIndexes;
				uint16_t curIndex = 0;
				for (int ms = 0; ms < meshSize; ms++)
				{
					const BlastModel::Chunk::Mesh& mesh = meshes[ms];
					materialIndexes.push_back(mesh.materialIndex);
					const SimpleMesh& simpleMesh = mesh.mesh;
					const std::vector<SimpleMesh::Vertex>& vertices = simpleMesh.vertices;
					const std::vector<uint16_t>& indices = simpleMesh.indices;

					int NumVertices = vertices.size();
					for (uint32_t i = 0; i < NumVertices; ++i)
					{
						positions.push_back(physx::PxVec3(vertices[i].position.x, vertices[i].position.y, vertices[i].position.z));
						normals.push_back(physx::PxVec3(vertices[i].normal.x, vertices[i].normal.y, vertices[i].normal.z));
						uv.push_back(physx::PxVec2(vertices[i].uv.x, vertices[i].uv.y));
					}
					int NumIndices = indices.size();
					for (uint32_t i = 0; i < NumIndices; ++i)
					{
						ind.push_back(indices[i] + curIndex);
					}
					curIndex += NumIndices;
					faceBreakPoint.push_back(NumIndices / 3);
				}

				graphicsMesh.materialAssignments.buf = new BPPMaterialAssignments[materialIndexes.size()];
				graphicsMesh.materialAssignments.arraySizes[0] = materialIndexes.size();
				for (size_t i = 0; i < materialIndexes.size(); ++i)
				{
					BPPMaterialAssignments& assignment = graphicsMesh.materialAssignments.buf[i];
					assignment.libraryMaterialID = materialIndexes[i];
					assignment.faceMaterialID = materialIndexes[i];
				}

				graphicsMesh.positions.buf = new nvidia::NvVec3[positions.size()];
				graphicsMesh.positions.arraySizes[0] = positions.size();
				for (size_t i = 0; i < positions.size(); ++i)
				{
					nvidia::NvVec3& item = graphicsMesh.positions.buf[i];
					item.x = positions[i].x;
					item.y = positions[i].y;
					item.z = positions[i].z;
				}

				graphicsMesh.normals.buf = new nvidia::NvVec3[normals.size()];
				graphicsMesh.normals.arraySizes[0] = normals.size();
				for (size_t i = 0; i < normals.size(); ++i)
				{
					nvidia::NvVec3& item = graphicsMesh.normals.buf[i];
					item.x = normals[i].x;
					item.y = normals[i].y;
					item.z = normals[i].z;
				}

				graphicsMesh.texcoords.buf = new nvidia::NvVec2[uv.size()];
				graphicsMesh.texcoords.arraySizes[0] = uv.size();
				for (size_t i = 0; i < uv.size(); ++i)
				{
					nvidia::NvVec2& item = graphicsMesh.texcoords.buf[i];
					item.x = uv[i].x;
					item.y = uv[i].y;
				}

				size_t indexCount = ind.size();
				size_t faceCount = ind.size() / 3;

				graphicsMesh.vertextCountInFace = 3;

				graphicsMesh.positionIndexes.buf = new int32_t[indexCount];
				graphicsMesh.positionIndexes.arraySizes[0] = indexCount;

				graphicsMesh.normalIndexes.buf = new int32_t[indexCount];
				graphicsMesh.normalIndexes.arraySizes[0] = indexCount;

				graphicsMesh.texcoordIndexes.buf = new int32_t[indexCount];
				graphicsMesh.texcoordIndexes.arraySizes[0] = indexCount;

				graphicsMesh.materialIDs.buf = new int32_t[faceCount];
				graphicsMesh.materialIDs.arraySizes[0] = faceCount;

				for (size_t i = 0; i < indexCount; ++i)
				{
					graphicsMesh.positionIndexes.buf[i] = ind[i];
					graphicsMesh.normalIndexes.buf[i] = ind[i];
					graphicsMesh.texcoordIndexes.buf[i] = ind[i];
					/*
					size_t j = 0;
					for (; j < faceBreakPoint.size(); ++j)
					{
						if (i < faceBreakPoint[j])
							break;
					}
					graphicsMesh.materialIDs.buf[i / 3] = j;
					*/
				}

				for (size_t f = 0; f < faceCount; f++)
				{
					int32_t ex = f < faceBreakPoint[0] ? 0 : 1;
					graphicsMesh.materialIDs.buf[f] = ex;
				}
			}
		}

		if (addTo)
		{
			merge(blast.chunks, chunkArray);
		}
		else
		{
			apart(blast.chunks, chunkArray);
		}
	}

	// bonds
	BPPBondArray bondArray;
	{
		bondArray.buf = new BPPBond[bondCount];
		bondArray.arraySizes[0] = bondCount;
		char bondname[10];
		bool visible;
		for (int bc = 0; bc < bondCount; ++bc)
		{
			BPPBond& bond = bondArray.buf[bc];
			bond.name.buf = nullptr;
			::init(bond);

			visible = isChunkVisible(fs, fromIDs[bc]) || isChunkVisible(fs, toIDs[bc]);
			bond.visible = visible;
			bond.fromChunk = fromIDs[bc];
			bond.toChunk = toIDs[bc];

			sprintf(bondname, "Bond_%d_%d", bond.fromChunk, bond.toChunk);
			copy(bond.name, bondname);
			bond.asset = asset.ID;

			bond.support.healthMask.buf = nullptr;
			bond.support.bondStrength = 1.0;
			bond.support.enableJoint = false;
		}

		if (addTo)
		{
			merge(blast.bonds, bondArray);
		}
		else
		{
			apart(blast.bonds, bondArray);
		}
	}

	freeBlast(bondArray);
	freeBlast(chunkArray);
	freeBlast(assetArray);

	m_bNeedRefreshTree = true;
}
#endif

void SampleManager::updateAssetFamilyStressSolver(BPPAsset* bppAsset, BPPStressSolver& stressSolver)
{
	if (nullptr == bppAsset || nullptr == bppAsset->name.buf || 0 == strlen(bppAsset->name.buf))
		return;

	BlastAsset* blastAsset = nullptr;
	std::map<BlastAsset*, AssetList::ModelAsset>::iterator itr = m_AssetDescMap.begin();
	for (; itr != m_AssetDescMap.end(); ++itr)
	{
		if (itr->second.name == bppAsset->name.buf)
		{
			blastAsset = itr->first;
			break;
		}
	}

	if (nullptr == blastAsset)
		return;

	std::vector<BlastFamily*>& families = m_AssetFamiliesMap[blastAsset];

	for (BlastFamily* family : families)
	{
		BlastFamily::Settings settings = family->getSettings();

		ExtStressSolverSettings & stressSolverSettings = settings.stressSolverSettings;
		stressSolverSettings.hardness = stressSolver.hardness;
		stressSolverSettings.stressLinearFactor = stressSolver.linearFactor;
		stressSolverSettings.stressAngularFactor = stressSolver.angularFactor;
		stressSolverSettings.bondIterationsPerFrame = stressSolver.bondIterationsPerFrame;
		stressSolverSettings.graphReductionLevel = stressSolver.graphReductionLevel;
		family->setSettings(settings);
	}
}

void SampleManager::updateModelMeshToProjectParam(BlastAsset* pBlastAsset)
{
	BlastProject& project = BlastProject::ins();
	BlastAssetModel* assetModel = dynamic_cast<BlastAssetModel*>(pBlastAsset);
	BPPBlast& blast = project.getParams().blast;
	std::map<BlastAsset*, AssetList::ModelAsset>& AssetDescMap = getAssetDescMap();
	AssetList::ModelAsset desc = AssetDescMap[pBlastAsset];

	int assetId = project.getAssetIDByName(desc.name.c_str());
	if (-1 == assetId)
		return;

	std::vector<BPPChunk*> chunks = project.getChildrenChunks(assetId);

	BPPChunk& chunk = *chunks[0];//unfracture model only has one chunk

	if (assetModel != nullptr)
	{
		const BlastModel& model = assetModel->getModel();

		BPPGraphicsMesh& graphicsMesh = chunk.graphicsMesh;

		const BlastModel::Chunk& chunk = model.chunks[0];//unfracture model only has one chunk

		const std::vector<BlastModel::Chunk::Mesh>& meshes = chunk.meshes;
		int meshSize = meshes.size();

		if (meshSize == 0)
		{
			return;
		}

		std::vector<physx::PxVec3> positions;
		for (int ms = 0; ms < meshSize; ms++)
		{
			const BlastModel::Chunk::Mesh& mesh = meshes[ms];
			const SimpleMesh& simpleMesh = mesh.mesh;
			const std::vector<SimpleMesh::Vertex>& vertices = simpleMesh.vertices;

			int NumVertices = vertices.size();
			for (uint32_t i = 0; i < NumVertices; ++i)
			{
				positions.push_back(physx::PxVec3(vertices[i].position.x, vertices[i].position.y, vertices[i].position.z));
			}
		}

		for (size_t i = 0; i < positions.size(); ++i)
		{
			nvidia::NvVec3& item = graphicsMesh.positions.buf[i];
			item.x = positions[i].x;
			item.y = positions[i].y;
			item.z = positions[i].z;
		}
	}
}

#if 0
void SampleManager::applyFamilyToProjectParam(BlastFamily* pBlastFamily, bool addTo)
{
	const BlastAsset& blastAsset = pBlastFamily->getBlastAsset();
	BlastAsset* pBlastAsset = (BlastAsset*)&blastAsset;

	std::vector<BlastFamily*>& fs = m_AssetFamiliesMap[pBlastAsset];
	std::map<BlastAsset*, AssetList::ModelAsset>& AssetDescMap = getAssetDescMap();
	AssetList::ModelAsset desc = AssetDescMap[pBlastAsset];

	const BlastFamily::Settings& familySetting = pBlastFamily->getSettings();
	physx::PxTransform transform = familySetting.transform;

	char str[MAX_PATH];

	BPPBlast& blast = BlastProject::ins().getParams().blast;
	int assetID = BlastProject::ins().getAssetIDByName(desc.name.c_str());

	// instance array
	BPPAssetInstanceArray instanceArray;
	instanceArray.arraySizes[0] = 1;
	instanceArray.buf = new BPPAssetInstance[1];
	BPPAssetInstance& instance = instanceArray.buf[0];
	if (addTo)
	{
		::init(instance);
		int instanceIndex = fs.size() - 1;
		sprintf(str, "%s_%d", desc.name.c_str(), instanceIndex);
		copy(instance.name, str);
		instance.asset = assetID;
		PxVec3 p = transform.p;
		PxQuat q = transform.q;
		instance.transform.position = nvidia::NvVec3(p.x, p.y, p.z);
		instance.transform.rotation = nvidia::NvVec4(q.x, q.y, q.z, q.w);
	}
	else
	{
		BPPAssetInstance* pInstance = getInstanceByFamily(pBlastFamily);
		copy(instance, *pInstance);
	}

	std::vector<BPPAssetInstance*> instances;
	BlastProject::ins().getAssetInstances(assetID, instances);
	int instanceSize = instances.size();
	std::vector<std::string> instanceNames(instanceSize);
	std::vector<BlastFamily*> instanceFamilys(instanceSize);
	for (int is = 0; is < instanceSize; is++)
	{
		instanceNames[is] = instances[is]->name;
		instanceFamilys[is] = m_instanceFamilyMap[instances[is]];
		m_instanceFamilyMap.erase(m_instanceFamilyMap.find(instances[is]));
	}

	if (addTo)
	{
		merge(blast.blastAssetInstances, instanceArray);
		instanceNames.push_back(str);
		instanceFamilys.push_back(pBlastFamily);

		std::vector<BPPChunk*> chunks = BlastProject::ins().getChildrenChunks(assetID);
		for (size_t i = 0; i < chunks.size(); ++i)
		{
			chunks[i]->visible = isChunkVisible(fs, i);
		}
	}
	else
	{
		apart(blast.blastAssetInstances, instanceArray);
	}

	instanceSize = instanceNames.size();
	for (int is = 0; is < instanceSize; is++)
	{
		BPPAssetInstance* curInstance = BlastProject::ins().getAssetInstance(assetID, instanceNames[is].c_str());
		if (curInstance != nullptr)
		{
			m_instanceFamilyMap[curInstance] = instanceFamilys[is];
		}
	}

	freeBlast(instanceArray);

	m_bNeedRefreshTree = true;
}
#endif

BlastAsset* SampleManager::loadBlastFile(std::string dir, std::string file, AssetList::ModelAsset modelAsset)
{
	GlobalSettings::Inst().m_projectFileDir = dir;
	GlobalSettings::Inst().m_projectFileName = file;

	TkFramework& framework = getBlastController().getTkFramework();
	PxPhysics& physics = getPhysXController().getPhysics();
	PxCooking& cooking = getPhysXController().getCooking();
	Renderer& renderer = getRenderer();
	ExtSerialization& serialization = *getBlastController().getExtSerialization();

	BlastAssetModelSimple* pBlastAssetModelSimple = new BlastAssetModelSimple(
		framework, physics, cooking, serialization, renderer, file.c_str());

	addBlastAsset(pBlastAssetModelSimple, modelAsset);

	return pBlastAssetModelSimple;
}

void SampleManager::addBlastAsset(BlastAssetModelSimple* pBlastAssetModelSimple, AssetList::ModelAsset modelAsset, bool inProject)
{
	// 1
	m_AssetDescMap[pBlastAssetModelSimple] = modelAsset;
	m_AssetFamiliesMap[pBlastAssetModelSimple].clear();
	pBlastAssetModelSimple->initialize();

	// 2
	m_sceneController->addBlastAsset(pBlastAssetModelSimple, modelAsset);

	if (!inProject)
	{
		// 3
		_addAssetToProjectParam(pBlastAssetModelSimple);
	}

	//4
	const BlastModel& model = pBlastAssetModelSimple->getModel();
	const physx::PxVec3 extent = (model.bbMax - model.bbMin) * 0.5f;
	atcore_float3 max = gfsdk_max(*(atcore_float3*)&m_assetExtents, *(atcore_float3*)&(extent));
	m_assetExtents = *(physx::PxVec3*)&max;

	getGizmoToolController().setAxisLength(m_assetExtents.magnitude());

	m_bNeedRefreshTree = true;
	/*
	BPPAsset* pBPPAsset = BlastProject::ins().getAsset(modelAsset.name.c_str());
	BlastSceneTree::ins()->addBlastAsset(*pBPPAsset);
	*/
}

void SampleManager::removeBlastAsset(BlastAssetModelSimple* pBlastAssetModelSimple)
{
	std::map<BlastAsset*, AssetList::ModelAsset>::iterator itADM = m_AssetDescMap.find(pBlastAssetModelSimple);
	/*
	AssetList::ModelAsset modelAsset = itADM->second;
	BPPAsset* pBPPAsset = BlastProject::ins().getAsset(modelAsset.name.c_str());
	BlastSceneTree::ins()->removeBlastInstances(*pBPPAsset);
	BlastSceneTree::ins()->removeBlastAsset(*pBPPAsset);
	*/
	// 3
	_removeInstancesFromProjectParam(pBlastAssetModelSimple);
	_removeAssetFromProjectParam(pBlastAssetModelSimple);

	// 2
	m_sceneController->removeBlastAsset(pBlastAssetModelSimple);

	_refreshInstanceFamilyMap();

	// 1
	m_AssetDescMap.erase(itADM);

	m_bNeedRefreshTree = true;
}

BlastFamily* SampleManager::addBlastFamily(BlastAsset* pBlastAsset, physx::PxTransform transform, bool inProject)
{
	if (pBlastAsset == nullptr)
	{
		return nullptr;
	}

	BlastFamily* pBlastFamily = m_sceneController->addBlastFamily(pBlastAsset, transform);

	if (!inProject)
	{
		_addInstanceToProjectParam(pBlastFamily);
	}

	_refreshInstanceFamilyMap();
	/*
	BPPAssetInstance* pBPPAssetInstance = getInstanceByFamily(pBlastFamily);
	BlastSceneTree::ins()->addBlastInstance(*pBPPAssetInstance);
	*/
	/*
	AssetList::ModelAsset modelAsset = m_AssetDescMap[pBlastAsset];
	int assetID = BlastProject::ins().getAssetIDByName(modelAsset.name.c_str());
	std::vector<BPPAssetInstance*> instances;
	BlastProject::ins().getAssetInstances(assetID, instances);
	std::vector<BlastFamily*> families = m_AssetFamiliesMap[pBlastAsset];
	int familiesSize = families.size();
	for (int fs = 0; fs < familiesSize; fs++)
	{
		m_instanceFamilyMap.insert(std::make_pair(instances[fs], families[fs]));
	}
	*/
	// should not quit here. we still need set up right bounding extent
	m_bNeedRefreshTree = true;
	return pBlastFamily;
}

bool SampleManager::removeBlastFamily(BlastAsset* pBlastAsset, int nFamilyIndex)
{
	if (pBlastAsset == nullptr)
	{
		return false;
	}

	std::map<BlastAsset*, std::vector<BlastFamily*>>::iterator itAFM = m_AssetFamiliesMap.find(pBlastAsset);
	if (itAFM == m_AssetFamiliesMap.end())
	{
		return false;
	}

	std::vector<BlastFamily*> families = itAFM->second;
	int familySize = families.size();
	if (familySize == 0 || familySize <= nFamilyIndex)
	{
		return false;
	}

	_removeInstanceFromProjectParam(families[nFamilyIndex]);
	/*
	BPPAssetInstance* pBPPAssetInstance = getInstanceByFamily(families[nFamilyIndex]);
	BlastSceneTree::ins()->addBlastInstance(*pBPPAssetInstance);
	*/
	m_sceneController->removeBlastFamily(pBlastAsset, nFamilyIndex);

	_refreshInstanceFamilyMap();
	m_bNeedRefreshTree = true;
	return true;
}

void SampleManager::refreshAsset(BlastAsset* pBlastAsset)
{
	m_fTool->setSourceAsset(pBlastAsset);
	m_fTool->finalizeFracturing();

	std::map<BlastAsset*, AssetList::ModelAsset>::iterator it = m_AssetDescMap.find(pBlastAsset);
	AssetList::ModelAsset desc = it->second;

	int nChunkCount = pBlastAsset->getPxAsset()->getChunkCount();
	int nBondCount = pBlastAsset->getPxAsset()->getTkAsset().getBondCount();

	std::vector<bool> supports(nChunkCount);
	std::vector<bool> statics(nChunkCount);

	int nCur = 0;

	BPPBlast& blast = BlastProject::ins().getParams().blast;
	int assetID = BlastProject::ins().getAssetIDByName(desc.name.c_str());
	int chunkSize = blast.chunks.arraySizes[0];
	for (int cs = 0; cs < chunkSize; cs++)
	{
		BPPChunk& chunk = blast.chunks.buf[cs];
		if (chunk.asset == assetID)
		{
			supports[nCur] = chunk.support;
			statics[nCur] = chunk.staticFlag;
			nCur++;
		}
	}

	if (nCur != nChunkCount)
	{
		assert("chunk size not right");
	}

	nCur = 0;

	std::vector<uint8_t> joints(nBondCount);
	std::vector<uint32_t> worlds(nBondCount);
	int bondSize = blast.bonds.arraySizes[0];
	for (int bs = 0; bs < bondSize; bs++)
	{
		BPPBond& bond = blast.bonds.buf[bs];
		if (bond.asset == assetID)
		{
			joints[nCur] = bond.support.enableJoint;
			worlds[nCur] = bond.toChunk;
			nCur++;
		}
	}

	if (nCur != nBondCount)
	{
		assert("bond size not right");
	}

	_replaceAsset(pBlastAsset, supports, statics, joints, worlds);
}

void SampleManager::UpdateCamera()
{
	m_renderer->UpdateCamera();
}

void SampleManager::_addAssetToProjectParam(BlastAsset* pBlastAsset)
{
	BlastAssetModel* assetModel = dynamic_cast<BlastAssetModel*>(pBlastAsset);

	BPPBlast& blast = BlastProject::ins().getParams().blast;

	std::map<BlastAsset*, std::vector<BlastFamily*>>& AssetFamiliesMap = getAssetFamiliesMap();
	std::map<BlastAsset*, AssetList::ModelAsset>& AssetDescMap = getAssetDescMap();

	AssetList::ModelAsset desc = AssetDescMap[pBlastAsset];
	std::vector<BlastFamily*>& fs = AssetFamiliesMap[pBlastAsset];

	BlastController& blastController = getBlastController();
	SceneController& sceneController = getSceneController();

	char str[MAX_PATH];

	// asset array
	BPPAssetArray assetArray;
	assetArray.arraySizes[0] = 1;
	assetArray.buf = new BPPAsset[1];
	BPPAsset& asset = assetArray.buf[0];
	::init(asset);
	copy(asset.name, desc.name.c_str());
	asset.ID = BlastProject::ins().generateNewAssetID();
	merge(blast.blastAssets, assetArray);

	const ExtPxAsset* pExtPxAsset = pBlastAsset->getPxAsset();
	const ExtPxChunk* pExtPxChunk = pExtPxAsset->getChunks();

	const TkAsset& tkAsset = pExtPxAsset->getTkAsset();
	uint32_t chunkCount = tkAsset.getChunkCount();
	const NvBlastChunk* pNvBlastChunk = tkAsset.getChunks();
	uint32_t bondCount = tkAsset.getBondCount();
	const NvBlastBond* pNvBlastBond = tkAsset.getBonds();

	const NvBlastSupportGraph supportGraph = tkAsset.getGraph();
	uint32_t* chunkIndices = supportGraph.chunkIndices;
	uint32_t* adjacencyPartition = supportGraph.adjacencyPartition;
	uint32_t* adjacentNodeIndices = supportGraph.adjacentNodeIndices;
	uint32_t* adjacentBondIndices = supportGraph.adjacentBondIndices;

	std::vector<bool> isSupports(chunkCount);
	isSupports.assign(chunkCount, false);
	std::vector<uint32_t> fromIDs(bondCount);
	std::vector<uint32_t> toIDs(bondCount);
	fromIDs.assign(bondCount, -1);
	toIDs.assign(bondCount, -1);

	for (uint32_t node0 = 0; node0 < supportGraph.nodeCount; ++node0)
	{
		const uint32_t chunkIndex0 = supportGraph.chunkIndices[node0];
		if (chunkIndex0 >= chunkCount)
		{
			continue;
		}

		isSupports[chunkIndex0] = true;

		for (uint32_t adjacencyIndex = adjacencyPartition[node0]; adjacencyIndex < adjacencyPartition[node0 + 1]; adjacencyIndex++)
		{
			uint32_t node1 = supportGraph.adjacentNodeIndices[adjacencyIndex];

			// add this condition if you don't want to iterate all bonds twice
			if (node0 > node1)
				continue;

			const uint32_t chunkIndex1 = supportGraph.chunkIndices[node1];

			uint32_t bondIndex = supportGraph.adjacentBondIndices[adjacencyIndex];

			if (chunkIndex0 < chunkIndex1)
			{
				fromIDs[bondIndex] = chunkIndex0;
				toIDs[bondIndex] = chunkIndex1;
			}
			else
			{
				fromIDs[bondIndex] = chunkIndex1;
				toIDs[bondIndex] = chunkIndex0;
			}
		}
	}

	// chunks
	BPPChunkArray chunkArray;
	{
		chunkArray.buf = new BPPChunk[chunkCount];
		chunkArray.arraySizes[0] = chunkCount;
		char chunkname[32];
		for (int cc = 0; cc < chunkCount; ++cc)
		{
			BPPChunk& chunk = chunkArray.buf[cc];
			::init(chunk);

			//std::vector<uint32_t> parentChunkIndexes;
			//parentChunkIndexes.push_back(cc);
			//uint32_t parentChunkIndex = cc;
			//while ((parentChunkIndex = pNvBlastChunk[parentChunkIndex].parentChunkIndex) != -1)
			//{
			//	parentChunkIndexes.push_back(parentChunkIndex);
			//}

			std::string strChunkName = "Chunk";
			sprintf(chunkname, "_%d", cc);
			strChunkName += chunkname;
			//for (int pcIndex = parentChunkIndexes.size() - 1; pcIndex >= 0; pcIndex--)
			//{
			//	sprintf(chunkname, "_%d", parentChunkIndexes[pcIndex]);
			//	strChunkName += chunkname;
			//}
			copy(chunk.name, strChunkName.c_str());

			chunk.asset = asset.ID;
			chunk.ID = cc;
			chunk.parentID = pNvBlastChunk[cc].parentChunkIndex;
			chunk.staticFlag = pExtPxChunk[cc].isStatic;
			chunk.visible = isChunkVisible(fs, cc);
			chunk.support = isSupports[cc];

			if (assetModel != nullptr)
			{
				const BlastModel& model = assetModel->getModel();

				BPPGraphicsMesh& graphicsMesh = chunk.graphicsMesh;
				::init(graphicsMesh);

				const BlastModel::Chunk& chunk = model.chunks[cc];

				const std::vector<BlastModel::Chunk::Mesh>& meshes = chunk.meshes;
				int meshSize = meshes.size();

				if (meshSize == 0)
				{
					continue;
				}

				std::vector<physx::PxVec3> positions;
				std::vector<physx::PxVec3> normals;
				std::vector<physx::PxVec3> tangents;
				std::vector<physx::PxVec2> uv;
				std::vector<uint32_t>  ind;
				std::vector<int>  faceBreakPoint;
				std::vector<uint32_t>  materialIndexes;
				uint16_t curIndex = 0;
				for (int ms = 0; ms < meshSize; ms++)
				{
					const BlastModel::Chunk::Mesh& mesh = meshes[ms];
					materialIndexes.push_back(mesh.materialIndex);
					const SimpleMesh& simpleMesh = mesh.mesh;
					const std::vector<SimpleMesh::Vertex>& vertices = simpleMesh.vertices;
					const std::vector<uint16_t>& indices = simpleMesh.indices;

					int NumVertices = vertices.size();
					for (uint32_t i = 0; i < NumVertices; ++i)
					{
						positions.push_back(physx::PxVec3(vertices[i].position.x, vertices[i].position.y, vertices[i].position.z));
						normals.push_back(physx::PxVec3(vertices[i].normal.x, vertices[i].normal.y, vertices[i].normal.z));
						tangents.push_back(physx::PxVec3(vertices[i].tangent.x, vertices[i].tangent.y, vertices[i].tangent.z));
						uv.push_back(physx::PxVec2(vertices[i].uv.x, vertices[i].uv.y));
					}
					int NumIndices = indices.size();
					for (uint32_t i = 0; i < NumIndices; ++i)
					{
						ind.push_back(indices[i] + curIndex);
					}
					curIndex += NumVertices;
					faceBreakPoint.push_back(NumIndices / 3);
				}

				graphicsMesh.materialAssignments.buf = new BPPMaterialAssignments[materialIndexes.size()];
				graphicsMesh.materialAssignments.arraySizes[0] = materialIndexes.size();
				for (size_t i = 0; i < materialIndexes.size(); ++i)
				{
					BPPMaterialAssignments& assignment = graphicsMesh.materialAssignments.buf[i];
					assignment.libraryMaterialID = materialIndexes[i];
					assignment.faceMaterialID = materialIndexes[i];
				}

				graphicsMesh.positions.buf = new nvidia::NvVec3[positions.size()];
				graphicsMesh.positions.arraySizes[0] = positions.size();
				for (size_t i = 0; i < positions.size(); ++i)
				{
					nvidia::NvVec3& item = graphicsMesh.positions.buf[i];
					item.x = positions[i].x;
					item.y = positions[i].y;
					item.z = positions[i].z;
				}

				graphicsMesh.normals.buf = new nvidia::NvVec3[normals.size()];
				graphicsMesh.normals.arraySizes[0] = normals.size();
				for (size_t i = 0; i < normals.size(); ++i)
				{
					nvidia::NvVec3& item = graphicsMesh.normals.buf[i];
					item.x = normals[i].x;
					item.y = normals[i].y;
					item.z = normals[i].z;
				}

				graphicsMesh.tangents.buf = new nvidia::NvVec3[tangents.size()];
				graphicsMesh.tangents.arraySizes[0] = tangents.size();
				for (size_t i = 0; i < tangents.size(); ++i)
				{
					nvidia::NvVec3& item = graphicsMesh.tangents.buf[i];
					item.x = tangents[i].x;
					item.y = tangents[i].y;
					item.z = tangents[i].z;
				}

				graphicsMesh.texcoords.buf = new nvidia::NvVec2[uv.size()];
				graphicsMesh.texcoords.arraySizes[0] = uv.size();
				for (size_t i = 0; i < uv.size(); ++i)
				{
					nvidia::NvVec2& item = graphicsMesh.texcoords.buf[i];
					item.x = uv[i].x;
					item.y = uv[i].y;
				}

				size_t indexCount = ind.size();
				size_t faceCount = ind.size() / 3;

				graphicsMesh.vertextCountInFace = 3;

				graphicsMesh.positionIndexes.buf = new int32_t[indexCount];
				graphicsMesh.positionIndexes.arraySizes[0] = indexCount;

				graphicsMesh.normalIndexes.buf = new int32_t[indexCount];
				graphicsMesh.normalIndexes.arraySizes[0] = indexCount;

				graphicsMesh.texcoordIndexes.buf = new int32_t[indexCount];
				graphicsMesh.texcoordIndexes.arraySizes[0] = indexCount;

				graphicsMesh.materialIDs.buf = new int32_t[faceCount];
				graphicsMesh.materialIDs.arraySizes[0] = faceCount;

				for (size_t i = 0; i < indexCount; ++i)
				{
					graphicsMesh.positionIndexes.buf[i] = ind[i];
					graphicsMesh.normalIndexes.buf[i] = ind[i];
					graphicsMesh.texcoordIndexes.buf[i] = ind[i];
				}

				for (size_t f = 0; f < faceCount; f++)
				{
					int32_t ex = f < faceBreakPoint[0] ? 0 : 1;
					graphicsMesh.materialIDs.buf[f] = ex;
				}
			}
		}

		merge(blast.chunks, chunkArray);
	}

	// bonds
	BPPBondArray bondArray;
	{
		bondArray.buf = new BPPBond[bondCount];
		bondArray.arraySizes[0] = bondCount;
		char bondname[64];
		bool visible;
		for (int bc = 0; bc < bondCount; ++bc)
		{
			BPPBond& bond = bondArray.buf[bc];
			bond.name.buf = nullptr;
			::init(bond);

			visible = isChunkVisible(fs, fromIDs[bc]) || isChunkVisible(fs, toIDs[bc]);
			bond.visible = visible;
			bond.fromChunk = fromIDs[bc];
			bond.toChunk = toIDs[bc];

			if (bond.toChunk == 0xFFFFFFFF)
			{
				sprintf(bondname, "Bond_%d_world", bond.fromChunk);
			}
			else
			{
				sprintf(bondname, "Bond_%d_%d", bond.fromChunk, bond.toChunk);
			}
			copy(bond.name, bondname);
			bond.asset = asset.ID;

			bond.support.healthMask.buf = nullptr;
			bond.support.bondStrength = 1.0;
			bond.support.enableJoint = false;
		}

		merge(blast.bonds, bondArray);
	}

	freeBlast(bondArray);
	freeBlast(chunkArray);
	freeBlast(assetArray);

	m_bNeedRefreshTree = true;
}

void SampleManager::_removeAssetFromProjectParam(BlastAsset* pBlastAsset)
{
	if (pBlastAsset == nullptr)
	{
		return;
	}

	std::map<BlastAsset*, AssetList::ModelAsset>::iterator itADM = m_AssetDescMap.find(pBlastAsset);
	if (itADM == m_AssetDescMap.end())
	{
		return;
	}

	AssetList::ModelAsset desc = m_AssetDescMap[pBlastAsset];

	BPPBlast& blast = BlastProject::ins().getParams().blast;
	
	int32_t assetId = BlastProject::ins().getAssetIDByName(desc.name.c_str());

	apart(blast.blastAssets, assetId);
	apart(blast.chunks, assetId);
	apart(blast.bonds, assetId);
}

void SampleManager::_addInstanceToProjectParam(BlastFamily* pBlastFamily)
{
	const BlastAsset& blastAsset = pBlastFamily->getBlastAsset();
	BlastAsset* pBlastAsset = (BlastAsset*)&blastAsset;

	std::vector<BlastFamily*>& fs = m_AssetFamiliesMap[pBlastAsset];
	AssetList::ModelAsset desc = m_AssetDescMap[pBlastAsset];

	const BlastFamily::Settings& familySetting = pBlastFamily->getSettings();
	physx::PxTransform transform = familySetting.transform;

	char str[MAX_PATH];

	BPPBlast& blast = BlastProject::ins().getParams().blast;
	int assetID = BlastProject::ins().getAssetIDByName(desc.name.c_str());

	// instance array
	BPPAssetInstanceArray instanceArray;
	instanceArray.arraySizes[0] = 1;
	instanceArray.buf = new BPPAssetInstance[1];
	BPPAssetInstance& instance = instanceArray.buf[0];
	::init(instance);
	int instanceIndex = fs.size() - 1;
	sprintf(str, "%s_%d", desc.name.c_str(), instanceIndex);
	copy(instance.name, str);
	instance.asset = assetID;
	PxVec3 p = transform.p;
	PxQuat q = transform.q;
	instance.transform.position = nvidia::NvVec3(p.x, p.y, p.z);
	instance.transform.rotation = nvidia::NvVec4(q.x, q.y, q.z, q.w);

	merge(blast.blastAssetInstances, instanceArray);

	std::vector<BPPChunk*> chunks = BlastProject::ins().getChildrenChunks(assetID);
	for (size_t i = 0; i < chunks.size(); ++i)
	{
		chunks[i]->visible = isChunkVisible(fs, i);
	}

	freeBlast(instanceArray);

	m_bNeedRefreshTree = true;
}

void SampleManager::_removeInstanceFromProjectParam(BlastFamily* pBlastFamily)
{
	BPPAssetInstance* pInstance = getInstanceByFamily(pBlastFamily);
	if (pInstance == nullptr)
	{
		return;
	}

	BPPBlast& blast = BlastProject::ins().getParams().blast;
	apart(blast.blastAssetInstances, pInstance->asset, pInstance->name.buf);
}

void SampleManager::_removeInstancesFromProjectParam(BlastAsset* pBlastAsset)
{
	if (pBlastAsset == nullptr)
	{
		return;
	}

	std::map<BlastAsset*, AssetList::ModelAsset>::iterator itADM = m_AssetDescMap.find(pBlastAsset);
	if (itADM == m_AssetDescMap.end())
	{
		return;
	}

	AssetList::ModelAsset desc = m_AssetDescMap[pBlastAsset];

	BPPBlast& blast = BlastProject::ins().getParams().blast;

	int32_t assetId = BlastProject::ins().getAssetIDByName(desc.name.c_str());

	apart(blast.blastAssetInstances, assetId);
}

void SampleManager::_refreshInstanceFamilyMap()
{
	m_instanceFamilyMap.clear();

	std::map<BlastAsset*, AssetList::ModelAsset>::iterator itADM;
	std::map<BlastAsset*, std::vector<BlastFamily*>>::iterator itAFM;
	for (itADM = m_AssetDescMap.begin(); itADM != m_AssetDescMap.end(); itADM++)
	{
		BlastAsset* pBlastAsset = itADM->first;
		itAFM = m_AssetFamiliesMap.find(pBlastAsset);
		if (itAFM == m_AssetFamiliesMap.end())
		{
			continue;
		}

		AssetList::ModelAsset modelAsset = itADM->second;
		int assetID = BlastProject::ins().getAssetIDByName(modelAsset.name.c_str());
		std::vector<BPPAssetInstance*> instances;
		BlastProject::ins().getAssetInstances(assetID, instances);
		int instancesSize = instances.size();
		std::vector<BlastFamily*> families = itAFM->second;
		int familiesSize = families.size();
		if (instancesSize != familiesSize)
		{
			assert("size of instance in scene and project not equal");
		}
		for (int fs = 0; fs < familiesSize; fs++)
		{
			m_instanceFamilyMap.insert(std::make_pair(instances[fs], families[fs]));
		}
	}
}

bool SampleManager::eventAlreadyHandled()
{
	bool isAlt = (GetAsyncKeyState(VK_MENU) && 0x8000);
	bool isLight = (GetAsyncKeyState('L') && 0x8000);
	return m_selectionToolController->IsEnabled() && !(isAlt || isLight);
}

void SampleManager::ApplyAutoSelectNewChunks(BlastAsset* pNewBlastAsset, std::vector<uint32_t>& NewChunkIndexes)
{
	std::map<BlastAsset*, std::vector<BlastFamily*>>::iterator itAFM = m_AssetFamiliesMap.find(pNewBlastAsset);
	if (itAFM == m_AssetFamiliesMap.end())
	{
		return;
	}

	bool autoSelectNewChunks = BlastProject::ins().getParams().fracture.general.autoSelectNewChunks;

	std::vector<BlastFamily*> families = itAFM->second;
	for (BlastFamily* pBlastFamily : families)
	{
		pBlastFamily->clearChunksSelected();

		if (!autoSelectNewChunks)
		{
			continue;
		}

		for (uint32_t chunkInd : NewChunkIndexes)
		{
			pBlastFamily->setChunkSelected(chunkInd, true);
			pBlastFamily->setChunkVisible(chunkInd, true);
		}
	}

	BlastSceneTree::ins()->ApplyAutoSelectNewChunks(pNewBlastAsset, NewChunkIndexes);
}

void SampleManager::ApplySelectionDepthTest()
{
	bool selectionDepthTest = BlastProject::ins().getParams().fracture.general.selectionDepthTest;

	std::vector<BlastFamily*>& families = m_blastController->getFamilies();

	for (BlastFamily* pBlastFamily : families)
	{
		std::vector<uint32_t> selectedChunks = pBlastFamily->getSelectedChunks();

		for (uint32_t chunkInd : selectedChunks)
		{
			pBlastFamily->setChunkSelected(chunkInd, true);
			pBlastFamily->setChunkVisible(chunkInd, true);
		}
	}
}