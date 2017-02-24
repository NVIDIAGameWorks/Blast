/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef SAMPLE_MANAGER_H
#define SAMPLE_MANAGER_H

#include "Application.h"
#include "Sample.h"
#include <map>

class SampleManager;
class BlastFractureTool;
class BlastAsset;
class BlastFamily;
class RenderMaterial;
namespace Nv
{
namespace Blast
{
	class Mesh;
	class RandomGeneratorBase;
	class VoronoiSitesGenerator;
	struct SlicingConfiguration;
}
}

class ISampleController : public IApplicationController
{
public:

	void setManager(SampleManager* manager)
	{
		m_manager = manager;
	}
protected:

	SampleManager* getManager() const
	{
		return m_manager;
	}

private:
	SampleManager* m_manager;
};

class FractureExecutor
{
	friend class SampleManager;
public:
	FractureExecutor()
		: m_fractureTool(0)
		, m_chunkId(-1)
		, m_randomGenerator(nullptr)
		, m_sourMesh(nullptr)
	{
	}

	virtual bool execute() = 0;
	void setSourceMesh(Nv::Blast::Mesh* mesh);
	void setSourceAsset(const BlastAsset* blastAsset);
	void setTargetChunk(uint32_t chunkId)	{ m_chunkId = chunkId; }
	void setRandomGenerator(Nv::Blast::RandomGeneratorBase* randomGenerator)	{ m_randomGenerator = randomGenerator; }
protected:
	BlastFractureTool*					m_fractureTool;
	uint32_t							m_chunkId;
	Nv::Blast::RandomGeneratorBase*		m_randomGenerator;
	Nv::Blast::Mesh*					m_sourMesh;
};

class VoronoiFractureExecutor : public FractureExecutor
{
public:
	VoronoiFractureExecutor();
	void setCellsCount(uint32_t cellsCount);

	virtual bool execute();

private:
	uint32_t	m_cellsCount;
};

class SliceFractureExecutor : public FractureExecutor
{
public:
	SliceFractureExecutor();
	void applyNoise(float amplitude, float frequency, int32_t octaves, float falloff, int32_t relaxIterations, float relaxFactor, int32_t seed = 0);
	void applyConfig(int32_t xSlices, int32_t ySlices, int32_t zSlices, float offsetVariations, float angleVariations);

	virtual bool execute();

private:
	Nv::Blast::SlicingConfiguration* m_config;
};

enum BlastToolType
{
	BTT_Damage = 0,
	BTT_Drag,
	BTT_Select,
	BTT_Translate,
	BTT_Scale,
	BTT_Rotation,
	BTT_Edit,
	BTT_Num
};

enum SelectMode
{
	SM_RESET = 0,
	SM_ADD,
	SM_SUB,
	SM_REMAIN,
};

class Renderer;
class PhysXController;
class BlastController;
class SceneController;
class DamageToolController;
class SelectionToolController;
class GizmoToolController;
class EditionToolController;
class SampleController;
class CommonUIController;
class SimpleRandomGenerator;

/**
*/
class SampleManager
{
	friend class VoronoiFractureExecutor;
	friend class SliceFractureExecutor;
  public:
	static SampleManager* ins();
	SampleManager(DeviceManager* pDeviceManager);
	~SampleManager();

	int init();
	int run();
	int free();

	void addModelAsset(std::string path, std::string file, bool isSkinned, physx::PxTransform transform, bool clear = true);

	bool createAsset(
		std::string path,
		std::string assetName,
		std::vector<physx::PxVec3>& positions,
		std::vector<physx::PxVec3>& normals,
		std::vector<physx::PxVec2>& uv,
		std::vector<unsigned int>&  indices,
		bool fracture = false);

	bool createAsset(
		const std::string& path,
		const std::string& assetName,
		const std::vector<Nv::Blast::Mesh* >& meshes,
		bool fracture = false);

	bool saveAsset();
	bool fractureAsset(std::string& path, std::string& assetName, const BlastAsset* blastAsset, int32_t chunkId);

	bool postProcessCurrentAsset();

	Renderer& getRenderer()
	{
		return *m_renderer;
	}

	PhysXController& getPhysXController() const
	{
		return *m_physXController;
	}

	BlastController& getBlastController() const
	{
		return *m_blastController;
	}

	SceneController& getSceneController() const
	{
		return *m_sceneController;
	}

	DamageToolController& getDamageToolController() const
	{
		return *m_damageToolController;
	}

	SelectionToolController& getSelectionToolController() const
	{
		return *m_selectionToolController;
	}

	GizmoToolController& getGizmoToolController() const
	{
		return *m_gizmoToolController;
	}

	EditionToolController& getEditionToolController() const
	{
		return *m_editionToolController;
	}

	SampleController& getSampleController() const
	{
		return *m_sampleController;
	}

	CommonUIController& getCommonUIController() const
	{
		return *m_commonUIController;
	}

	const SampleConfig&	getConfig() const
	{
		return m_config;
	}

	std::vector<uint32_t> getCurrentSelectedChunks();
	std::map<BlastAsset*, std::vector<uint32_t>> getSelectedChunks();
	void clearChunksSelected();
	void setChunkSelected(std::vector<uint32_t> depths, bool selected);
	void setChunkVisible(std::vector<uint32_t> depths, bool bVisible);

	void setFractureExecutor(FractureExecutor* executor);

	void setBlastToolType(BlastToolType type);

	void output(const char* str);
	void output(float value);
	void output(physx::PxVec3& vec);

	void clearScene();
	void resetScene();

	std::map<BlastAsset*, std::vector<BlastFamily*>>& getAssetFamiliesMap()
	{
		return m_AssetFamiliesMap;
	}
	std::map<BlastAsset*, AssetList::ModelAsset>& getAssetDescMap()
	{
		return m_AssetDescMap;
	}

	std::map<std::string, RenderMaterial*>& getRenderMaterials(){ return m_RenderMaterialMap; }
	void addRenderMaterial(RenderMaterial* pRenderMaterial);
	void removeRenderMaterial(std::string name);
	void deleteRenderMaterial(std::string name);
	void renameRenderMaterial(std::string oldName, std::string newName);
	
	bool m_bNeedRefreshTree;

	void getCurrentSelectedInstance(BlastAsset** ppBlastAsset, int& index);
	void setCurrentSelectedInstance(BlastAsset* pBlastAsset, int index);
	void getMaterialForCurrentFamily(RenderMaterial** ppRenderMaterial, bool externalSurface);
	void setMaterialForCurrentFamily(RenderMaterial* pRenderMaterial, bool externalSurface);

private:
	bool _createAsset(
		const std::string& assetName,
		const std::string& outDir,
		const std::vector<Nv::Blast::Mesh* >& meshes);

	void _setSourceAsset();

private:
	Renderer*             m_renderer;
	PhysXController*      m_physXController;
	BlastController*      m_blastController;
	SceneController*      m_sceneController;
	DamageToolController* m_damageToolController;
	SelectionToolController* m_selectionToolController;
	GizmoToolController*  m_gizmoToolController;
	EditionToolController* m_editionToolController;
	SampleController*     m_sampleController;
	CommonUIController*   m_commonUIController;

	SampleConfig	m_config;

	Application* m_pApplication;
	BlastToolType m_ToolType;

	BlastFractureTool*			m_fTool;
	FractureExecutor*			m_fractureExecutor;

	std::map<BlastAsset*, std::vector<BlastFamily*>> m_AssetFamiliesMap;
	std::map<BlastAsset*, AssetList::ModelAsset> m_AssetDescMap;
	std::map<std::string, RenderMaterial*> m_RenderMaterialMap;
	std::vector<std::string> m_NeedDeleteRenderMaterials;

	BlastAsset* m_pCurBlastAsset;
	int m_nCurFamilyIndex;

	bool m_bNeedConfig;
};


#endif