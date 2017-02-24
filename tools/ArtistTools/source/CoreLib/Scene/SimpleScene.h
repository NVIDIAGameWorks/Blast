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
// Copyright (c) 2013-2015 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//

#pragma once

#include <Windows.h>
#include "MathUtil.h"
#include <QtCore/QList>
#include <QtCore/QMap>
#include "Camera.h"
#include "Timer.h"
class FurCharacter;

class DeviceManager;
class SampleManager;

namespace NvParameterized
{
	class Interface;
}

struct CameraBookmark
{
	CameraBookmark()
	{
	}
	CameraBookmark(const QString& inName, const Camera& inCamera)
		: name(inName)
		, camera(inCamera)
	{
	}
	QString name;
	Camera camera;
};

//////////////////////////////////////////////////////////////////////////////////////
// This scene object does the followings
//	-	handles the overall project settings load/save
//	-	owns and calls FurRenderer's draw functions
//	-	owns and handle main camera and mouse callbacks
//	-	owns and handles backdoor communication with the SDK and game engines
//////////////////////////////////////////////////////////////////////////////////////
class CORELIB_EXPORT SimpleScene
{
public:
	SimpleScene();

	static SimpleScene*	Inst();

	// prepare shaders, manage objects
	bool Initialize(HWND hWnd, int backdoor = 0);

	bool LoadProject(const char* dir, const char* file);
	bool SaveProject(const char* dir, const char* file);
	bool LoadParameters(NvParameterized::Interface* iface);
	bool SaveParameters(NvParameterized::Interface* iface);
	bool LoadCameraBookmarks(NvParameterized::Interface* iface);
	bool SaveCameraBookmarks(NvParameterized::Interface* iface);
	bool Clear();

	bool LoadSceneFromFbx(const char* dir, const char* fbxName);

	bool IsUpdatingUI() { return m_isUpdatingUI; }
	void setIsUpdatingUI(bool b) { m_isUpdatingUI = b;}
	bool IsSceneLoading() { return 	m_isSceneLoading; }

	Camera*	GetCamera() { return m_pCamera; }
	FurCharacter& GetFurCharacter() { return *m_pFurCharacter; }
	void SetFurCharacter(FurCharacter* pFurCharacter) { m_pFurCharacter = pFurCharacter; }

	DeviceManager& GetDeviceManager() { return *m_pDeviceManager; }
	void SetDeviceManager(DeviceManager* pDeviceManager) { m_pDeviceManager = pDeviceManager; }
	SampleManager& GetSampleManager() { return *m_pSampleManager; }
	void SetSampleManager(SampleManager* pSampleManager) { m_pSampleManager = pSampleManager; }

	void ResetUpDir(bool zup);
	void ResetLhs(bool lhs);

	bool LoadBackgroundTextureFile();
	void ClearBackgroundTexture();

	// backdoor
	void sendParam(const char *str, NvFloat32 v);
	void sendParam(const char *str, NvUInt32 v);
	void sendParam(const char *str, atcore_float3 v);
	void sendParam(const char *str, atcore_float4 v);
	void sendParam(const char *str, NvBoolean v);

	bool IsFurModified() const { return m_isFurModified; }
	bool IsProjectModified() const { return m_isProjectModified; }
	void SetFurModified(bool isModified) {	m_isFurModified = isModified; }
	void SetProjectModified(bool isModified) {	m_isProjectModified = isModified; }

	void UpdateCamera();
	void FitCamera();

	void Draw();
	void Timeout();
	void Resize(int w, int h);
	void Drag(char type);
	void WheelZoom();
	void Shutdown();

	void onMouseDown(atcore_float2 position);
	void onMouseUp(atcore_float2 position);
	void onMouseMove(atcore_float2 position);
	void onMouseWheel(float deltaWheel);

	QString createBookmark();
	void removeBookmark(const QString& name);
	void activateBookmark(const QString& name);
	void renameBookmark(const QString& oldName, const QString& newName);
	QList<QString> getBookmarkNames();

	static float NextTimeStep();

	Timer& GetTimer();

private:
	// initialize scene components
	bool InitCameraMouse(HWND hAppWnd);
	bool InitFurSDK();

	void DrawGround();
	void DrawAxis();
	void DrawHUD();
	void DrawWind();

	// camera / mouse
	void PanCamera();
	void ZoomCamera();
	void WheelZoomCamera();
	void RotateCamera();

	void RotateLightDirection();
	void PanLight();

	void RotateWindDirection();

	QString _generateBookmarkName();

public:

	Camera*				m_pCamera;
	Camera*				m_pWindCamera;

	QList<CameraBookmark>	m_cameraBookmarks;

	FurCharacter*		m_pFurCharacter;

	DeviceManager*		m_pDeviceManager;
	SampleManager*		m_pSampleManager;

	bool				m_isProjectModified;
	bool				m_isFurModified;
	bool				m_isSceneLoading;
	bool				m_isUpdatingUI;
};

