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


class SampleManager;

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


class Renderer;
class PhysXController;
class BlastController;
class SceneController;
class DamageToolController;
class SampleController;
class CommonUIController;


/**
*/
class SampleManager
{
  public:
	SampleManager(const SampleConfig& config);
	int run();

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


  private:
	  Renderer*             m_renderer;
	  PhysXController*      m_physXController;
	  BlastController*      m_blastController;
	  SceneController*      m_sceneController;
	  DamageToolController* m_damageToolController;
	  SampleController*     m_sampleController;
	  CommonUIController*   m_commonUIController;

	  const SampleConfig&	m_config;
};


#endif