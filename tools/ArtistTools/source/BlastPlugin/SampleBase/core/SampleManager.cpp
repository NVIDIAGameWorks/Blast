/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/
#include "AppMainWindow.h"
#include "GlobalSettings.h"
#include <QtCore/QFileInfo>

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

using namespace physx;

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
	std::vector<ExtPxAssetDesc::SubchunkDesc>& pxSubchunks)
{
	ConvexMeshBuilder collisionBuilder(cooking, &physics->getPhysicsInsertionCallback());

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
		pxSubchunks[i].geometry = physx::PxConvexMeshGeometry(collisionBuilder.buildConvexMesh(vertices));
		pxChunks[i].isStatic = false;
		pxChunks[i].subchunkCount = 1;
		pxChunks[i].subchunks = &pxSubchunks[i];
	}

	// only effect when chunk is support
	pxChunks[0].isStatic = true;
}

void saveFractureToObj(std::vector<std::vector<Triangle> > chunksGeometry, std::string name, std::string path)
{
	MaterialAssignmentsPanel* pMaterialAssignmentsPanel = MaterialAssignmentsPanel::ins();
	std::vector<std::string> materialNames;
	std::vector<std::string> materialPaths;
	pMaterialAssignmentsPanel->getMaterialNameAndPaths(materialNames, materialPaths);

	uint32_t submeshCount = 2;
	// export materials (mtl file)
	{
		std::string mtlFilePath = GlobalSettings::MakeFileName(path.c_str(), std::string(name + ".mtl").c_str());
		FILE* f = fopen(mtlFilePath.c_str(), "w");
		if (!f)
			return;

		for (uint32_t submeshIndex = 0; submeshIndex < submeshCount; ++submeshIndex)
		{
			fprintf(f, "newmtl %s\n", materialNames[submeshIndex].c_str());
			fprintf(f, "\tmap_Kd %s\n", materialPaths[submeshIndex].c_str());
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
				if (chunksGeometry[vc][i].userInfo != 0)
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

void FractureExecutor::setSourceMesh(Nv::Blast::Mesh* mesh)
{
	assert(m_fractureTool);
	m_sourMesh = mesh;
	m_fractureTool->setSourceMesh(mesh);
}

void FractureExecutor::setSourceAsset(const BlastAsset* blastAsset)
{
	assert(m_fractureTool);
	m_fractureTool->setSourceAsset(blastAsset);
	m_sourMesh = nullptr;
}

VoronoiFractureExecutor::VoronoiFractureExecutor()
: m_cellsCount(5)
{
	if (sSampleManager)
		m_fractureTool = sSampleManager->m_fTool;
}

void VoronoiFractureExecutor::setCellsCount(uint32_t cellsCount)
{
	m_cellsCount = cellsCount;
}

bool VoronoiFractureExecutor::execute()
{
	Nv::Blast::Mesh* mesh = nullptr;
	if (m_sourMesh)
	{
		mesh = m_sourMesh;
	}
	else
	{
		mesh = m_fractureTool->getSourceMesh(m_chunkId);
	}
	// Prevent crash Junma Added By Lixu
	if (mesh == nullptr)
		return false;

	VoronoiSitesGenerator stGenerator(mesh, (m_randomGenerator == nullptr ? &sRandomGenerator : m_randomGenerator));
	stGenerator.uniformlyGenerateSitesInMesh(m_cellsCount);
	m_fractureTool->voronoiFracturing(m_chunkId, stGenerator.getVoronoiSites(), false);
	m_fractureTool->finalizeFracturing();

	return sSampleManager->postProcessCurrentAsset();
}

SliceFractureExecutor::SliceFractureExecutor()
: m_config(new Nv::Blast::SlicingConfiguration())
{
	if (sSampleManager)
		m_fractureTool = sSampleManager->m_fTool;
}

void SliceFractureExecutor::applyNoise(float amplitude, float frequency, int32_t octaves, float falloff, int32_t relaxIterations, float relaxFactor, int32_t seed)
{
	m_fractureTool->applyNoise(amplitude, frequency, octaves, falloff, relaxIterations, relaxFactor, seed);
}

void SliceFractureExecutor::applyConfig(int32_t xSlices, int32_t ySlices, int32_t zSlices, float offsetVariations, float angleVariations)
{
	m_config->x_slices = xSlices;
	m_config->y_slices = ySlices;
	m_config->z_slices = zSlices;
	m_config->offset_variations = offsetVariations;
	m_config->angle_variations = angleVariations;
}

bool SliceFractureExecutor::execute()
{
	m_fractureTool->slicing(m_chunkId, *m_config, false, (m_randomGenerator == nullptr ? &sRandomGenerator : m_randomGenerator));
	m_fractureTool->finalizeFracturing();
	return sSampleManager->postProcessCurrentAsset();
}

static VoronoiFractureExecutor sVoronoiFracture;

SampleManager* SampleManager::ins()
{
	return sSampleManager;
}

SampleManager::SampleManager(DeviceManager* pDeviceManager)
{	
	sSampleManager = this;
	m_bNeedConfig = false;
	m_bNeedRefreshTree = false;

	m_renderer = new Renderer();
	m_physXController = new PhysXController(ExtImpactDamageManager::FilterShader);
	m_blastController = new BlastController();
	m_sceneController = new SceneController();
	m_damageToolController = new DamageToolController();
	m_selectionToolController = new SelectionToolController();
	m_gizmoToolController = new GizmoToolController();
	m_editionToolController = new EditionToolController();
	m_sampleController = new SampleController();
//	m_commonUIController = new CommonUIController();

	m_pApplication = new Application(pDeviceManager);

	Application& app = *m_pApplication;

	app.addControllerToFront(m_renderer);
	app.addControllerToFront(m_physXController);
	app.addControllerToFront(m_blastController);
	app.addControllerToFront(m_sceneController);
	app.addControllerToFront(m_damageToolController);
	app.addControllerToFront(m_selectionToolController);
	app.addControllerToFront(m_gizmoToolController);
	app.addControllerToFront(m_editionToolController);
	app.addControllerToFront(m_sampleController);
//	app.addControllerToFront(m_commonUIController);

	for (IApplicationController* c : app.getControllers())
	{
		(static_cast<ISampleController*>(c))->setManager(this);
	}

	m_config.sampleName = L"";
	m_config.assetsFile = "";

	m_config.additionalResourcesDir.clear();
	m_config.additionalResourcesDir.push_back("../resources");
	m_config.additionalResourcesDir.push_back("../../../../bin/resources");

	m_config.additionalAssetList.models.clear();
	m_config.additionalAssetList.boxes.clear();
	m_config.additionalAssetList.composites.clear();

	m_fTool = new BlastFractureTool(loggingCallback);
	m_fractureExecutor = nullptr;

	setFractureExecutor(&sVoronoiFracture);

	m_pCurBlastAsset = nullptr;
	m_nCurFamilyIndex = -1;
}

SampleManager::~SampleManager()
{
	delete m_renderer;
	delete m_physXController;
	delete m_blastController;
	delete m_sceneController;
	delete m_damageToolController;
	delete m_selectionToolController;
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

	m_ToolType = BTT_Num;
	setBlastToolType(BTT_Edit);

	return 0;
}

int SampleManager::run()
{
	if (m_bNeedConfig)
	{
		getSceneController().onSampleStop();
		getSceneController().onSampleStart();

		_setSourceAsset();

		m_bNeedConfig = false;
		m_bNeedRefreshTree = true;
	}

	Application& app = *m_pApplication;
	app.run();

	std::vector<std::string>::iterator itStr;
	std::vector<Renderable*>::iterator itRenderable;
	for (itStr = m_NeedDeleteRenderMaterials.begin(); itStr != m_NeedDeleteRenderMaterials.end(); itStr++)
	{
		std::string materialName = *itStr;
		RenderMaterial* pRenderMaterial = m_RenderMaterialMap[materialName];
		std::vector<Renderable*>& renderables = pRenderMaterial->getRelatedRenderables();
		for (itRenderable = renderables.begin(); itRenderable != renderables.end(); itRenderable++)
		{
			Renderable* pRenderable = *itRenderable;
			pRenderable->setMaterial(*RenderMaterial::getDefaultRenderMaterial());
		}

		removeRenderMaterial(materialName);
	}
	m_NeedDeleteRenderMaterials.clear();

	MaterialLibraryPanel::ins()->deleteMaterials();

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

	/*
	std::vector<AssetList::ModelAsset>& modelAssets = m_config.additionalAssetList.models;
	std::vector<AssetList::ModelAsset>::iterator it;
	char filename[50];
	for (it = modelAssets.begin(); it != modelAssets.end(); it++)
	{
		AssetList::ModelAsset& m = *it;

		sprintf(filename, "../../../../bin/resources/models/%s.bpxa", m.id.c_str());
		DeleteFileA(filename);

		sprintf(filename, "../../../../bin/resources/models/%s.mtl", m.id.c_str());
		DeleteFileA(filename);

		sprintf(filename, "../../../../bin/resources/models/%s.obj", m.id.c_str());
		DeleteFileA(filename);
	}
	*/

	return 0;
}

void SampleManager::addModelAsset(std::string path, std::string file, bool isSkinned, physx::PxTransform transform, bool clear)
{
	if (clear)
	{
		//m_config.additionalAssetList.models.clear();
		//clearScene();
		AppMainWindow::Inst().menu_clearScene();
		GlobalSettings::Inst().m_projectFileDir = path;
		GlobalSettings::Inst().m_projectFileName = file;
		QFileInfo fileInfo(file.c_str());
		std::string ext = fileInfo.suffix().toUtf8().data();
		if (ext.length() < 1)
			GlobalSettings::Inst().m_projectFileName += ".blastProj";
	}
	else
	{
		std::vector<AssetList::ModelAsset>& modelAssets = m_config.additionalAssetList.models;
		std::vector<AssetList::ModelAsset>::iterator it = modelAssets.begin();
		for (; it != modelAssets.end(); it++)
		{
			AssetList::ModelAsset& m = *it;
			if (m.id == file)
			{
				modelAssets.erase(it);
				break;
			}
		}
	}

	AssetList::ModelAsset modelAsset;
	modelAsset.name = file;
	modelAsset.id = file;
	modelAsset.file = file;
	modelAsset.isSkinned = isSkinned;
	modelAsset.transform = transform;

	m_config.additionalAssetList.models.push_back(modelAsset);

	m_bNeedConfig = true;
}

bool SampleManager::createAsset(
	std::string path,
	std::string assetName,
	std::vector<physx::PxVec3>& positions,
	std::vector<physx::PxVec3>& normals,
	std::vector<physx::PxVec2>& uv,
	std::vector<unsigned int>&  indices,
	bool fracture)
{
	PhysXController& pc = getPhysXController();
	BlastController& bc = getBlastController();

	physics = &pc.getPhysics();
	foundation = &physics->getFoundation();
	cooking = &pc.getCooking();
	physicsManager = &bc.getExtPxManager();

	std::vector<Nv::Blast::Mesh* > meshes;
	PxVec3* nr = (!normals.empty()) ? normals.data() : 0;
	PxVec2* uvp = (!uv.empty()) ? uv.data() : 0;
	Nv::Blast::Mesh* sourceMesh = new Nv::Blast::Mesh(positions.data(), nr, uvp, static_cast<uint32_t>(positions.size()),
		indices.data(), static_cast<uint32_t>(indices.size()));
	meshes.push_back(sourceMesh);

	m_fractureExecutor->setSourceMesh(sourceMesh);
	if (fracture)
	{
		m_fractureExecutor->execute();
		m_fractureExecutor = &sVoronoiFracture;
	}
	else
	{
		m_fTool->finalizeFracturing();
	}

	std::string outDir = path;
	_createAsset(assetName, outDir, meshes);

	delete sourceMesh;
	sourceMesh = 0;

	m_bNeedConfig = true;

	return true;
}

bool SampleManager::createAsset(
	const std::string& path,
	const std::string& assetName,
	const std::vector<Nv::Blast::Mesh* >& meshes,
	bool fracture)
{
	PhysXController& pc = getPhysXController();
	BlastController& bc = getBlastController();

	physics = &pc.getPhysics();
	foundation = &physics->getFoundation();
	cooking = &pc.getCooking();
	physicsManager = &bc.getExtPxManager();

	if (meshes.size() == 1)
	{
		Nv::Blast::Mesh* sourceMesh = meshes[0];
		m_fractureExecutor->setSourceMesh(sourceMesh);
		if (fracture)
		{
			m_fractureExecutor->execute();
			m_fractureExecutor = &sVoronoiFracture;
		}
		else
		{
			m_fTool->finalizeFracturing();
		}
	}

	std::string outDir = path;
	_createAsset(assetName, outDir, meshes);

	m_bNeedConfig = true;

	return true;
}

bool SampleManager::saveAsset()
{
	if (m_pCurBlastAsset == nullptr)
	{
		return false;
	}

	AssetList::ModelAsset& desc = m_AssetDescMap[m_pCurBlastAsset];
	
	PhysXController& pc = getPhysXController();
	BlastController& bc = getBlastController();
	physics = &pc.getPhysics();
	foundation = &physics->getFoundation();
	cooking = &pc.getCooking();
	physicsManager = &bc.getExtPxManager();

	std::string outDir = GlobalSettings::Inst().m_projectFileDir;

	std::string outBlastFilePath = GlobalSettings::MakeFileName(outDir.c_str(), std::string(desc.name + ".bpxa").c_str());
	const ExtPxAsset* asset = m_pCurBlastAsset->getPxAsset();
	if (asset == nullptr)
	{
		return false;
	}
	physx::PsFileBuffer fileBuf(outBlastFilePath.c_str(), physx::PxFileBuf::OPEN_WRITE_ONLY);
	if (!asset->serialize(fileBuf, *cooking))
	{
		return false;
	}
	fileBuf.close();

	m_fTool->setSourceAsset(m_pCurBlastAsset);
	m_fTool->finalizeFracturing();

	size_t nChunkListSize = m_fTool->getChunkList().size();
	std::vector<std::vector<Triangle> > chunkMeshes(nChunkListSize);
	std::vector<bool> isSupport(nChunkListSize);
	for (uint32_t i = 0; i < nChunkListSize; ++i)
	{
		m_fTool->getBaseMesh(i, chunkMeshes[i]);
		isSupport[i] = m_fTool->getChunkList()[i].isLeaf;
	}

	BlastBondGenerator bondGenerator(cooking, &physics->getPhysicsInsertionCallback());
	BondGenerationConfig cnf;
	cnf.bondMode = BondGenerationConfig::AVERAGE;
	std::vector<NvBlastChunkDesc> chunkDesc;
	std::vector<NvBlastBondDesc> bondDescs;
	bondGenerator.buildDescFromInternalFracture(m_fTool, isSupport, bondDescs, chunkDesc);
	const uint32_t chunkCount = static_cast<uint32_t>(chunkDesc.size());
	const uint32_t bondCount = static_cast<uint32_t>(bondDescs.size());
	if (bondCount == 0)
	{
		std::cout << "Can't create bonds descriptors..." << std::endl;
	}

	std::vector<uint32_t> chunkReorderInvMap;
	{
		std::vector<uint32_t> chunkReorderMap(chunkCount);
		std::vector<char> scratch(chunkCount * sizeof(NvBlastChunkDesc));
		NvBlastEnsureAssetExactSupportCoverage(chunkDesc.data(), chunkCount, scratch.data(), loggingCallback);
		NvBlastBuildAssetDescChunkReorderMap(chunkReorderMap.data(), chunkDesc.data(), chunkCount, scratch.data(), loggingCallback);
		NvBlastApplyAssetDescChunkReorderMapInplace(chunkDesc.data(), chunkCount, bondDescs.data(), bondCount, chunkReorderMap.data(), scratch.data(), loggingCallback);
		chunkReorderInvMap.resize(chunkReorderMap.size());
		Nv::Blast::invertMap(chunkReorderInvMap.data(), chunkReorderMap.data(), static_cast<unsigned int>(chunkReorderMap.size()));
	}

	std::vector<std::vector<Triangle>> resultGeometry(chunkMeshes.size());
	for (uint32_t i = 0; i < chunkMeshes.size(); ++i)
	{
		uint32_t chunkIndex = chunkReorderInvMap[i];
		resultGeometry[chunkIndex] = chunkMeshes[i];
	}

	saveFractureToObj(resultGeometry, desc.name, outDir);

	std::string saveInfo = outBlastFilePath + " saved successfully\n";
	output(saveInfo.c_str());

	return true;
}

bool SampleManager::fractureAsset(std::string& path, std::string& assetName, const BlastAsset* pBlastAsset, int32_t chunkId)
{
	PhysXController& pc = getPhysXController();
	BlastController& bc = getBlastController();

	physics = &pc.getPhysics();
	foundation = &physics->getFoundation();
	cooking = &pc.getCooking();
	physicsManager = &bc.getExtPxManager();

	m_fractureExecutor->setSourceAsset(pBlastAsset);
	m_fractureExecutor->setTargetChunk(chunkId);
	m_fractureExecutor->execute();
	m_fractureExecutor = &sVoronoiFracture;

	std::string outDir = path;
	
	std::string outBlastFilePath = GlobalSettings::MakeFileName(outDir.c_str(), std::string(assetName + ".bpxa").c_str());

	std::vector<NvBlastChunkDesc> chunkDesc;
	std::vector<NvBlastBondDesc> bondDescs;
	std::vector<std::vector<Triangle> > chunkMeshes;
	std::vector<bool> isSupport;

	size_t nChunkListSize = m_fTool->getChunkList().size();
	chunkMeshes.resize(nChunkListSize);
	isSupport.resize(nChunkListSize);
	for (uint32_t i = 0; i < nChunkListSize; ++i)
	{
		m_fTool->getBaseMesh(i, chunkMeshes[i]);
		isSupport[i] = m_fTool->getChunkList()[i].isLeaf;
	}

	BlastBondGenerator bondGenerator(cooking, &physics->getPhysicsInsertionCallback());

	BondGenerationConfig cnf;
	cnf.bondMode = BondGenerationConfig::AVERAGE;
	bondGenerator.buildDescFromInternalFracture(m_fTool, isSupport, bondDescs, chunkDesc);

	const uint32_t chunkCount = static_cast<uint32_t>(chunkDesc.size());
	const uint32_t bondCount = static_cast<uint32_t>(bondDescs.size());
	if (bondCount == 0)
	{
		std::cout << "Can't create bonds descriptors..." << std::endl;
	}

	// order chunks, build map
	std::vector<uint32_t> chunkReorderInvMap;
	{
		std::vector<uint32_t> chunkReorderMap(chunkCount);
		std::vector<char> scratch(chunkCount * sizeof(NvBlastChunkDesc));
		NvBlastEnsureAssetExactSupportCoverage(chunkDesc.data(), chunkCount, scratch.data(), loggingCallback);
		NvBlastBuildAssetDescChunkReorderMap(chunkReorderMap.data(), chunkDesc.data(), chunkCount, scratch.data(), loggingCallback);
		NvBlastApplyAssetDescChunkReorderMapInplace(chunkDesc.data(), chunkCount, bondDescs.data(), bondCount, chunkReorderMap.data(), scratch.data(), loggingCallback);
		chunkReorderInvMap.resize(chunkReorderMap.size());
		Nv::Blast::invertMap(chunkReorderInvMap.data(), chunkReorderMap.data(), static_cast<unsigned int>(chunkReorderMap.size()));
	}

	// get result geometry
	std::vector<std::vector<Triangle>> resultGeometry(chunkMeshes.size());
	for (uint32_t i = 0; i < chunkMeshes.size(); ++i)
	{
		uint32_t chunkIndex = chunkReorderInvMap[i];
		resultGeometry[chunkIndex] = chunkMeshes[i];
	}

	// prepare physics data (convexes)
	std::vector<ExtPxAssetDesc::ChunkDesc> pxChunks(chunkCount);
	std::vector<ExtPxAssetDesc::SubchunkDesc> pxSubchunks;
	buildPxChunks(resultGeometry, pxChunks, pxSubchunks);

	// build and serialize ExtPhysicsAsset
	ExtPxAssetDesc	descriptor;
	descriptor.bondCount = bondCount;
	descriptor.bondDescs = bondDescs.data();
	descriptor.chunkCount = chunkCount;
	descriptor.chunkDescs = chunkDesc.data();
	descriptor.bondFlags = nullptr;
	descriptor.pxChunks = pxChunks.data();
	ExtPxAsset* asset = ExtPxAsset::create(descriptor, bc.getTkFramework());
	if (asset == nullptr)
	{
		return false;
	}

	physx::PsFileBuffer fileBuf(outBlastFilePath.c_str(), physx::PxFileBuf::OPEN_WRITE_ONLY);
	if (!asset->serialize(fileBuf, *cooking))
	{
		return false;
	}
	fileBuf.close();
	asset->release();

	saveFractureToObj(resultGeometry, assetName, outDir);

	m_bNeedConfig = true;

	return true;
}

bool SampleManager::postProcessCurrentAsset()
{
	std::vector<AssetList::ModelAsset>& models = m_config.additionalAssetList.models;
	if (models.size() < 0)
	{
		return true;
	}

	std::string assetName = models.at(models.size() - 1).file;
	std::string outDir = GlobalSettings::Inst().m_projectFileDir;
	std::vector<Nv::Blast::Mesh* > meshes;
	_createAsset(assetName, outDir, meshes);

	m_bNeedConfig = true;

	return true;
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

void SampleManager::setBlastToolType(BlastToolType type)
{
	if (m_ToolType == type)
	{
		getPhysXController().setPaused(type != BTT_Damage);
		return;
	}

	// refresh selection
	bool needClear = true;
	bool currentGizmo = (m_ToolType >= BTT_Translate && m_ToolType <= BTT_Rotation);
	bool switchToGizmo = (type >= BTT_Translate && type <= BTT_Rotation);
	if (currentGizmo && switchToGizmo)
	{
		needClear = false;
	}
	if (needClear)
	{
		getSelectionToolController().clearSelect();
		getGizmoToolController().resetPos();
	}

	getDamageToolController().getPickPointer()->setHidden(type != BTT_Damage);
	getGizmoToolController().showAxisRenderables(switchToGizmo);
	getPhysXController().setPaused(type != BTT_Damage);

	getDamageToolController().DisableController();
	getSelectionToolController().DisableController();
	getGizmoToolController().DisableController();
	getEditionToolController().DisableController();

	switch (type)
	{
		case BTT_Damage:
		{
			getDamageToolController().EnableController();
		}
			break;
		case BTT_Drag:
		{
			getDamageToolController().EnableController();
		}
			break;
		case BTT_Select:
		{
			getSelectionToolController().EnableController();
		}
			break;
		case BTT_Translate:
		{
			getGizmoToolController().EnableController();
			getGizmoToolController().setGizmoToolMode(GTM_Translate);
		}
			break;
		case BTT_Scale:
		{
			getGizmoToolController().EnableController();
			getGizmoToolController().setGizmoToolMode(GTM_Scale);
		}
			break;
		case BTT_Rotation:
		{
			getGizmoToolController().EnableController();
			getGizmoToolController().setGizmoToolMode(GTM_Rotation);
		}
			break;
		case BTT_Edit:
		{
			getEditionToolController().EnableController();
		}
			break;
		default:
			break;
	}

	m_ToolType = type;
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
	getSceneController().ClearScene();
	setBlastToolType(BTT_Edit);

	MaterialLibraryPanel::ins()->deleteMaterialMap();

	m_config.sampleName.clear();
	m_config.assetsFile.clear();
	m_config.additionalAssetList.models.clear();
	m_config.additionalAssetList.composites.clear();
	m_config.additionalAssetList.boxes.clear();

	std::map<BlastAsset*, std::vector<BlastFamily*>>::iterator it;
	for (it = m_AssetFamiliesMap.begin(); it != m_AssetFamiliesMap.end(); it++)
	{
		std::vector<BlastFamily*>& fs = it->second;
		fs.clear();
	}
	m_AssetFamiliesMap.clear();
	m_AssetDescMap.clear();

	m_bNeedRefreshTree = true;
}

void SampleManager::resetScene()
{
	getSceneController().ResetScene();
	setBlastToolType(BTT_Edit);
}

bool SampleManager::_createAsset(
	const std::string& assetName,
	const std::string& outDir,
	const std::vector<Nv::Blast::Mesh* >& meshes)
{
	PhysXController& pc = getPhysXController();
	BlastController& bc = getBlastController();

	physics = &pc.getPhysics();
	foundation = &physics->getFoundation();
	cooking = &pc.getCooking();
	physicsManager = &bc.getExtPxManager();
	TkFramework& tk = bc.getTkFramework();

	std::string outBlastFilePath = GlobalSettings::MakeFileName(outDir.c_str(), std::string(assetName + ".bpxa").c_str());

	std::vector<NvBlastChunkDesc> chunkDesc;
	std::vector<NvBlastBondDesc> bondDescs;
	std::vector<std::vector<Triangle> > chunkMeshes;
	std::vector<bool> isSupport;

	if (meshes.size() <= 1)
	{
		size_t nChunkListSize = m_fTool->getChunkList().size();
		chunkMeshes.resize(nChunkListSize);
		isSupport.resize(nChunkListSize);
		for (uint32_t i = 0; i < nChunkListSize; ++i)
		{
			m_fTool->getBaseMesh(i, chunkMeshes[i]);
			isSupport[i] = m_fTool->getChunkList()[i].isLeaf;
		}
	}
	// If there are more than one mesh, then it seems that it prefractured, lets consider first mesh in meshes as depth 0, other meshes as depth 1 chunks
	// we should just build Blast descriptors for such input.
	else
	{
		chunkMeshes.resize(meshes.size());
		std::vector<PxVec3> chunkCentroids(meshes.size(), PxVec3(0, 0, 0));

		for (uint32_t i = 0; i < meshes.size(); ++i)
		{
			std::vector<Triangle>& chunk = chunkMeshes[i];

			Vertex* vbf = meshes[i]->getVertices();
			Edge* ebf = meshes[i]->getEdges();

			for (uint32_t fc = 0; fc < meshes[i]->getFacetCount(); ++fc)
			{
				Facet* f = meshes[i]->getFacet(fc);
				Triangle tr;
				tr.a = vbf[ebf[f->firstEdgeNumber].s];
				tr.b = vbf[ebf[f->firstEdgeNumber + 1].s];
				tr.c = vbf[ebf[f->firstEdgeNumber + 2].s];
				chunk.push_back(tr);

				chunkCentroids[i] += tr.a.p + tr.b.p + tr.c.p;
			}

			chunkCentroids[i] *= 1.0f / (3 * meshes[i]->getFacetCount());
		}

		isSupport.resize(chunkMeshes.size());
		chunkDesc.resize(chunkMeshes.size());
		isSupport[0] = false;

		chunkDesc[0].centroid[0] = chunkCentroids[0].x;
		chunkDesc[0].centroid[1] = chunkCentroids[0].y;
		chunkDesc[0].centroid[2] = chunkCentroids[0].z;
		chunkDesc[0].parentChunkIndex = UINT32_MAX;

		for (uint32_t i = 1; i < chunkDesc.size(); ++i)
		{
			chunkDesc[i].parentChunkIndex = 0;
			chunkDesc[i].flags = NvBlastChunkDesc::SupportFlag;
			chunkDesc[i].centroid[0] = chunkCentroids[i].x;
			chunkDesc[i].centroid[1] = chunkCentroids[i].y;
			chunkDesc[i].centroid[2] = chunkCentroids[i].z;
			isSupport[i] = true;
		}
	}

	BlastBondGenerator bondGenerator(cooking, &physics->getPhysicsInsertionCallback());

	BondGenerationConfig cnf;
	cnf.bondMode = BondGenerationConfig::AVERAGE;

	if (meshes.size() > 1)
	{
		bondGenerator.bondsFromPrefractured(chunkMeshes, isSupport, bondDescs, cnf);
	}
	else
	{
		bondGenerator.buildDescFromInternalFracture(m_fTool, isSupport, bondDescs, chunkDesc);
	}

	const uint32_t chunkCount = static_cast<uint32_t>(chunkDesc.size());
	const uint32_t bondCount = static_cast<uint32_t>(bondDescs.size());
	if (bondCount == 0)
	{
		std::cout << "Can't create bonds descriptors..." << std::endl;
	}

	// order chunks, build map
	std::vector<uint32_t> chunkReorderInvMap;
	{
		std::vector<uint32_t> chunkReorderMap(chunkCount);
		std::vector<char> scratch(chunkCount * sizeof(NvBlastChunkDesc));
		NvBlastEnsureAssetExactSupportCoverage(chunkDesc.data(), chunkCount, scratch.data(), loggingCallback);
		NvBlastBuildAssetDescChunkReorderMap(chunkReorderMap.data(), chunkDesc.data(), chunkCount, scratch.data(), loggingCallback);
		NvBlastApplyAssetDescChunkReorderMapInplace(chunkDesc.data(), chunkCount, bondDescs.data(), bondCount, chunkReorderMap.data(), scratch.data(), loggingCallback);
		chunkReorderInvMap.resize(chunkReorderMap.size());
		Nv::Blast::invertMap(chunkReorderInvMap.data(), chunkReorderMap.data(), static_cast<unsigned int>(chunkReorderMap.size()));
	}

	// get result geometry
	std::vector<std::vector<Triangle>> resultGeometry(chunkMeshes.size());
	for (uint32_t i = 0; i < chunkMeshes.size(); ++i)
	{
		uint32_t chunkIndex = chunkReorderInvMap[i];
		resultGeometry[chunkIndex] = chunkMeshes[i];
	}

	// prepare physics data (convexes)
	std::vector<ExtPxAssetDesc::ChunkDesc> pxChunks(chunkCount);
	std::vector<ExtPxAssetDesc::SubchunkDesc> pxSubchunks;
	buildPxChunks(resultGeometry, pxChunks, pxSubchunks);

	// build and serialize ExtPhysicsAsset
	ExtPxAssetDesc	descriptor;
	descriptor.bondCount = bondCount;
	descriptor.bondDescs = bondDescs.data();
	descriptor.chunkCount = chunkCount;
	descriptor.chunkDescs = chunkDesc.data();
	descriptor.bondFlags = nullptr;
	descriptor.pxChunks = pxChunks.data();
	ExtPxAsset* asset = ExtPxAsset::create(descriptor, tk);
	if (asset == nullptr)
	{
		return false;
	}

	physx::PsFileBuffer fileBuf(outBlastFilePath.c_str(), physx::PxFileBuf::OPEN_WRITE_ONLY);
	if (!asset->serialize(fileBuf, *cooking))
	{
		return false;
	}
	fileBuf.close();
	asset->release();

	saveFractureToObj(resultGeometry, assetName, outDir);

	m_bNeedConfig = true;

	return true;
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

void SampleManager::addRenderMaterial(RenderMaterial* pRenderMaterial)
{
	if (pRenderMaterial == nullptr)
	{
		return;
	}

	std::string materialName = pRenderMaterial->getMaterialName();
	if (materialName.empty())
	{
		return;
	}

	m_RenderMaterialMap[materialName] = pRenderMaterial;

	std::string textureFileName = pRenderMaterial->getTextureFileName();
	MaterialLibraryPanel::ins()->addMaterial(materialName, textureFileName);
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
		m_RenderMaterialMap.erase(it);
		MaterialLibraryPanel::ins()->removeMaterial(name);
	}
}

void SampleManager::deleteRenderMaterial(std::string name)
{
	if (name.empty())
	{
		return;
	}

	m_NeedDeleteRenderMaterials.push_back(name);
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
}

void SampleManager::getMaterialForCurrentFamily(RenderMaterial** ppRenderMaterial, bool externalSurface)
{
	if (m_pCurBlastAsset == nullptr || m_nCurFamilyIndex < 0)
	{
		return;
	}

	std::vector<BlastFamily*>& fs = m_AssetFamiliesMap[m_pCurBlastAsset];
	int fsSize = fs.size();
	if (fsSize == 0 || fsSize < m_nCurFamilyIndex)
	{
		return;
	}

	BlastFamily* pBlastFamily = fs[m_nCurFamilyIndex];
	pBlastFamily->getMaterial(ppRenderMaterial, externalSurface);
}

void SampleManager::setMaterialForCurrentFamily(RenderMaterial* pRenderMaterial, bool externalSurface)
{
	if (m_pCurBlastAsset == nullptr || m_nCurFamilyIndex < 0)
	{
		return;
	}

	std::vector<BlastFamily*>& fs = m_AssetFamiliesMap[m_pCurBlastAsset];
	int fsSize = fs.size();
	if (fsSize == 0 || fsSize < m_nCurFamilyIndex)
	{
		return;
	}

	BlastFamily* pBlastFamily = fs[m_nCurFamilyIndex];
	pBlastFamily->setMaterial(pRenderMaterial, externalSurface);
}