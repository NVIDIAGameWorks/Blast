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


#ifndef SAMPLE_MANAGER_H
#define SAMPLE_MANAGER_H

#include "Application.h"
#include "Sample.h"
#include <map>
#include <functional>
#include "ProjectParams.h"
#include "RenderMaterial.h"
class SampleManager;
class BlastFractureTool;
class BlastAsset;
class BlastFamily;

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
		, m_randomGenerator(nullptr)
		, m_pCurBlastAsset(nullptr)
	{
		m_chunkIds.clear();
	}

	virtual bool execute() = 0;
	void setSourceAsset(BlastAsset* blastAsset);
	void setTargetChunk(uint32_t chunkId) 
	{ 
		m_chunkIds.clear();
		m_chunkIds.push_back(chunkId); 
	}
	void setTargetChunks(std::vector<uint32_t>& chunkIds) 
	{ 
		m_chunkIds.clear();
		std::vector<uint32_t>::iterator it;
		for (it = chunkIds.begin(); it != chunkIds.end(); it++)
		{
			m_chunkIds.push_back(*it);
		}
		std::sort(m_chunkIds.begin(), m_chunkIds.end(), std::greater<uint32_t>());
	}
	void setRandomGenerator(Nv::Blast::RandomGeneratorBase* randomGenerator)	{ m_randomGenerator = randomGenerator; }
protected:
	BlastFractureTool*					m_fractureTool;
	std::vector<uint32_t>				m_chunkIds;
	Nv::Blast::RandomGeneratorBase*		m_randomGenerator;
	BlastAsset*							m_pCurBlastAsset;
};

class VoronoiFractureExecutor : public FractureExecutor
{
public:
	VoronoiFractureExecutor();
	void setBPPVoronoi(BPPVoronoi* voronoi) { m_voronoi = voronoi; }

	virtual bool execute();

private:
	BPPVoronoi* m_voronoi;
};

class SliceFractureExecutor : public FractureExecutor
{
public:
	SliceFractureExecutor();
	void setBPPSlice(BPPSlice* slice) { m_slice = slice; }

	virtual bool execute();

private:
	BPPSlice* m_slice;
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
class ExplodeToolController;
class GizmoToolController;
class EditionToolController;
class SampleController;
class CommonUIController;
class SimpleRandomGenerator;
class BlastAssetModelSimple;
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

	bool createAsset(
		BlastAssetModelSimple** ppBlastAsset,
		std::vector<Nv::Blast::Mesh*>& meshes,
		std::vector<int32_t>& parentIds,
		std::vector<bool>& supports,
		std::vector<bool>& statics,
		std::vector<uint8_t>& joints,
		std::vector<uint32_t>& worlds);

	bool saveAsset(BlastAsset* pBlastAsset);

	bool exportAsset();

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

	ExplodeToolController& getExplodeToolController() const
	{
		return *m_explodeToolController;
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

	std::vector<uint32_t> getCurrentSelectedChunks();
	std::map<BlastAsset*, std::vector<uint32_t>> getSelectedChunks();
	void clearChunksSelected();
	void setChunkSelected(std::vector<uint32_t> depths, bool selected);
	void setChunkVisible(std::vector<uint32_t> depths, bool bVisible);

	void setFractureExecutor(FractureExecutor* executor);

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

	BlastFamily* getFamilyByInstance(BPPAssetInstance* instance);
	BPPAssetInstance* getInstanceByFamily(BlastFamily* family);
	void updateFamily(BlastFamily* oldFamily, BlastFamily* newFamily);

	std::map<std::string, RenderMaterial*>& getRenderMaterials(){ return m_RenderMaterialMap; }
	void removeRenderMaterial(std::string name);
	void renameRenderMaterial(std::string oldName, std::string newName);
	void reloadRenderMaterial(std::string name, float r, float g, float b, bool diffuse = true);
	void reloadRenderMaterial(std::string name, std::string texture, RenderMaterial::TextureType tt = RenderMaterial::TT_Diffuse);
	void reloadRenderMaterial(std::string name, float specularShininess);
	RenderMaterial* getRenderMaterial(std::string name, bool create = true);
	
	bool m_bNeedRefreshTree;

	void getCurrentSelectedInstance(BlastAsset** ppBlastAsset, int& index);
	void setCurrentSelectedInstance(BlastAsset* pBlastAsset, int index);
	void getMaterialForCurrentFamily(std::string& name, bool externalSurface);
	void setMaterialForCurrentFamily(std::string name, bool externalSurface);

	void updateAssetFamilyStressSolver(BPPAsset* bppAsset, BPPStressSolver& stressSolver);
	// only update unfractured mode mesh
	void updateModelMeshToProjectParam(BlastAsset* pBlastAsset);

	BlastAsset* loadBlastFile(std::string dir, std::string file, AssetList::ModelAsset modelAsset);
	void addBlastAsset(BlastAssetModelSimple* pBlastAsset, AssetList::ModelAsset modelAsset, bool inProject = false);
	void removeBlastAsset(BlastAssetModelSimple* pBlastAsset);
	BlastFamily* addBlastFamily(BlastAsset* pBlastAsset, physx::PxTransform transform, bool inProject = false);
	bool removeBlastFamily(BlastAsset* pBlastAsset, int nFamilyIndex);

	BlastAsset* getCurBlastAsset() { return m_pCurBlastAsset; }

	void refreshAsset(BlastAsset* pBlastAsset);

	void UpdateCamera();

	bool IsSimulating() { return m_simulating; }
	void EnableSimulating(bool bSimulating);
	bool IsStepforward() { return m_stepforward; }
	void EnableStepforward(bool bStepforward);

	physx::PxVec3 getAssetExtent() { return m_assetExtents; }

	bool eventAlreadyHandled();

	void ApplyAutoSelectNewChunks(BlastAsset* pNewBlastAsset, std::vector<uint32_t>& NewChunkIndexes);
	void ApplySelectionDepthTest();

private:
	void _createAsset(BlastAssetModelSimple** ppBlastAsset, 
		std::vector<bool>& supports,
		std::vector<bool>& statics,
		std::vector<uint8_t>& joints,
		std::vector<uint32_t>& worlds);

	BlastAsset* _replaceAsset(BlastAsset* pBlastAsset,
		std::vector<bool>& supports,
		std::vector<bool>& statics,
		std::vector<uint8_t>& joints,
		std::vector<uint32_t>& worlds);

	void _setSourceAsset();

	void _addAssetToProjectParam(BlastAsset* pBlastAsset);
	void _removeAssetFromProjectParam(BlastAsset* pBlastAsset);
	void _addInstanceToProjectParam(BlastFamily* pBlastFamily);
	void _removeInstanceFromProjectParam(BlastFamily* pBlastFamily);
	void _removeInstancesFromProjectParam(BlastAsset* pBlastAsset);
	void _refreshInstanceFamilyMap();

private:
	Renderer*             m_renderer;
	PhysXController*      m_physXController;
	BlastController*      m_blastController;
	SceneController*      m_sceneController;
	DamageToolController* m_damageToolController;
	SelectionToolController* m_selectionToolController;
	ExplodeToolController*   m_explodeToolController;
	GizmoToolController*  m_gizmoToolController;
	EditionToolController* m_editionToolController;
	SampleController*     m_sampleController;
	CommonUIController*   m_commonUIController;
	
	Application* m_pApplication;

	BlastFractureTool*			m_fTool;
	FractureExecutor*			m_fractureExecutor;

	std::map<BlastAsset*, std::vector<BlastFamily*>> m_AssetFamiliesMap;
	std::map<BlastAsset*, AssetList::ModelAsset> m_AssetDescMap;
	std::map<std::string, RenderMaterial*> m_RenderMaterialMap;
	std::vector<std::string> m_NeedDeleteRenderMaterials;
	std::map<BPPAssetInstance*, BlastFamily*> m_instanceFamilyMap;

	BlastAsset* m_pCurBlastAsset;
	int m_nCurFamilyIndex;
	physx::PxVec3 m_assetExtents;
	bool m_simulating;
	bool m_stepforward;
};


#endif