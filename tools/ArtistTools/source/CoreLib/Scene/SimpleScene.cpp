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
#include "SimpleScene.h"

#include "AppMainWindow.h"
#include "Automate.h"
#include "Backdoor.h"
#include "Gamepad.h"
#include "GlobalSettings.h"
#include "Light.h"
#include "Mouse.h"
#include "SimpleRenderable.h"
#include "Stats.h"
#include "CorelibUtils.h"
//#include "Timer.h"
#include "ViewerOutput.h"

#include "FoundationHolder.h"

#include <NvAllocatorCallback.h>
#include <NvErrorCallback.h>
#include <NsAlignedMalloc.h>
#include <NsVersionNumber.h>

#include <Nv/Common/NvCoMemoryAllocator.h>

#include "RenderInterface.h"
#include "DeviceManager.h"

#ifndef NV_ARTISTTOOLS
#include "FurCharacter.h"
#include "FurRenderer.h"
#include "ProjectParams.h"
#include "HairInstance.h"
#include "HairSDK.h"

namespace { // anonymous

class ErrorCallback: public nvidia::NvErrorCallback
{
	public:
	void reportError(nvidia::NvErrorCode::Enum code, const char* message, const char* file, int line) NV_OVERRIDE
	{
		if (code & (nvidia::NvErrorCode::eDEBUG_WARNING | nvidia::NvErrorCode::ePERF_WARNING))
		{
			viewer_warn("%s", message);
			return;
		}
		viewer_err("%s", message);
		if (code != nvidia::NvErrorCode::eNO_ERROR)
		{
			viewer_err("%s", message);
		}
	}
};

class AllocatorCallback: public nvidia::NvAllocatorCallback
{
	public:

	void* allocate(size_t size, const char* typeName, const char* filename, int line) NV_OVERRIDE
	{
		return NvCo::MemoryAllocator::getInstance()->simpleAllocate(size);
	}

	void deallocate(void* ptr) NV_OVERRIDE
	{
		NvCo::MemoryAllocator::getInstance()->simpleDeallocate(ptr);
	}
};

} // namespace anonymous

static AllocatorCallback s_allocator;
static ErrorCallback s_error;

// global singletons (can't create more than one instance)
struct ProjectParamsContext*	g_projectParamsContext = 0;

class CustomLogger : public NvCo::Logger
{
public:
	virtual void log(NvCo::LogSeverity::Enum severity, const char* message, const char* functionName, const char* file, int line) NV_OVERRIDE
	{
		using namespace NvCo;

		switch (severity)
		{
			default:
			case LogSeverity::FATAL_ERROR:
			case LogSeverity::NON_FATAL_ERROR:
				viewer_err("%s", message);
				break;
			case LogSeverity::WARNING:
				viewer_warn("%s", message);
				break;
			case LogSeverity::INFO:
				viewer_info("%s", message);
				break;
			case LogSeverity::DEBUG_INFO:
				viewer_msg("%s", message);
				break;
		}
	}
	virtual void flush() NV_OVERRIDE {}
};

CustomLogger g_logHandler;
#else
#include "GPUProfiler.h"
#include "MeshShaderParam.h"
#include <Nv\Common\NvCoLogger.h>
#endif // NV_ARTISTTOOLS

Gamepad& theGamepad = Gamepad::Instance();
Mouse							g_mouse;
Timer							g_fpsTimer;
Backdoor*						g_pBackdoor;

///////////////////////////////////////////////////////////////////////////////
SimpleScene* SimpleScene::Inst()
{
	static SimpleScene scene;
	return &scene;
}

///////////////////////////////////////////////////////////////////////////////
SimpleScene::SimpleScene()
	:
	m_pCamera(0),
	m_pWindCamera(0),
	m_isProjectModified(false),
	m_isFurModified(false),
	m_isSceneLoading(false)
{
#ifndef NV_ARTISTTOOLS
	m_pFurCharacter = new FurCharacter;
#endif // NV_ARTISTTOOLS
}

///////////////////////////////////////////////////////////////////////////////
bool SimpleScene::Initialize( HWND hWnd, int backdoor)
{
	g_fpsTimer.Start();

	if(!InitCameraMouse(hWnd)) 
		return false;

	if (!SimpleRenderable::Initialize())
		return false;

	Light::Initialize();

	//nvidia::shdfnd::initializeSharedFoundation(NV_FOUNDATION_VERSION, s_allocator, s_error);

	FoundationHolder::GetFoundation();

	GPUProfiler_Initialize();


	if (backdoor == 1) // master mode
		g_pBackdoor = createBackdoor("BACKDOOR_SERVER","BACKDOOR_CLIENT");

	SetFurModified(false);
	SetProjectModified(false);

	CoreLib::Inst()->SimpleScene_Initialize(backdoor);

	AppMainWindow::Inst().updateUI();

	return true;
}

///////////////////////////////////////////////////////////////////////////////
void SimpleScene::Shutdown()
{
#ifndef SAFE_DELETE
#define SAFE_DELETE(x) {if(x!=NV_NULL){delete x;x=NV_NULL;}}
#endif
	
	SAFE_DELETE(m_pCamera);
	SAFE_DELETE(m_pWindCamera);

	GPUProfiler_Shutdown();

	SimpleRenderable::Shutdown();
	Light::Shutdown();

	if (g_pBackdoor)
	{
		g_pBackdoor->release();
		g_pBackdoor = nullptr;
	}

#ifndef NV_ARTISTTOOLS
	GetFurCharacter().Clear();

	ShutdownBlastSDK();

	if (g_projectParamsContext)
	{
		ReleaseProjectParamsContext(g_projectParamsContext);
		g_projectParamsContext = nullptr;
	}
#else
	CoreLib::Inst()->SimpleScene_Shutdown();
#endif // NV_ARTISTTOOLS

	RenderInterface::Shutdown();

	nvidia::shdfnd::terminateSharedFoundation();
}

///////////////////////////////////////////////////////////////////////////////
bool SimpleScene::Clear()
{
	GlobalSettings::Inst().m_sceneLoaded = false;
	GlobalSettings::Inst().m_firstFrame = true;

#ifndef NV_ARTISTTOOLS
	GetFurCharacter().Clear();
#else
	CoreLib::Inst()->SimpleScene_Clear();
#endif // NV_ARTISTTOOLS

	m_cameraBookmarks.clear();
	return true;
}

///////////////////////////////////////////////////////////////////////////////
bool SimpleScene::InitCameraMouse(HWND hAppWnd)
{
	// default camera pose settings
	m_pCamera = new Camera(GlobalSettings::Inst().m_zup, false);
	m_pCamera->SetDefaults();

	// init wind camera
	{
		m_pWindCamera = new Camera(false);
		m_pWindCamera->SetDefaults();

		GlobalSettings::Inst().m_windDir = m_pWindCamera->GetZAxis();
	}

	// init mouse
	g_mouse.Initialize(::GetModuleHandle(NV_NULL), (HWND)hAppWnd/*_hWidget*/);

	theGamepad.Initialize();
	return true;
}

float SimpleScene::NextTimeStep()
{
	static LONGLONG g_lastRenderTicks = 0;

	// Work out the timestep
	LONGLONG ticks = g_fpsTimer.GetTicksElapsed();
	LONGLONG deltaTicks = ticks - g_lastRenderTicks;
	g_lastRenderTicks = ticks;

	float timeStep = deltaTicks / float(g_fpsTimer.GetTicksPerSecond());

	const float maxTimeStep = 0.1f;

	timeStep = (timeStep < 0.0f) ? 0.0f : timeStep;
	timeStep = (timeStep > maxTimeStep) ? maxTimeStep : timeStep;

	return timeStep;
}

Timer& SimpleScene::GetTimer()
{
	return g_fpsTimer;
}

///////////////////////////////////////////////////////////////////////////////
void SimpleScene::Draw()
{
	if (IsSceneLoading())
		return;

	GlobalSettings& globalSettings = GlobalSettings::Inst();

	// draw background
	RenderInterface::ClearRenderWindow(0.35f, 0.35f, 0.35f);
	RenderInterface::RenderBackgroundTexture();
	
	globalSettings.setTimeStep(NextTimeStep());
	//globalSettings.setTimeStep(1.0f/60.0f);

	// increment frame timer
	if (globalSettings.m_animate)
	{
		globalSettings.stepAnimation();
	}

	if (globalSettings.m_playStopped)
		AppMainWindow::Inst().updateMainToolbar();

	// update camera 
	UpdateCamera();

	// show ground grid
	if (globalSettings.m_showGrid)
		DrawGround();

	// draw wind icon
	if (globalSettings.m_visualizeWind)
		DrawWind();

	// draw axis lines
	if (globalSettings.m_showAxis)
		DrawAxis();

	// handle game pad
	theGamepad.Process();

	// visualize shadow map
	if (GlobalSettings::Inst().m_visualizeShadowMap)
		Light::RenderShadowMap();

	CoreLib::Inst()->SimpleScene_Draw_DX12();

	RenderInterface::SwitchToDX11();

	CoreLib::Inst()->SimpleScene_Draw_DX11();

	RenderInterface::FlushDX11();

	// draw lights
	Light::DrawLights(m_pCamera);

	// present current window
	RenderInterface::PresentRenderWindow();
}

///////////////////////////////////////////////////////////////////////////////
void SimpleScene::Timeout()
{
	if (m_pCamera == nullptr)
	{
		// this could be called when quiting. have to check nullptr
		return;
	}

	g_mouse.Update();

	if (IsAutomateMode())
	{
		AutomateRun();
	}

	Draw();

	if (g_pBackdoor)
	{
		int argc;
		const char **argv = g_pBackdoor->getInput(argc);
		if (argc > 0)
		{
			char message[1024];
			strcpy(message,"");
			for (int i = 0; i < argc; i++)
			{
				strcat(message, argv[i]);
				strcat(message," ");
			}

			viewer_info("Message received: %s", message);
		}
	}

}

#include <Shlwapi.h>
#include <FbxUtil.h>

///////////////////////////////////////////////////////////////////////////////
bool 
SimpleScene::LoadSceneFromFbx(const char* dir, const char* fbxName)
{
	return CoreLib::Inst()->SimpleScene_LoadSceneFromFbx(dir, fbxName);

	/*
	GlobalSettings& globalSettings = GlobalSettings::Inst();

	char fbxFilePath[MAX_PATH];

	float sceneUnit = globalSettings.getSceneUnitInCentimeters();

	PathCombineA(fbxFilePath, dir, fbxName);

	AppMainWindow::Inst().setProgress("Initializing FBX loader", 0);
	FbxUtil::Initialize(fbxFilePath, sceneUnit);

	char rootBoneName[MAX_PATH];
	int upAxis = 0;

	FbxUtil::GetGlobalSettings(
		&globalSettings.m_frameStartTime, 
		&globalSettings.m_frameEndTime, 
		&globalSettings.m_animationFps, 
		&upAxis, rootBoneName);

	if (upAxis == 1)
		SimpleScene::Inst()->ResetUpDir(false);
	else if (upAxis = 2)
		SimpleScene::Inst()->ResetUpDir(true);

#ifndef NV_ARTISTTOOLS
	SimpleScene::Inst()->GetFurCharacter().LoadMeshFromFbx(dir, fbxName);
#else
	CoreLib::Inst()->SimpleScene_LoadSceneFromFbx(dir, fbxName);
#endif // NV_ARTISTTOOLS

	FbxUtil::Release();

	globalSettings.m_sceneLoaded = true;
	globalSettings.m_animationIndex = 1;
	globalSettings.m_firstFrame = true;

	//globalSettings.m_currentBoneIndex = findBoneByName(rootBoneName);
	globalSettings.resetAnimation();

	return true;
	*/
}

///////////////////////////////////////////////////////////////////////////////
bool SimpleScene::LoadProject(const char* dir, const char* file)
{
	Clear();

	GlobalSettings& globalSettings = GlobalSettings::Inst();
	globalSettings.m_projectFileDir = dir;
	globalSettings.m_projectFileName = file;

	CoreLib::Inst()->SimpleScene_LoadProject(dir, file);

	SetProjectModified(false);
	
	return true;
}

///////////////////////////////////////////////////////////////////////////////
bool SimpleScene::LoadParameters(NvParameterized::Interface* iface)
{
	m_isSceneLoading = true;

	AppMainWindow::Inst().startProgress();

#ifndef NV_ARTISTTOOLS
	nvidia::parameterized::HairProjectParameters* params = static_cast<nvidia::parameterized::HairProjectParameters*>(iface);

	nvidia::parameterized::HairProjectParametersNS::ParametersStruct& srcDesc = params->parameters();

	if (m_pCamera)
	{
		m_pCamera->LoadParameters(&srcDesc.camera);
	}

	LoadCameraBookmarks(iface);

	if (m_pWindCamera)
	{
		m_pWindCamera->LoadParameters(&srcDesc.windCamera);
	}
	
	GlobalSettings& globalSettings = GlobalSettings::Inst();

	// Load scene settings
	globalSettings.m_repeatAnimation	= srcDesc.scene.repeatAnimation;
	globalSettings.m_animationSpeed		= srcDesc.scene.animationSpeed;
	globalSettings.m_showGrid			= srcDesc.scene.showGrid;
	globalSettings.m_showAxis			= srcDesc.scene.showAxis;
	globalSettings.m_zup				= srcDesc.scene.upAxis == 1;
	globalSettings.m_sceneUnitIndex		= srcDesc.scene.sceneUnitIndex;

	// Load renderer settings
	NvParameterized::Handle handle(iface);

	// Load fbx paths
	if (iface->getParameterHandle("fbxFilePaths", handle) == NvParameterized::ERROR_NONE)
	{
		int arraySize;
		handle.getArraySize(arraySize);
		char** strArray = new char*[arraySize];
		handle.getParamStringArray(strArray, arraySize);
		for (int idx = 0; idx < arraySize; ++idx)
		{
			LoadSceneFromFbx(
				globalSettings.m_projectFileDir.c_str(),
				strArray[idx]);
		}
		delete [] strArray;
	}

	// get general fur renderer settings
	if (iface->getParameterHandle("renderer", handle) == NvParameterized::ERROR_NONE)
		FurRenderer::LoadParameters(handle);

	// get fur character mesh setting 
	if (false == GetFurCharacter().LoadMeshParameters(handle))
		return false;

	// Load apx paths (hair instances)
	if (iface->getParameterHandle("apxFilePaths", handle) == NvParameterized::ERROR_NONE)
	{
		if (false == GetFurCharacter().LoadHairParameters(handle))
			return false;
	}
#else
	CoreLib::Inst()->SimpleScene_LoadParameters(iface);
#endif // NV_ARTISTTOOLS
	
	m_isSceneLoading = false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
bool SimpleScene::SaveProject(const char* dir, const char* file)
{
	GlobalSettings& globalSettings = GlobalSettings::Inst();

	globalSettings.m_projectFileDir = dir;
	globalSettings.m_projectFileName = file;

	std::string saveFilePath = globalSettings.getAbsoluteFilePath();
	
	std::string tempFilePath = utils::GetTempFilePath();

#ifndef NV_ARTISTTOOLS
	if (ProjectParamsSave(g_projectParamsContext, tempFilePath.c_str(), this))
	{
		if (!utils::RenameFile(tempFilePath.c_str(), saveFilePath.c_str(), true /* overwrite */))
		{
			return false;
		}
		SetProjectModified(false);
		return true;
	}
#else
	return CoreLib::Inst()->SimpleScene_SaveProject(dir, file);
#endif // NV_ARTISTTOOLS

	return false;
}

///////////////////////////////////////////////////////////////////////////////
bool SimpleScene::LoadBackgroundTextureFile()
{
	QString texName = AppMainWindow::Inst().OpenTextureFile();

	return RenderInterface::LoadBackgroundTexture(texName.toLocal8Bit());
}

//////////////////////////////////////////////////////////////////////////
void SimpleScene::ClearBackgroundTexture()
{
	RenderInterface::ClearBackgroundTexture();
}


//////////////////////////////////////////////////////////////////////////
// slots and handlers / mouse / camera

void SimpleScene::Resize( int w, int h )
{
	RenderInterface::ResizeRenderWindow(w,h);

	m_pCamera->SetSize(w,h);

	UpdateCamera();
			
	SetProjectModified(true);
}

void SimpleScene::onMouseDown(atcore_float2 position)
{
	g_mouse.SetPosition(position);
}

void SimpleScene::onMouseUp(atcore_float2 position)
{
}

void SimpleScene::onMouseMove(atcore_float2 position)
{
	g_mouse.SetDelta(position);
}

void SimpleScene::onMouseWheel(float deltaWheel)
{
	g_mouse.SetDeltaWheel(deltaWheel);
}

QString SimpleScene::createBookmark()
{
	QString bookmark = _generateBookmarkName();
	m_cameraBookmarks.append(CameraBookmark(bookmark, *m_pCamera));
	return bookmark;
}

void SimpleScene::removeBookmark(const QString& name)
{
	int bookmarksCount = m_cameraBookmarks.size();
	for (int i = 0; i < bookmarksCount; ++i)
	{
		const CameraBookmark& bookmark = m_cameraBookmarks[i];
		if (bookmark.name == name)
		{
			m_cameraBookmarks.removeAt(i);
			break;
		}
	}
}

void SimpleScene::activateBookmark(const QString& name)
{
	int bookmarksCount = m_cameraBookmarks.size();
	for (int i = 0; i < bookmarksCount; ++i)
	{
		const CameraBookmark& bookmark = m_cameraBookmarks[i];
		if (bookmark.name == name)
		{
			*m_pCamera = bookmark.camera;
			break;
		}
	}
}

void SimpleScene::renameBookmark(const QString& oldName, const QString& newName)
{
	int bookmarksCount = m_cameraBookmarks.size();
	for (int i = 0; i < bookmarksCount; ++i)
	{
		CameraBookmark& bookmark = m_cameraBookmarks[i];
		if (bookmark.name == oldName)
		{
			bookmark.name = newName;
			break;
		}
	}
}

QList<QString> SimpleScene::getBookmarkNames()
{
	QList<QString> names;
	int bookmarksCount = m_cameraBookmarks.size();
	for (int i = 0; i < bookmarksCount; ++i)
	{
		const CameraBookmark& bookmark = m_cameraBookmarks[i];
		names.append(bookmark.name);
	}

	return names;
}

void SimpleScene::Drag( char type )
{
	g_mouse.Update();
	
	switch(type)
	{
	case 'R': RotateCamera(); break;
	case 'Z': ZoomCamera(); break;
	case 'P': PanCamera(); break;
	case 'L': RotateLightDirection(); break;
	case 'W': RotateWindDirection(); break;

	case 'K': PanLight(); break;
	}
	SetProjectModified(true);
}

void SimpleScene::WheelZoom()
{
	WheelZoomCamera();
	SetProjectModified(true);
}

void SimpleScene::PanCamera()
{
	if (g_mouse.GetDelta().x != 0.0f || g_mouse.GetDelta().y != 0.0f)
	{
		int w = m_pCamera->GetWidth();
		int h = m_pCamera->GetHeight();

		atcore_float2 delta = g_mouse.GetDelta() * 2.0f;
		delta.x /= (float)(w);
		delta.y /= (float)(h);
		m_pCamera->Pan(delta);
	}
	SetProjectModified(true);
}

void SimpleScene::ZoomCamera()
{
	float dz = (- g_mouse.GetDelta().y);

	if (dz != 0.0f)
	{
		dz *= 5.0f / m_pCamera->GetHeight();

		m_pCamera->Dolly(dz);
	}
	SetProjectModified(true);
}

void SimpleScene::WheelZoomCamera()
{
	float dz = - g_mouse.GetDeltaWheel() ;

	if (dz != 0.0f)
	{
		dz /= 5.0f;

		m_pCamera->Dolly(dz);
	}
	SetProjectModified(true);
}

void SimpleScene::RotateCamera()
{
	if (g_mouse.GetDelta().x != 0.0f || g_mouse.GetDelta().y != 0.0f)
	{
		m_pCamera->Orbit(g_mouse.GetDelta() * 0.125f);
	}
	SetProjectModified(true);
}

void SimpleScene::RotateLightDirection()
{
	if (g_mouse.GetDelta().x != 0.0f || g_mouse.GetDelta().y != 0.0f)
	{
		atcore_float2 delta = g_mouse.GetDelta() * 0.5f;
		delta.y *= -1.0f;

		if (Light::GetLinkLightOption())
		{
			Light* pKeyLight = Light::GetLight(Light::KEY_LIGHT);
			Light* pFillLight = Light::GetLight(Light::FILL_LIGHT);
			Light* pRimLight = Light::GetLight(Light::RIM_LIGHT);
			if (pKeyLight->m_selected)
			{
				pKeyLight->Orbit(delta);
				pFillLight->Orbit(delta);
				pRimLight->Orbit(delta);
			}
			else
			{
				if (pFillLight->m_selected)
				{
					pFillLight->Orbit(delta);
				}

				if (pRimLight->m_selected)
				{
					pRimLight->Orbit(delta);
				}
			}

			Light* pEnvLight = Light::GetLight(Light::ENV_LIGHT);
			if (pEnvLight->m_selected)
			{
				pEnvLight->Orbit(delta);
			}
		}
		else
		{
			for (int i = 0; i < 4; i++)
			{
				Light* pLight = Light::GetLight(i);
				if (!pLight)
					continue;

				if (pLight->m_selected)
				{
					pLight->Orbit(delta);
				}
			}
		}
	}
	SetProjectModified(true);
}

void SimpleScene::PanLight()
{
	if (g_mouse.GetDelta().x != 0.0f || g_mouse.GetDelta().y != 0.0f)
	{
		int w = m_pCamera->GetWidth();
		int h = m_pCamera->GetHeight();

		atcore_float2 delta = g_mouse.GetDelta() * 2.0f;
		delta.x /= (float)(w);
		delta.y /= (float)(h);

		for (int i = 0; i < 4; i++)
		{
			Light* pLight = Light::GetLight(i);
			if (!pLight)
				continue;

			if (pLight->m_selected)
				pLight->Pan(delta, m_pCamera->GetXAxis(), m_pCamera->GetYAxis());
		}
	}
	SetProjectModified(true);
}

void SimpleScene::RotateWindDirection()
{
	if (g_mouse.GetDelta().x != 0.0f || g_mouse.GetDelta().y != 0.0f)
	{
		m_pWindCamera->Orbit(g_mouse.GetDelta() * 0.5f);

		GlobalSettings::Inst().m_windDir = m_pWindCamera->GetZAxis();		
	}

	SetProjectModified(true);
}

QString SimpleScene::_generateBookmarkName()
{
	QString name;
	for (int i = 1; ; i++)
	{
		name = QString("View%1").arg(i, 2, 10, QChar('0'));
		int found = -1;
		int bookmarksCount = m_cameraBookmarks.size();
		for (int j = 0; j < bookmarksCount; ++j)
		{
			const CameraBookmark& bookmark = m_cameraBookmarks[j];
			if (bookmark.name == name)
			{
				found = j;
				break;
			}
		}

		if (-1 == found)
			break;
	}
	return name;
}

void SimpleScene::UpdateCamera()
{
	if (!m_pCamera)
		return;

	float sceneUnit = GlobalSettings::Inst().getSceneUnitInCentimeters() ;
	float fov = (GlobalSettings::Inst().m_fovAngle / 360.0f) * 3.141592653589793;	

	m_pCamera->SetFOV(fov);

	int w = m_pCamera->GetWidth();
	int h = m_pCamera->GetHeight();

	float aspect = ((float)(w) / (float)(h));

	float minZ = 1.0f;
	float maxZ = 10000.0f;	// should calculate dynamically
/*
	if (sceneUnit != 0.0f)
	{
		minZ /= sceneUnit;
		maxZ /= sceneUnit;
	}
*/
	m_pCamera->SetAspetRatio(aspect);
	m_pCamera->SetZFar(maxZ);
	m_pCamera->SetZNear(minZ);

	m_pCamera->Perspective();

	CoreLib::Inst()->SimpleScene_UpdateCamera();
}

void SimpleScene::FitCamera()
{
	if (!m_pCamera)
		return;

	atcore_float3 center, extents;

#ifndef NV_ARTISTTOOLS
	if (m_pFurCharacter)
		m_pFurCharacter->ComputeBounds(center, extents, false);
#else
	bool valid = CoreLib::Inst()->SimpleScene_FitCamera(center, extents);
	if (!valid)
	{
		return;
	}
#endif // NV_ARTISTTOOLS

	m_pCamera->FitBounds(center, extents);
}

void SimpleScene::ResetUpDir(bool zup)
{
	m_pCamera->ResetUpDir(zup);
	m_pWindCamera->ResetUpDir(zup);
	Light::ResetUpDir(zup);

	GlobalSettings::Inst().m_zup = zup;

	CoreLib::Inst()->SimpleScene_ResetUpDir(zup);

	SetProjectModified(true);
}

void SimpleScene::ResetLhs(bool lhs)
{
	m_pCamera->ResetLhs(lhs);
	m_pWindCamera->ResetLhs(lhs);
	Light::ResetLhs(lhs);

	GlobalSettings::Inst().m_lhs = lhs;
}

///////////////////////////////////////////////////////////////////////////////
bool SimpleScene::SaveParameters(NvParameterized::Interface* iface)
{
#ifndef NV_ARTISTTOOLS
	nvidia::parameterized::HairProjectParameters* params = static_cast<nvidia::parameterized::HairProjectParameters*>(iface);
	nvidia::parameterized::HairProjectParametersNS::ParametersStruct& targetDesc = params->parameters();

	if (m_pCamera)
		m_pCamera->SaveParameters(&targetDesc.camera);

	SaveCameraBookmarks(iface);

	if (m_pWindCamera)
		m_pWindCamera->SaveParameters(&targetDesc.windCamera);

	// Save scene settings
	GlobalSettings& globalSettings = GlobalSettings::Inst();

	targetDesc.scene.repeatAnimation	= globalSettings.m_repeatAnimation;
	targetDesc.scene.animationSpeed		= globalSettings.m_animationSpeed;
	targetDesc.scene.showGrid			= globalSettings.m_showGrid;
	targetDesc.scene.showAxis			= globalSettings.m_showAxis;

	targetDesc.scene.upAxis				= (globalSettings.m_zup) ? 1 : 2;
	targetDesc.scene.sceneUnitIndex		= globalSettings.m_sceneUnitIndex;

	FurCharacter& character = GetFurCharacter();
	NvParameterized::Handle handle(iface);

	// Save renderer settings
	if (iface->getParameterHandle("renderer", handle) == NvParameterized::ERROR_NONE)
	{
		FurRenderer::SaveParameters(handle);
		character.SaveMeshParameters(handle);
	}

	if (iface->getParameterHandle("renderer.textureFilePath", handle) == NvParameterized::ERROR_NONE)
	{
		std::string textureFilePath = globalSettings.getRelativePath(globalSettings.m_backgroundTextureFilePath.c_str());
		handle.setParamString(textureFilePath.c_str());
	}

	// save hair path
	if (iface->getParameterHandle("apxFilePaths", handle) == NvParameterized::ERROR_NONE)
		character.SaveHairParameters(handle);
#else
	CoreLib::Inst()->SimpleScene_SaveParameters(iface);
#endif // NV_ARTISTTOOLS

	return true;
}

///////////////////////////////////////////////////////////////////////////////
bool SimpleScene::LoadCameraBookmarks(NvParameterized::Interface* iface)
{
#ifndef NV_ARTISTTOOLS
	nvidia::parameterized::HairProjectParameters* params = static_cast<nvidia::parameterized::HairProjectParameters*>(iface);
	nvidia::parameterized::HairProjectParametersNS::ParametersStruct& srcDesc = params->parameters();
	nvidia::parameterized::HairProjectParametersNS::CameraBookmark_DynamicArray1D_Type& bookmarks = srcDesc.cameraBookmarks;

	NvParameterized::Handle cameraBookmarksHandle(iface);
	if (iface->getParameterHandle("cameraBookmarks", cameraBookmarksHandle) != NvParameterized::ERROR_NONE)
		return false;

	int numCameraBookmarks = 0;
	cameraBookmarksHandle.getArraySize(numCameraBookmarks);
	for (int idx = 0; idx < numCameraBookmarks; ++idx)
	{
		NvParameterized::Handle cameraBookmarkHandle(cameraBookmarksHandle);
		if (cameraBookmarksHandle.getChildHandle(idx, cameraBookmarkHandle) == NvParameterized::ERROR_NONE)
		{
			CameraBookmark cameraBookmark;
			cameraBookmark.camera.LoadParameters((void*)&(bookmarks.buf[idx].camera));
			cameraBookmark.camera.SetSize(m_pCamera->GetWidth(), m_pCamera->GetHeight());
			cameraBookmark.name = bookmarks.buf[idx].name.buf;
			m_cameraBookmarks.append(cameraBookmark);
		}
	}
#else
	CoreLib::Inst()->SimpleScene_LoadCameraBookmarks(iface);
#endif // NV_ARTISTTOOLS

	return true;
}

///////////////////////////////////////////////////////////////////////////////
bool SimpleScene::SaveCameraBookmarks(NvParameterized::Interface* iface)
{
#ifndef NV_ARTISTTOOLS
	nvidia::parameterized::HairProjectParameters* params = static_cast<nvidia::parameterized::HairProjectParameters*>(iface);
	nvidia::parameterized::HairProjectParametersNS::ParametersStruct& srcDesc = params->parameters();
	nvidia::parameterized::HairProjectParametersNS::CameraBookmark_DynamicArray1D_Type& bookmarks = srcDesc.cameraBookmarks;

	NvParameterized::Handle cameraBookmarksHandle(iface);
	if (iface->getParameterHandle("cameraBookmarks", cameraBookmarksHandle) != NvParameterized::ERROR_NONE)
		return false;

	int numCameraBookmarks = m_cameraBookmarks.size();
	cameraBookmarksHandle.resizeArray(numCameraBookmarks);

	for (int idx = 0; idx < numCameraBookmarks; ++idx)
	{
		NvParameterized::Handle cameraBookmarkHandle(cameraBookmarksHandle);
		if (cameraBookmarksHandle.getChildHandle(idx, cameraBookmarkHandle) == NvParameterized::ERROR_NONE)
		{
			NvParameterized::Handle tempHandle(cameraBookmarkHandle);
			CameraBookmark& bookmark = m_cameraBookmarks[idx];

			if (ParamGetChild(cameraBookmarkHandle, tempHandle, "name"))
			{
				tempHandle.setParamString(bookmark.name.toStdString().c_str());
			}

			bookmark.camera.SaveParameters((void*)&(bookmarks.buf[idx].camera));
		}
	}
#else
	CoreLib::Inst()->SimpleScene_SaveCameraBookmarks(iface);
#endif // NV_ARTISTTOOLS

	return true;
}

///////////////////////////////////////////////////////////////////////////////
void SimpleScene::sendParam(const char *str, NvFloat32 v)
{
	if (g_pBackdoor)
	{
		char message[1024];
		sprintf(message, "%s %f", str, v);
//		viewer_info(message);
		g_pBackdoor->send("%s", message);
	}
}

///////////////////////////////////////////////////////////////////////////////
void SimpleScene::sendParam(const char *str, NvUInt32 v)
{
	if (g_pBackdoor)
	{
		char message[1024];
		sprintf(message, "%s %d", str, v);
//		viewer_info(message);
		g_pBackdoor->send("%s", message);
	}
}

///////////////////////////////////////////////////////////////////////////////
void SimpleScene::sendParam(const char *str, atcore_float3 v)
{
	if (g_pBackdoor)
	{
		char message[1024];
		sprintf(message, "%s %f %f %f", str, v.x, v.y, v.z);
//		viewer_info(message);
		g_pBackdoor->send("%s", message);
	}
}

///////////////////////////////////////////////////////////////////////////////
void SimpleScene::sendParam(const char *str, atcore_float4 v)
{
	if (g_pBackdoor)
	{
		char message[1024];
		sprintf(message, "%s %f %f %f %f", str, v.x, v.y, v.z, v.w);
//		viewer_info(message);
		g_pBackdoor->send("%s", message);
	}
}

///////////////////////////////////////////////////////////////////////////////
void SimpleScene::sendParam(const char *str, NvBoolean v)
{
	if (g_pBackdoor)
	{
		char message[1024];
		sprintf(message, "%s %d", str, int(v));
//		viewer_info(message);
		g_pBackdoor->send("%s", message);
	}
}

///////////////////////////////////////////////////////////////////////////////
void SimpleScene::DrawGround()
{
	bool zup = GlobalSettings::Inst().m_zup;

	SimpleShaderParam param;
	{
		param.useVertexColor = true;
		gfsdk_makeIdentity(param.world);
		param.view = m_pCamera->GetViewMatrix();
		param.projection = m_pCamera->GetProjectionMatrix();
	}
	RenderInterface::CopyShaderParam(RenderInterface::SHADER_TYPE_SIMPLE_COLOR,
		(void*)&param, sizeof(SimpleShaderParam) );

	SimpleRenderable::Draw(
		zup ? SimpleRenderable::GROUND_ZUP :
		SimpleRenderable::GROUND_YUP, false);
}

///////////////////////////////////////////////////////////////////////////////
void SimpleScene::DrawWind()
{
	// Convert camera axis into world matrix
	atcore_float4x4 windMat = gfsdk_transpose(m_pWindCamera->GetViewMatrix());

	// Extract rotation axis from the view matrix
	atcore_float4x4 viewMat = m_pCamera->GetViewMatrix();
	if (m_pCamera->UseLHS())
		gfsdk_setPosition(viewMat, gfsdk_makeFloat3(0, 0, 80));
	else
		gfsdk_setPosition(viewMat, gfsdk_makeFloat3(0, 0, -80));
	
	SimpleShaderParam param;
	{
		param.useVertexColor = true;
		param.world = windMat;
		param.view = viewMat;
		param.projection = m_pCamera->GetProjectionMatrix();
	}
	RenderInterface::CopyShaderParam(RenderInterface::SHADER_TYPE_SIMPLE_COLOR,
		(void*)&param, sizeof(SimpleShaderParam) );

	int targetWidth		= m_pCamera->GetWidth();
	int targetHeight	= m_pCamera->GetHeight();
	float aspectRatio = (float)targetWidth / (float)targetHeight;

	// Wind view size
	const int srcHeight = 64;
	const int srcWidth = aspectRatio*srcHeight;
	const int originX = 64;

	RenderInterface::Viewport savedVP, vp;
	RenderInterface::GetViewport(savedVP);

	// set the viewport transform
	{
		vp.TopLeftX = originX + ( (srcWidth > srcHeight) ? (srcHeight - srcWidth)/2.0f : 0);
		vp.TopLeftY = targetHeight-srcHeight;
		vp.Width    = (float)srcWidth;
		vp.Height   = (float)srcHeight;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
	}

	RenderInterface::SetViewport(vp);

	bool zUP = GlobalSettings::Inst().m_zup;
	SimpleRenderable::Draw(zUP ? 
		SimpleRenderable::WIND_ZUP : 
		SimpleRenderable::WIND_YUP, false);

	// Restore states
	RenderInterface::SetViewport(savedVP);
}

///////////////////////////////////////////////////////////////////////////////
void SimpleScene::DrawAxis()
{
	// Extract rotation axis from the view matrix
	atcore_float4x4 viewMat = m_pCamera->GetViewMatrix();
	if (m_pCamera->UseLHS())
		gfsdk_setPosition(viewMat, gfsdk_makeFloat3(0, 0, 4));
	else
		gfsdk_setPosition(viewMat, gfsdk_makeFloat3(0, 0, -4));

	SimpleShaderParam param;
	{
		param.useVertexColor = true;
		gfsdk_makeIdentity(param.world);
		param.view = viewMat;
		param.projection = m_pCamera->GetProjectionMatrix();
	}
	RenderInterface::CopyShaderParam(RenderInterface::SHADER_TYPE_SIMPLE_COLOR,
		(void*)&param, sizeof(SimpleShaderParam) );

	int targetWidth		= m_pCamera->GetWidth();
	int targetHeight	= m_pCamera->GetHeight();
	float aspectRatio = (float)targetWidth / (float)targetHeight;

	// Axis view size
	const int srcHeight = 64;
	const int srcWidth = aspectRatio*srcHeight;

	RenderInterface::Viewport savedVP, vp;
	RenderInterface::GetViewport(savedVP);

	// set the viewport transform
	{
		vp.TopLeftX = (srcWidth > srcHeight) ? (srcHeight - srcWidth)/2.0f : 0; // To make it like a square view
		vp.TopLeftY = targetHeight-srcHeight;
		vp.Width    = (float)srcWidth;
		vp.Height   = (float)srcHeight;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
	}

	RenderInterface::SetViewport(vp);

	bool zUP = GlobalSettings::Inst().m_zup;
	SimpleRenderable::Draw(zUP ? 
		SimpleRenderable::AXIS_ZUP : 
		SimpleRenderable::AXIS_YUP, false);

	RenderInterface::SetViewport(savedVP);
}
