/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef SCENE_CONTROLLER_H
#define SCENE_CONTROLLER_H

#include "SampleManager.h"
#include <map>


class CFirstPersonCamera;
class BlastAssetBoxes;
class SceneActor;
class BlastAsset;
class SingleSceneAsset;
class Scene;
// Add By Lixu Begin
class PhysXSceneActor;
// Add By Lixu End
class SceneController : public ISampleController
{
public:

	SceneController();
	virtual ~SceneController();

	virtual LRESULT MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void Animate(double dt);
	void drawUI();
	void drawStatsUI();

	virtual void onInitialize();
	virtual void onSampleStart();
	virtual void onSampleStop();
	virtual void onTerminate();

	// commands
	int releaseAll();
	void spawnAsset(int32_t);

// Add By Lixu Begin
	void addProjectile();
	void clearProjectile();
	void ResetScene();
	void ClearScene();
	bool GetAssetDesc(const BlastAsset* asset, AssetList::ModelAsset& desc);
	void GetProjectilesNames(std::vector<std::string>& projectilesNames);
// Add By Lixu End

private:
	void addAssets(const AssetList& assetList, bool loadModels = true);
	void throwCube();

	SceneController& operator= (SceneController&);

	//////// used controllers ////////

	Renderer& getRenderer() const
	{
		return getManager()->getRenderer();
	}

	PhysXController& getPhysXController() const
	{
		return getManager()->getPhysXController();
	}

	BlastController& getBlastController() const
	{
		return getManager()->getBlastController();
	}

	CommonUIController& getCommonUIController() const
	{
		return getManager()->getCommonUIController();
	}


	//////// internal data ////////

	Scene* m_scene;

	float m_cubeScale;

// Add By Lixu Begin
	std::vector<std::string> m_UsedNames;
	std::vector<std::string> m_ReusedNames;
	std::vector<PhysXSceneActor*> m_Projectiles;
// Add By Lixu End
};

#endif