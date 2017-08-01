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
class BlastAssetModelSimple;
class ModelSceneAsset;
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
	std::vector<PhysXSceneActor*> getPrejectiles() { return m_Projectiles; }
	std::string getProjectileName(PhysXSceneActor* projectile);
	bool getProjectileVisible(PhysXSceneActor* projectile);
	void setProjectileVisible(PhysXSceneActor* projectile, bool val);
	void ResetScene();
	void ClearScene();
	void addBlastAsset(BlastAssetModelSimple* pBlastAsset, AssetList::ModelAsset modelAsset);
	void removeBlastAsset(BlastAssetModelSimple* pBlastAsset);
	BlastFamily* addBlastFamily(BlastAsset* pBlastAsset, physx::PxTransform transform);
	void removeBlastFamily(BlastAsset* pBlastAsset, int nFamilyIndex);
// Add By Lixu End

private:
	void addAssets(const AssetList& assetList, bool loadModels = true);
	void throwCube();
	float getCubeSpeed();

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
	float m_cubeThrowDownTime;

// Add By Lixu Begin
	std::vector<std::string> m_UsedNames;
	std::vector<std::string> m_ReusedNames;
	std::vector<PhysXSceneActor*> m_Projectiles;

	std::map<BlastAsset*, ModelSceneAsset*> BlastToSceneMap;
// Add By Lixu End
};

#endif