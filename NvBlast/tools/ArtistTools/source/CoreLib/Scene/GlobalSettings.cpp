// This code contains NVIDIA Confidential Information and is disclosed 
// under the Mutual Non-Disclosure Agreement. 
// 
// Notice 
// ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES 
// NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO 
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT, 
// MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 
// 
// NVIDIA Corporation assumes no responsibility for the consequences of use of such 
// information or for any infringement of patents or other rights of third parties that may 
// result from its use. No license is granted by implication or otherwise under any patent 
// or patent rights of NVIDIA Corporation. No third party distribution is allowed unless 
// expressly authorized by NVIDIA.  Details are subject to change without notice. 
// This code supersedes and replaces all information previously supplied. 
// NVIDIA Corporation products are not authorized for use as critical 
// components in life support devices or systems without express written approval of 
// NVIDIA Corporation. 
// 
// Copyright (c) 2013 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//

#include "GlobalSettings.h"
#include <QtCore/QTimer>
#include "Settings.h"

static GlobalSettings g_settings;
static QTimer g_frameTimer(NV_NULL);

////////////////////////////////////////////////////////////////////////////////////////////
GlobalSettings::GlobalSettings()
{
	Init();
}

////////////////////////////////////////////////////////////////////////////////////////////
GlobalSettings& GlobalSettings::Inst()
{
	return g_settings;
}

////////////////////////////////////////////////////////////////////////////////////////////
QTimer& GlobalSettings::GetFrameTimer()
{
	return g_frameTimer;
}

////////////////////////////////////////////////////////////////////////////////////////////
void	
GlobalSettings::Init()
{
	m_animate = false;
	m_simulate = true;
	m_simulateStep = false;
	m_firstFrame = true;
	m_resetAnimationOnLoop = true;

	m_animationFps = 24.0f;
	m_repeatAnimation = true;
	m_frameTime = 0.0f;
	m_animationSpeed = 1.0f;
	m_playStopped = false;
	m_currentBoneIndex = 0;

	m_simulationFrameCalc.init(0, 1.0f / 60.0f);
	m_simulationFps = 60.0f;
	m_timeStep = 0.0f;

	m_visualizeWind = false;
	m_showGraphicsMesh = true;
	m_showWireframe	= false;
	m_showHUD = false;
	m_visualizeBoneNames = false;

	m_computeStatistics = false;
	m_computeProfile = false;
	m_previewLOD = false;
	m_renderStyle = MESH_RENDER_TEXTURED;

	m_sceneLoaded	= false;
	m_useDQ			= false;
	m_zup			= false;
	m_lhs			= false;

	m_sceneUnitIndex = SCENE_UNIT_CENTIMETER;  // default to use cm

	m_showGrid = true;
	m_showAxis = true;

	m_fovAngle = 75.0f;

	m_loadTextures	= true;
	m_loadMaterials	= true;
	m_loadGroom	= true;
	m_loadCollision	= true;
	m_loadConstraints	= true;

	m_msaaOption = 0;

	m_windDir.x = 0;
	m_windDir.y = -1.0f;
	m_windDir.z = 0.5f;
	m_windStrength = 0.0f;
	m_windNoise	= 0.0f;

	m_frameStartTime = 0.0f;
	m_frameEndTime = 100.0f;

	m_animationIndex = 0;
	
	m_lockRootBone = false;
	
	m_controlTextureOption = 0;
	m_showSkinnedMeshOnly = false;

	m_useLighting = true;
	m_showFPS = true;

	m_renderFrameCnt = 0;
	m_renderFps = 60.0f;

	m_projectFileDir = "";
	m_projectFileName = "";
	m_backgroundTextureFilePath = "";
}

///////////////////////////////////////////////////////////////////////////////
bool GlobalSettings::toggleSimulation()
{
	m_simulate = !m_simulate;
	m_simulateStep = false;

	return m_simulate;
} 
 
///////////////////////////////////////////////////////////////////////////////
void GlobalSettings::stepSimulation()
{
	m_simulateStep = true;
}

//////////////////////////////////////////////////////////////////////////////
void GlobalSettings::setRenderFps(float fps)
{
	m_renderFps = fps;

	OptionValue* option = nullptr;

	option = AppSettings::Inst().GetOptionValue("User/PerfMode");
	if (option)
	{
		if (option->Value.Bool == OA_TRUE)
		{
			g_frameTimer.setInterval(0);
		}
		else
			g_frameTimer.setInterval((int)(1000 / fps));
	}
}

//////////////////////////////////////////////////////////////////////////////
void GlobalSettings::setSimulationFps(float fps)
{
	m_simulationFps = fps;
	m_simulationFrameCalc.setTimeStep(1.0f/ fps);
}

//////////////////////////////////////////////////////////////////////////////
void GlobalSettings::resetAnimation()
{
	m_frameTime = m_frameStartTime;
	m_firstFrame = m_resetAnimationOnLoop ? true : false;
	m_playStopped = false;
	m_simulateStep = true;
}

///////////////////////////////////////////////////////////////////////////////
void GlobalSettings::toggleAnimationRepeat()
{
	m_repeatAnimation = !m_repeatAnimation;
} 

///////////////////////////////////////////////////////////////////////////////
bool GlobalSettings::toggleAnimation()
{
	m_animate = !m_animate;

	if (m_animate && m_playStopped)
		resetAnimation();

	return m_animate;
} 
 
///////////////////////////////////////////////////////////////////////////////
void GlobalSettings::stepAnimation()
{
	if (!m_sceneLoaded)
		return;

	if (m_repeatAnimation)
		m_playStopped = false;

	if (!m_playStopped)
		m_frameTime += m_animationSpeed * m_animationFps * m_timeStep; 

	if (m_frameTime > m_frameEndTime)
	{
		if (m_repeatAnimation)
			resetAnimation();
		else
		{
			m_frameTime = m_frameEndTime;
			m_playStopped = true;
			m_animate = false;
		}
	}

	if (!m_animate)
	{
		stepSimulation();
	}
}

///////////////////////////////////////////////////////////////////////////////
void GlobalSettings::setSceneUnitIndex(int i)
{
	if (i < 0)
		return;

	if (i >= SCENE_UNIT_END)
		return;

	m_sceneUnitIndex = i;
}


///////////////////////////////////////////////////////////////////////////////
float GlobalSettings::getSceneUnitInCentimeters()
{
	float unit = 1.0f;

	switch (m_sceneUnitIndex)
	{
	case SCENE_UNIT_CENTIMETER:
		unit = 1.0f;
		break;
	case SCENE_UNIT_METER:
		unit = 100.0f;
		break;
	case SCENE_UNIT_INCH:
		unit = 2.54f;
		break;
	case SCENE_UNIT_DECIMETER:
		unit = 10.0f;
		break;
	}

	return unit;
}

///////////////////////////////////////////////////////////////////////////////
atcore_float3 GlobalSettings::getGravityDir()
{
	atcore_float3 gravityDir;	

	if (m_zup)
	{
		gravityDir.x = 0.0f;
		gravityDir.y = 0.0f;
		gravityDir.z = -1.0f;
	}
	else
	{
		gravityDir.x = 0.0f;
		gravityDir.y = -1.0f;
		gravityDir.z = 0.0f;
	}

	return gravityDir;
}

///////////////////////////////////////////////////////////////////////////////
std::string	GlobalSettings::getAbsoluteFilePath()
{
	return m_projectFileDir + "\\" + m_projectFileName;
}

///////////////////////////////////////////////////////////////////////////////
std::string GlobalSettings::getAbsoluteFilePath(const char* fileName)
{
	return m_projectFileDir + "\\" + fileName;
}

#include <QtCore/QFileInfo>
#include <QtCore/QDir>

///////////////////////////////////////////////////////////////////////////////
std::string GlobalSettings::getRelativePath(const char* filePath)
{
	QDir projectDir(m_projectFileDir.c_str());
	QByteArray relPath = projectDir.relativeFilePath(filePath).toLocal8Bit();

	return (const char*)relPath;
}
