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

#pragma once

#include "MathUtil.h"
#include <string>

#include <Nv/Common/Animation/NvCoFrameCalculator.h>

class QTimer;

enum 
{
	MESH_RENDER_WIREFRAME,
	MESH_RENDER_FLAT,
	MESH_RENDER_SHADED,
	MESH_RENDER_TEXTURED,
	MESH_RENDER_END,
};

enum
{
	SCENE_UNIT_UNKNOWN,
	SCENE_UNIT_CENTIMETER,
	SCENE_UNIT_METER,
	SCENE_UNIT_INCH,
	SCENE_UNIT_DECIMETER,
	SCENE_UNIT_END,
};

//////////////////////////////////////////////////////////////////////////////////////
// Placeholder for all the global settings used in hairworks viewer.
// This stores all simple options that are not part of hair instance descriptor
// such as animation logic in UI, scene settings, scene visualization options etc.
//////////////////////////////////////////////////////////////////////////////////////
class CORELIB_EXPORT GlobalSettings
{
public:
	GlobalSettings();

	void	Init();
	static	GlobalSettings& Inst();
	static QTimer& GetFrameTimer();
	static const char* MakeFileName(const char* path, const char* name);

public:

	// animation settings
	bool	m_animate;
	float	m_animationFps;
	int		m_animationIndex;
	float	m_animationSpeed;
	int		m_currentBoneIndex;
	bool	m_firstFrame;
	float	m_frameTime;
	float	m_frameStartTime;
	float	m_frameEndTime;
	bool	m_lockRootBone;
	bool	m_playStopped;
	bool	m_repeatAnimation;
	bool	m_resetAnimationOnLoop;

	bool	m_simulate;
	bool	m_simulateStep;

	float m_timeStep;
	NvCo::FrameCalculator m_simulationFrameCalc;
	float	m_simulationFps;
	
	// scene settings
	bool	m_useDQ;
	bool	m_zup;
	bool	m_lhs;
	bool	m_sceneLoaded;
	int		m_sceneUnitIndex;
	std::string m_backgroundTextureFilePath;

	// statistics and profiling
	bool	m_computeStatistics;
	bool	m_computeProfile;

	// render option
	float	m_fovAngle;
	bool	m_useLighting;
	int		m_msaaOption;
	int		m_renderFrameCnt;
	float	m_renderFps;

	// viusialization
	int		m_controlTextureOption;
	bool	m_previewLOD;
	int		m_renderStyle;
	bool	m_showAxis;
	bool	m_showHUD;
	bool    m_showFPS;
	bool	m_showGraphicsMesh;
	bool	m_showGrid;
	bool	m_showWireframe;
	bool	m_showSkinnedMeshOnly;
	bool	m_visualizeShadowMap;
	bool	m_visualizeWind;
	bool	m_visualizeBoneNames;

	// global wind
	atcore_float3	m_windDir;
	float			m_windStrength;
	float			m_windNoise;

	// import settings
	bool	m_loadTextures;
	bool	m_loadMaterials;
	bool	m_loadGroom;
	bool	m_loadCollision;
	bool	m_loadConstraints;

	// file path
	std::string		m_projectFileDir;
	std::string		m_projectFileName;

	// convenience access functions
public:

	bool isBindPose() { return m_animationIndex == 0; }
	float getSceneUnitInCentimeters();
	static float getSceneUnitInCentimeters(int unitIndex);
	void setSceneUnitIndex(int i);
	void setSceneUnitByUnitInCm(float fUnitInCm);
	static int identifyUnitByUnitInCm(float fUnitInCm);
	static bool isSupportedUnitByUnitInCm(float fUnitInCm);

	void setTimeStep(float timeStep) { m_timeStep = timeStep; }

	bool toggleSimulation();
	void stepSimulation();

	void setRenderFps(float fps);
	void setSimulationFps(float fps);

	void resetAnimation();
	void stepAnimation();
	bool toggleAnimation();
	void toggleAnimationRepeat();
	void getAnimationRange( float& start, float& end);

	std::string getAbsoluteFilePath();
	std::string getAbsoluteFilePath(const char* fileName);
	std::string getRelativePath(const char* filePath);

	atcore_float3 getGravityDir();
};

