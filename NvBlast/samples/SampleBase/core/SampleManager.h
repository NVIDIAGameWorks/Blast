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