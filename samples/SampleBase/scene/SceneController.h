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
};

#endif