#include <QtCore/QtPlugin>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QAction>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QPushButton>
#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include "PluginBlast.h"
#include "BlastPlugin.h"
#include "SlideSpinBox.h"
#include "Utils.h"
#include "CorelibUtils.h"
#include "Settings.h"
#include "GlobalSettings.h"
#include "SimpleScene.h"
#include "Camera.h"
#include "Light.h"
#include "ProjectParams.h"
#include "FoundationHolder.h"
#include "AppMainWindow.h"
#include "ViewerOutput.h"
#include "ExpandablePanel.h"
#include "DisplayMeshesPanel.h"
#include "PluginBlast.h"
#include "BlastToolbar.h"
#include "MaterialLibraryPanel.h"
#include "MaterialAssignmentsPanel.h"
#include "FileReferencesPanel.h"
#include "GeneralPanel.h"
#include "BlastCompositePanel.h"
#include "BlastSceneTree.h"
#include "FiltersDockWidget.h"
#include "DefaultDamagePanel.h"
#include "FiltersDockWidget.h"
#include "FractureCutoutSettingsPanel.h"
#include "FractureGeneralPanel.h"
#include "FractureShellCutSettingsPanel.h"
#include "FractureSliceSettingsPanel.h"
#include "FractureVisualizersPanel.h"
#include "FractureVoronoiSettingsPanel.h"
#include "SupportPanel.h"
#include "BlastSceneTree.h"
#include "FiltersDockWidget.h"

// A singleton, sort of... To pass the events from WindowProc to the object.
DeviceManager* g_DeviceManagerInstance = NULL;

HWND g_hWnd = 0;

DeviceManager* GetDeviceManager()
{
	return g_DeviceManagerInstance;
}

QString BlastPlugin::GetPluginName()
{
	return BlastPluginName;
}

bool BlastPlugin::CoreLib_RunApp()
{
	return true;
}

bool BlastPlugin::LoadRenderPlugin(std::string api)
{
	return PluginBlast::Create(api);
}

bool BlastPlugin::GetBoneNames(std::vector<std::string>& BoneNames)
{
	return true;
}

bool BlastPlugin::MainToolbar_updateValues()
{ 
	return true; 
}

bool BlastPlugin::CurveEditor_updateValues(int _paramId, float* _values)
{ 
	return true; 
}

bool BlastPlugin::CurveEditor_onUpdateValues(int _paramId, float* _values)
{ 
	return true; 
}

bool BlastPlugin::DisplayMeshesPanel_updateValues()
{
	return true; 
}
bool BlastPlugin::DisplayMeshesPanel_EmitToggleSignal(unsigned int id, bool visible)
{ 
	return true; 
}

bool BlastPlugin::Camera_LoadParameters(void* ptr, Camera* pCamera)
{
	nvidia::parameterized::BlastProjectParametersNS::Camera_Type* param =
		static_cast<nvidia::parameterized::BlastProjectParametersNS::Camera_Type*>(ptr);

	pCamera->_zup = param->flags == 1;
	pCamera->_fov = param->fov;
	pCamera->_aspectRatio = param->aspectRatio;
	pCamera->_znear = param->znear;
	pCamera->_zfar = param->zfar;
	pCamera->_isPerspective = param->isPerspective;
	memcpy(&pCamera->_eye, &param->eye, sizeof(pCamera->_eye));
	memcpy(&pCamera->_at, &param->at, sizeof(pCamera->_at));
	pCamera->_lookDistance = param->lookDistance;
	memcpy(&pCamera->_orientation, &param->orientation, sizeof(pCamera->_orientation));
	memcpy(&pCamera->_viewMatrix, &param->viewMatrix, sizeof(pCamera->_viewMatrix));
	memcpy(&pCamera->_projectionMatrix, &param->projectionMatrix, sizeof(pCamera->_projectionMatrix));

	return true;
}

bool BlastPlugin::Camera_SaveParameters(void * ptr, Camera* pCamera)
{
	nvidia::parameterized::BlastProjectParametersNS::Camera_Type* outParam =
		static_cast<nvidia::parameterized::BlastProjectParametersNS::Camera_Type*>(ptr);

	outParam->flags = (pCamera->_zup ? 1 : 2);
	outParam->fov = pCamera->_fov;
	outParam->aspectRatio = pCamera->_aspectRatio;
	outParam->znear = pCamera->_znear;
	outParam->zfar = pCamera->_zfar;
	outParam->width = 0;
	outParam->height = 0;
	outParam->isPerspective = pCamera->_isPerspective;
	memcpy(&outParam->eye, &pCamera->_eye, sizeof(outParam->eye));
	memcpy(&outParam->at, &pCamera->_at, sizeof(outParam->at));
	outParam->lookDistance = pCamera->_lookDistance;
	memcpy(&outParam->orientation, &pCamera->_orientation, sizeof(outParam->orientation));
	memcpy(&outParam->viewMatrix, &pCamera->_viewMatrix, sizeof(outParam->viewMatrix));
	memcpy(&outParam->projectionMatrix, &pCamera->_projectionMatrix, sizeof(outParam->projectionMatrix));
	return true; 
}

bool BlastPlugin::Gamepad_ToggleSimulation()
{
	return true; 
}

bool BlastPlugin::Gamepad_LoadSamples(QString fn)
{
	return true; 
}

bool BlastPlugin::Gamepad_ResetScene()
{
	return true; 
}

bool BlastPlugin::Gamepad_StartAnimation()
{ 
	return true; 
}

bool BlastPlugin::GamepadHandler_ShowHair()
{ 
	return true;
}

bool BlastPlugin::GamepadHandler_SpinWindStrength(float windStrength)
{ 
	return true; 
}

bool BlastPlugin::Gamepad_ResetAnimation()
{ 
	return true;
}

bool BlastPlugin::Gamepad_PlayPauseAnimation()
{ 
	return true;
}

bool BlastPlugin::Light_loadParameters(NvParameterized::Handle& handle, Light* pLight)
{
	if (pLight == nullptr)
		return false;

	NvParameterized::NvParameters* params = static_cast<NvParameterized::NvParameters*>(handle.getInterface());
	size_t offset = 0;
	nvidia::parameterized::BlastProjectParametersNS::Light_Type* param = nullptr;
	params->getVarPtr(handle, (void*&)param, offset);

	pLight->m_enable = param->enable;
	pLight->m_useShadows = param->useShadows;
	pLight->m_visualize = param->visualize;
	pLight->m_intensity = param->intensity;

	memcpy(&pLight->m_color, &param->color, sizeof(atcore_float3));

	atcore_float3 axisX, axisY, axisZ, lightPos;
	memcpy(&axisX, &param->lightAxisX, sizeof(atcore_float3));
	memcpy(&axisY, &param->lightAxisY, sizeof(atcore_float3));
	memcpy(&axisZ, &param->lightAxisZ, sizeof(atcore_float3));
	memcpy(&lightPos, &param->lightPos, sizeof(atcore_float3));

	pLight->SetShadowMapResolution(param->shadowMapResolution);

	pLight->m_lightCamera.SetEye(lightPos);
	pLight->m_lightCamera.SetViewMatrix(axisX, axisY, axisZ);
	pLight->m_lightCamera.BuildViewMatrix();
	return true; 
}

bool BlastPlugin::Light_saveParameters(NvParameterized::Handle& handle, Light* pLight)
{
	if (pLight == nullptr)
		return false;

	NvParameterized::NvParameters* params = static_cast<NvParameterized::NvParameters*>(handle.getInterface());
	size_t offset = 0;
	nvidia::parameterized::BlastProjectParametersNS::Light_Type* param = nullptr;
	params->getVarPtr(handle, (void*&)param, offset);
	memset((void*)param, 0, sizeof(nvidia::parameterized::BlastProjectParametersNS::Light_Type));

	param->enable = pLight->m_enable;
	param->useShadows = pLight->m_useShadows;
	param->visualize = pLight->m_visualize;
	param->intensity = pLight->m_intensity;

	param->shadowMapResolution = pLight->m_shadowMapResolutionIndex;

	memcpy(&param->color, &pLight->m_color, sizeof(atcore_float3));

	{
		atcore_float3 axisX = pLight->m_lightCamera.GetXAxis();
		atcore_float3 axisY = pLight->m_lightCamera.GetYAxis();
		atcore_float3 axisZ = pLight->m_lightCamera.GetZAxis();
		atcore_float3 lightPos = pLight->m_lightCamera.GetEye();

		memcpy(&param->lightAxisX, &axisX, sizeof(atcore_float3));
		memcpy(&param->lightAxisY, &axisY, sizeof(atcore_float3));
		memcpy(&param->lightAxisZ, &axisZ, sizeof(atcore_float3));
		memcpy(&param->lightPos, &lightPos, sizeof(atcore_float3));
	}
	return true; 
}

bool BlastPlugin::SimpleScene_SimpleScene()
{ 
	return true; 
}


#include "Timer.h"
Timer g_fpsTimer;

#include "D3DWidget.h"
#include "SampleManager.h"

#include <QtWidgets/QShortcut>

bool BlastPlugin::SimpleScene_Initialize(int backdoor)
{
	if (!CreateProjectParamsContext())
		return false;

	D3DWidget* d3dWidget = AppMainWindow::Inst().GetRenderWidget();
	g_hWnd = (HWND)d3dWidget->winId();

	RECT rc;
	GetClientRect((HWND)g_hWnd, &rc);
	int wBuf = rc.right - rc.left;
	int hBuf = rc.bottom - rc.top;

	DeviceCreationParameters deviceParams;
	deviceParams.swapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	deviceParams.swapChainSampleCount = 4;
	deviceParams.startFullscreen = false;
	deviceParams.hWnd = g_hWnd;
	deviceParams.backBufferWidth = wBuf;
	deviceParams.backBufferHeight = hBuf;
#if defined(DEBUG) | defined(_DEBUG)
	deviceParams.createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
#endif
	deviceParams.featureLevel = D3D_FEATURE_LEVEL_11_0;

	DeviceManager* pDeviceManager = new DeviceManager;
	//pDeviceManager->CreateWindowDeviceAndSwapChain(deviceParams);
	RenderPlugin* pPlugin = RenderPlugin::Instance();
	D3DHandles handles;
	pDeviceManager->SetWindowHandle(g_hWnd);
	pDeviceManager->SetWindowDeviceAndSwapChain(pPlugin->GetDeviceHandles(handles));
	g_DeviceManagerInstance = pDeviceManager;
	SimpleScene::Inst()->SetDeviceManager(pDeviceManager);

	SampleManager* pSampleManager = new SampleManager(pDeviceManager);
	SimpleScene::Inst()->SetSampleManager(pSampleManager);

	// post build will copy samples\resources to bin\resources. If debug in VS, need set working directory to binary's folder

	pSampleManager->init();

	QShortcut* shortCut;

	shortCut = new QShortcut(QKeySequence(Qt::Key_K), d3dWidget);
	connect(shortCut, SIGNAL(activated()), this, SLOT(shortcut_damagetool()));

	shortCut = new QShortcut(QKeySequence(Qt::Key_Q), d3dWidget);
	connect(shortCut, SIGNAL(activated()), this, SLOT(shortcut_selecttool()));

	shortCut = new QShortcut(QKeySequence(Qt::Key_W), d3dWidget);
	connect(shortCut, SIGNAL(activated()), this, SLOT(shortcut_Translate()));
	shortCut = new QShortcut(QKeySequence(Qt::Key_E), d3dWidget);
	connect(shortCut, SIGNAL(activated()), this, SLOT(shortcut_Rotation()));
	shortCut = new QShortcut(QKeySequence(Qt::Key_R), d3dWidget);
	connect(shortCut, SIGNAL(activated()), this, SLOT(shortcut_Scale()));

	shortCut = new QShortcut(QKeySequence(Qt::Key_L), d3dWidget);
	connect(shortCut, SIGNAL(activated()), this, SLOT(shortcut_edittool()));
	
	_chunkContextMenu = new QMenu();
	action_Make_Support = new QAction(tr("Make Support"), d3dWidget);
	_chunkContextMenu->addAction(action_Make_Support);
	connect(action_Make_Support, SIGNAL(triggered()), this, SLOT(slot_Make_Support()));

	action_Make_Static_Support = new QAction(tr("Make Static Support"), d3dWidget);
	_chunkContextMenu->addAction(action_Make_Static_Support);
	connect(action_Make_Static_Support, SIGNAL(triggered()), this, SLOT(slot_Make_Static_Support()));

	action_Remove_Support = new QAction(tr("Remove Support"), d3dWidget);
	_chunkContextMenu->addAction(action_Remove_Support);
	connect(action_Remove_Support, SIGNAL(triggered()), this, SLOT(slot_Remove_Support()));

	_bondContextMenu = new QMenu();
	action_Bond_Chunks = new QAction(tr("Bond Chunks"), d3dWidget);
	_bondContextMenu->addAction(action_Bond_Chunks);
	connect(action_Bond_Chunks, SIGNAL(triggered()), this, SLOT(slot_Bond_Chunks()));

	action_Bond_Chunks_with_Joints = new QAction(tr("Bond Chunks With Joints"), d3dWidget);
	_bondContextMenu->addAction(action_Bond_Chunks_with_Joints);
	connect(action_Bond_Chunks_with_Joints, SIGNAL(triggered()), this, SLOT(slot_Bond_Chunks_with_Joints()));

	action_Remove_all_Bonds = new QAction(tr("Remove All Bonds"), d3dWidget);
	_bondContextMenu->addAction(action_Remove_all_Bonds);
	connect(action_Remove_all_Bonds, SIGNAL(triggered()), this, SLOT(slot_Remove_all_Bonds()));
	
	return true; 
}
bool BlastPlugin::SimpleScene_Shutdown()
{
	SampleManager& sm = SimpleScene::Inst()->GetSampleManager();
	sm.free();
	//SimpleScene::Inst()->SetSampleManager(NV_NULL);

	ReleaseProjectParamsContext();

	g_DeviceManagerInstance = NV_NULL;

	return true;
}

//#include "SceneController.h"
bool BlastPlugin::SimpleScene_Clear()
{
	SampleManager& sampleManager = SimpleScene::Inst()->GetSampleManager();
	sampleManager.clearScene();
//	SceneController& sceneController = sampleManager.getSceneController();
//	sceneController.ClearScene();

	BlastProject::ins().clear();

	return true; 
}

bool BlastPlugin::SimpleScene_Draw_DX12()
{ 	
	return true;
}

///////////////////////////////////////////////////////////////////////////////
#include <Stats.h>
namespace
{
	Stats				g_StatsCPU;
	Stats				g_StatsGPU;

	Stats				g_FrameStatsCPU;
	Stats				g_FrameStatsGPU;
}

//////////////////////////////////////////////////////////////////////////////
void BlastPlugin::ResetFrameTimer()
{
	g_StatsCPU.reset();
	g_StatsGPU.reset();
}

void BlastPlugin::DrawHUD()
{
	GlobalSettings& globalSettings = GlobalSettings::Inst();

	if (!globalSettings.m_showHUD)
		return;

	// finish frame timer
	g_FrameStatsCPU.add(g_StatsCPU);
	g_FrameStatsGPU.add(g_StatsGPU);

	wchar_t sz[1000];

	RenderInterface::TxtHelperBegin();
	RenderInterface::TxtHelperSetInsertionPos(2, 0);
	RenderInterface::TxtHelperSetForegroundColor(1.0f, 1.0f, 1.0f, 1.0f);

	if (RenderInterface::GetDeviceInfoString(sz))
		RenderInterface::TxtHelperDrawTextLine(sz);

	{
	//	const NvHair::BuildInfo& buildInfo = GetHairSDK()->getBuildInfo();

	//	char releaseVersion[NvHair::BuildInfo::VERSION_BUFFER_SIZE];
	//	buildInfo.m_versionToStringFunc(buildInfo.m_releaseVersion, releaseVersion);

		swprintf_s(sz, 1000, L"Lib build: %S Release version: %S\n", "n.a.", "1.0.0");
		RenderInterface::TxtHelperDrawTextLine(sz);
	}

	swprintf_s(sz, 1000, L"Current frame time %3.2f[%d-%d:%2.2f]\n",
		globalSettings.m_frameTime,
		(int)globalSettings.m_frameStartTime,
		(int)globalSettings.m_frameEndTime,
		globalSettings.m_animationSpeed);

	RenderInterface::TxtHelperDrawTextLine(sz);

	bool computeProfile = globalSettings.m_computeProfile;

	//FurCharacter& character = SimpleScene::Inst()->GetFurCharacter();
	//std::vector<HairInstance*> validInstances;
	//character.GetValidHairInstances(validInstances);

	if (globalSettings.m_computeStatistics && globalSettings.m_showHUD)
	{

		//for (int i = 0; i < validInstances.size(); i++)
		//{
		//	HairInstance* pHairInstance = validInstances[i];

		//	NvHair::Stats& hairStats = pHairInstance->m_hairStats;

		//	int totalHairs = hairStats.m_numHairs;
		//	int totalFaces = hairStats.m_numFaces;

		//	float averageNumCVsPerHair = hairStats.m_averageNumCvsPerHair;
		//	float averageDensity = hairStats.m_averageDensity;
		//	float averageHairsPerFace = hairStats.m_averageHairsPerFace;
		//	float distanceLODFactor = hairStats.m_distanceLodFactor;
		//	float detailLODFactor = hairStats.m_detailLodFactor;
		//	float camDistance = hairStats.m_camDistance;

		//	int m = pHairInstance->getDescriptor().m_splineMultiplier;
		//	float vertsPerHair = m * (averageNumCVsPerHair - 1.0f) + 1;
		//	float totalLines = totalHairs * vertsPerHair;

		//	const char* hairName = pHairInstance->m_assetName.c_str();
		//	std::wstring hairNameW;
		//	std::copy(hairName, hairName + strlen(hairName), back_inserter(hairNameW));

		//	swprintf_s(sz, 1000, L"Asset Name (%s), %d hairs, %d faces, %.1f lines",
		//		hairNameW.c_str(), totalHairs, totalFaces, totalLines);
		//	RenderInterface::TxtHelperDrawTextLine(sz);

		//	swprintf_s(sz, 1000, L"    average density %.2f, avg hairs per face %.2f, avg CVs per hair %2.1f, avg render vertices per hair %3.1f",
		//		averageDensity, averageHairsPerFace, averageNumCVsPerHair, vertsPerHair);
		//	RenderInterface::TxtHelperDrawTextLine(sz);

		//	swprintf_s(sz, 1000, L"    distance LOD Factor: %.2f, detail LOD Factor: %.2f, camera distance: %.2f",
		//		distanceLODFactor, detailLODFactor, camDistance);
		//	RenderInterface::TxtHelperDrawTextLine(sz);
		//}
	}

	if (globalSettings.m_showFPS)
	{
		static double fps = 0;
		static double lastFrames = GlobalSettings::Inst().m_renderFrameCnt;
		Timer& g_fpsTimer = SimpleScene::Inst()->GetTimer();
		static double lastFpsTime = g_fpsTimer.GetTimeInSeconds();
		static int numFrames = 1;

		static Stats lastStatsCPU;
		static Stats lastStatsGPU;

		Stats& statsGPU = g_FrameStatsGPU;
		Stats& statsCPU = g_FrameStatsCPU;

		double currentFpsTime = g_fpsTimer.GetTimeInSeconds();
		if ((currentFpsTime - lastFpsTime) > 1.0)
		{
			double currentFrames = GlobalSettings::Inst().m_renderFrameCnt;
			numFrames = currentFrames - lastFrames;

			fps = numFrames / (currentFpsTime - lastFpsTime);

			lastFrames = currentFrames;
			lastFpsTime = currentFpsTime;

			lastStatsGPU = statsGPU;
			lastStatsCPU = statsCPU;

			lastStatsGPU.average((float)numFrames);
			lastStatsCPU.average((float)numFrames);

			statsGPU.reset();
			statsCPU.reset();
		}

		if (globalSettings.m_computeProfile)
		{
			swprintf_s(sz, 1000, L"Render time (GPU/CPU): Total %.2f/%.2fms, Hair %.2f/%.2fms, Mesh %.2f/%.2fms, Shadow %.2f/%.2fms, Stats %.2f/%.2fms",
				lastStatsGPU.m_totalRenderTime, lastStatsCPU.m_totalRenderTime,
				lastStatsGPU.m_hairRenderTime, lastStatsCPU.m_hairRenderTime,
				lastStatsGPU.m_meshRenderTime, lastStatsCPU.m_meshRenderTime,
				lastStatsGPU.m_shadowRenderTime, lastStatsCPU.m_shadowRenderTime,
				lastStatsGPU.m_hairStatsTime, lastStatsCPU.m_hairStatsTime);

			RenderInterface::TxtHelperDrawTextLine(sz);

			swprintf_s(sz, 1000, L"Update time (GPU/CPU): Total %.2f/%.2fms, Hair Update %.2f/%.2fms, Mesh Skinning %.2f/%.2fms, Sim %.2f/%.2fms",
				lastStatsGPU.m_totalUpdateTime, lastStatsCPU.m_totalUpdateTime,
				lastStatsGPU.m_hairSkinningTime, lastStatsCPU.m_hairSkinningTime,
				lastStatsGPU.m_meshSkinningTime, lastStatsCPU.m_meshSkinningTime,
				lastStatsGPU.m_hairSimulationTime, lastStatsCPU.m_hairSimulationTime);
			RenderInterface::TxtHelperDrawTextLine(sz);

			{
				float cpuMeshTime = lastStatsCPU.m_meshRenderTime + lastStatsCPU.m_meshSkinningTime;
				float gpuMeshTime = lastStatsGPU.m_meshRenderTime + lastStatsGPU.m_meshSkinningTime;

				float cpuHairTime = lastStatsCPU.m_totalRenderTime + lastStatsCPU.m_totalUpdateTime - cpuMeshTime;
				float gpuHairTime = lastStatsGPU.m_totalRenderTime + lastStatsGPU.m_totalUpdateTime - gpuMeshTime;

				float gpuFPS = 1000.0f / gpuHairTime;
				RenderInterface::TxtHelperSetForegroundColor(0.2f, 1.0f, 0.2f, 1.0f);
				swprintf_s(sz, 1000, L"Hair: GPU Fps %.2f, GPU time %.2fms, CPU time %.2fms",
					gpuFPS, gpuHairTime, cpuHairTime);
				RenderInterface::TxtHelperDrawTextLine(sz);
			}

			float gpuTime = lastStatsGPU.m_totalRenderTime + lastStatsGPU.m_totalUpdateTime;
			float gpuFPS = 1000.0f / gpuTime;
			RenderInterface::TxtHelperSetForegroundColor(1.0f, 0.5f, 0.5f, 1.0f);
			swprintf_s(sz, 1000, L"Total: GPU Fps %.2f, GPU time %.2fms, CPU time %.2fms",
				gpuFPS,
				lastStatsGPU.m_totalRenderTime + lastStatsGPU.m_totalUpdateTime,
				lastStatsCPU.m_totalRenderTime + lastStatsCPU.m_totalUpdateTime
				);
			RenderInterface::TxtHelperDrawTextLine(sz);

		}
		else
		{
			swprintf_s(sz, 1000, L"Fps %.2f", fps);
			RenderInterface::TxtHelperDrawTextLine(sz);
		}
	}

	RenderInterface::TxtHelperEnd();
}

bool BlastPlugin::SimpleScene_Draw_DX11()
{
	BlastPlugin::DrawHUD();
	return true;
}

bool BlastPlugin::SimpleScene_FitCamera(atcore_float3& center, atcore_float3& extents)
{
	return true; 
}
bool BlastPlugin::SimpleScene_DrawGround()
{ 
	return true;
}
bool BlastPlugin::SimpleScene_DrawWind()
{ 
	return true;
}
bool BlastPlugin::SimpleScene_DrawAxis()
{ 
	return true; 
}

#include <Shlwapi.h>
#include <FbxUtil.h>
#include <MeshData.h>
#include <PxVec2.h>
#include <SourceAssetOpenDlg.h>
bool BlastPlugin::SimpleScene_LoadSceneFromFbx(const char* d, const char* f)
{
	SourceAssetOpenDlg dlg(false, &AppMainWindow::Inst());
	int res = dlg.exec();
	if (res != QDialog::Accepted || dlg.getFile().isEmpty())
		return false;

	QFileInfo fileInfo(dlg.getFile());
	std::string dir = QDir::toNativeSeparators(fileInfo.absoluteDir().absolutePath()).toLocal8Bit();
	std::string fbxName = fileInfo.fileName().toLocal8Bit();
	
	GlobalSettings& globalSettings = GlobalSettings::Inst();

	char fbxFilePath[MAX_PATH];

	float sceneUnit = globalSettings.getSceneUnitInCentimeters();

	PathCombineA(fbxFilePath, dir.c_str(), fbxName.c_str());

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

	SampleManager& sampleManager = SimpleScene::Inst()->GetSampleManager();

	int numMeshes = 0;
	char* meshNames = 0;
	char* skinned = 0;
	FbxUtil::GetMeshInfo(&numMeshes, &meshNames, &skinned);
	if (numMeshes > 1)
	{
		//return false;
	}

	// to do later when numMeshes is more than one
	numMeshes = 1;

	for (int i = 0; i < numMeshes; i++)
	{
		const char* meshName = meshNames + i * 128;

		MeshDesc meshDesc;
		FbxUtil::CreateMeshDescriptor(meshName, meshDesc);

		MeshMaterial* materials = 0;
		int numMaterials = 0;
		FbxUtil::GetMeshMaterials(meshName, &numMaterials, &materials);
		
		SkinData skinData;
		FbxUtil::InitializeSkinData(meshName, skinData);

		meshDesc.UpdateNormal(false);

		std::vector<physx::PxVec3> positions;
		std::vector<physx::PxVec3> normals;
		std::vector<physx::PxVec2> uv;
		std::vector<unsigned int>  indices;

		for (uint32_t i = 0; i < meshDesc.m_NumVertices; ++i)
		{
			atcore_float3 pos = meshDesc.m_pVertices[i];
			atcore_float3 vertexNormal = meshDesc.m_pVertexNormals[i];
			atcore_float2 texcoord = meshDesc.m_pTexCoords[i];

			positions.push_back(physx::PxVec3(pos.x, pos.y, pos.z));
			normals.push_back(physx::PxVec3(vertexNormal.x, vertexNormal.y, vertexNormal.z));
			uv.push_back(physx::PxVec2(texcoord.x, texcoord.y));
		}

		for (uint32_t i = 0; i < meshDesc.m_NumTriangles; ++i)
		{
			indices.push_back(meshDesc.m_pIndices[i * 3 + 0]);
			indices.push_back(meshDesc.m_pIndices[i * 3 + 1]);
			indices.push_back(meshDesc.m_pIndices[i * 3 + 2]);
		}

		sampleManager.createAsset(meshName, positions, normals, uv, indices);

		physx::PxTransform t(physx::PxIdentity);
		{
			QVector3D Position = dlg.getPosition();
			t.p = physx::PxVec3(Position.x(), Position.y(), Position.z());

			QVector3D RotationAxis = dlg.getRotationAxis();
			physx::PxVec3 Axis = physx::PxVec3(RotationAxis.x(), RotationAxis.y(), RotationAxis.z());
			Axis = Axis.getNormalized();
			float RotationDegree = dlg.getRotationDegree();
			float DEGREE_TO_RAD = acos(-1.0) / 180.0;
			RotationDegree = RotationDegree * DEGREE_TO_RAD;
			t.q = physx::PxQuat(RotationDegree, Axis);
		}
		sampleManager.addModelAsset(meshName, dlg.getSkinned(), t, !dlg.isAppend());
	}

	FbxUtil::Release();

	globalSettings.m_sceneLoaded = true;
	globalSettings.m_animationIndex = 1;
	globalSettings.m_firstFrame = true;

	globalSettings.resetAnimation();

	return true; 
}

bool BlastPlugin::SimpleScene_LoadProject(const char* dir, const char* file)
{
	GlobalSettings& globalSettings = GlobalSettings::Inst();
	SimpleScene* pScene = SimpleScene::Inst();
	nvidia::parameterized::BlastProjectParametersNS::ParametersStruct params;
	if (!ProjectParamsLoad(globalSettings.getAbsoluteFilePath().c_str(), pScene))
		return false;

	return true;
}
bool BlastPlugin::SimpleScene_SaveProject(const char* dir, const char* file)
{ 
	GlobalSettings& globalSettings = GlobalSettings::Inst();
	SimpleScene* pScene = SimpleScene::Inst();

	QString saveFilePath = globalSettings.getAbsoluteFilePath().c_str();
	QString tempFilePath = utils::GetTempFilePath().c_str();
	if (ProjectParamsSave(tempFilePath.toUtf8().data(), pScene))
	{
		if (!utils::RenameFile(tempFilePath.toUtf8().data(), saveFilePath.toUtf8().data(), true /* overwrite */))
		{
			return false;
		}
		pScene->SetProjectModified(false);
		return true;
	}

	return false;
}
bool BlastPlugin::SimpleScene_LoadParameters(NvParameterized::Interface* iface)
{ 
	nvidia::parameterized::BlastProjectParameters* params = static_cast<nvidia::parameterized::BlastProjectParameters*>(iface);

	nvidia::parameterized::BlastProjectParametersNS::ParametersStruct& srcDesc = params->parameters();
	copy(BlastProject::ins().getParams(), srcDesc);

	for (int i = 0; i < srcDesc.blast.blastAssets.arraySizes[0]; ++i)
	{
		BPPAsset& asset = srcDesc.blast.blastAssets.buf[i];
		QFileInfo fileInfo(asset.path.buf);
		QByteArray tmp = fileInfo.baseName().toUtf8();
		const char* fileName = tmp.data();
		physx::PxTransform t(physx::PxIdentity);
		BPPAssetInstance* instance = BlastProject::ins().getAssetInstance(asset.path.buf, 0);
		if (instance != nullptr)
		{
			nvidia::NvVec3& postion = instance->transform.position;
			nvidia::NvVec4& rotation = instance->transform.rotation;
			t.p = physx::PxVec3(postion.x, postion.y, postion.z);
			t.q = physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w);
		}
		SimpleScene::Inst()->GetSampleManager().addModelAsset(fileName, false, t, true);
	}

	SimpleScene* pScene = SimpleScene::Inst();

	if (pScene->m_pCamera)
	{
		pScene->m_pCamera->LoadParameters(&srcDesc.camera);
	}

	pScene->LoadCameraBookmarks(iface);

	if (pScene->m_pWindCamera)
	{
		pScene->m_pWindCamera->LoadParameters(&srcDesc.windCamera);
	}

	GlobalSettings& globalSettings = GlobalSettings::Inst();

	// Load scene settings
	globalSettings.m_repeatAnimation = srcDesc.scene.repeatAnimation;
	globalSettings.m_animationSpeed = srcDesc.scene.animationSpeed;
	globalSettings.m_showGrid = srcDesc.scene.showGrid;
	globalSettings.m_showAxis = srcDesc.scene.showAxis;
	globalSettings.m_zup = srcDesc.scene.upAxis == 1;
	globalSettings.m_sceneUnitIndex = srcDesc.scene.sceneUnitIndex;

	// Load render settings
	NvParameterized::Handle handle(iface);
	if (iface->getParameterHandle("renderer", handle) == NvParameterized::ERROR_NONE)
	{
		globalSettings.m_computeStatistics = srcDesc.renderer.showStatistics;
		globalSettings.m_showGraphicsMesh = srcDesc.renderer.showGraphicsMesh;
		globalSettings.m_useLighting = srcDesc.renderer.useLighting;
		globalSettings.m_showSkinnedMeshOnly = srcDesc.renderer.showSkinnedMeshOnly;
		globalSettings.m_renderFps = srcDesc.renderer.renderFps;
		globalSettings.m_simulationFps = srcDesc.renderer.simulationFps;

		Light::LoadParameters(handle);
	}

	//// Load fbx paths
	//if (iface->getParameterHandle("fbxFilePaths", handle) == NvParameterized::ERROR_NONE)
	//{
	//	int arraySize;
	//	handle.getArraySize(arraySize);
	//	char** strArray = new char*[arraySize];
	//	handle.getParamStringArray(strArray, arraySize);
	//	for (int idx = 0; idx < arraySize; ++idx)
	//	{
	//		pScene->LoadSceneFromFbx(
	//			globalSettings.m_projectFileDir.c_str(),
	//			strArray[idx]);
	//	}
	//	delete[] strArray;
	//}

	//// get fur character mesh setting 
	//if (false == pScene->GetFurCharacter().LoadMeshParameters(handle))
	//	return false;

	//// Load apx paths (hair instances)
	//if (iface->getParameterHandle("apxFilePaths", handle) == NvParameterized::ERROR_NONE)
	//{
	//	if (false == pScene->GetFurCharacter().LoadHairParameters(handle))
	//		return false;
	//}

	BlastProject::ins().loadUserPreset();
	return true;
}
bool BlastPlugin::SimpleScene_SaveParameters(NvParameterized::Interface* iface)
{
	nvidia::parameterized::BlastProjectParameters* params = static_cast<nvidia::parameterized::BlastProjectParameters*>(iface);
	nvidia::parameterized::BlastProjectParametersNS::ParametersStruct& targetDesc = params->parameters();
	memset(&targetDesc, sizeof(BPParams), 0);
	BPParams& srcParams = BlastProject::ins().getParams();
	copy(targetDesc, srcParams);

	SimpleScene* pScene = SimpleScene::Inst();

	if (pScene->m_pCamera)
		pScene->m_pCamera->SaveParameters(&targetDesc.camera);

	pScene->SaveCameraBookmarks(iface);

	if (pScene->m_pWindCamera)
		pScene->m_pWindCamera->SaveParameters(&targetDesc.windCamera);

	// Save scene settings
	GlobalSettings& globalSettings = GlobalSettings::Inst();

	targetDesc.scene.repeatAnimation = globalSettings.m_repeatAnimation;
	targetDesc.scene.animationSpeed = globalSettings.m_animationSpeed;
	targetDesc.scene.showGrid = globalSettings.m_showGrid;
	targetDesc.scene.showAxis = globalSettings.m_showAxis;
	targetDesc.scene.upAxis = (globalSettings.m_zup) ? 1 : 2;
	targetDesc.scene.sceneUnitIndex = globalSettings.m_sceneUnitIndex;

	// Save render settings
	NvParameterized::Handle handle(iface);
	if (iface->getParameterHandle("renderer", handle) == NvParameterized::ERROR_NONE)
	{
		targetDesc.renderer.renderFps = globalSettings.m_renderFps;
		targetDesc.renderer.simulationFps = globalSettings.m_simulationFps;
		targetDesc.renderer.showStatistics = globalSettings.m_computeStatistics;
		targetDesc.renderer.showGraphicsMesh = globalSettings.m_showGraphicsMesh;
		targetDesc.renderer.useLighting = globalSettings.m_useLighting;
		targetDesc.renderer.showSkinnedMeshOnly = globalSettings.m_showSkinnedMeshOnly;

		Light::SaveParameters(handle);
	}

	//if (iface->getParameterHandle("renderer.textureFilePath", handle) == NvParameterized::ERROR_NONE)
	//{
	//	std::string textureFilePath = globalSettings.getRelativePath(globalSettings.m_backgroundTextureFilePath.c_str());
	//	handle.setParamString(textureFilePath.c_str());
	//}

	//// save hair path
	//if (iface->getParameterHandle("apxFilePaths", handle) == NvParameterized::ERROR_NONE)
	//	character.SaveHairParameters(handle);

	return true; 
}

bool BlastPlugin::SimpleScene_LoadCameraBookmarks(NvParameterized::Interface* iface)
{
	return true;
}

bool BlastPlugin::SimpleScene_SaveCameraBookmarks(NvParameterized::Interface* iface)
{
	SimpleScene* pScene = SimpleScene::Inst();

	nvidia::parameterized::BlastProjectParameters* params = static_cast<nvidia::parameterized::BlastProjectParameters*>(iface);
	nvidia::parameterized::BlastProjectParametersNS::ParametersStruct& srcDesc = params->parameters();
	nvidia::parameterized::BlastProjectParametersNS::CameraBookmark_DynamicArray1D_Type& bookmarks = srcDesc.cameraBookmarks;

	NvParameterized::Handle cameraBookmarksHandle(iface);
	if (iface->getParameterHandle("cameraBookmarks", cameraBookmarksHandle) != NvParameterized::ERROR_NONE)
		return false;

	int numCameraBookmarks = pScene->m_cameraBookmarks.size();
	cameraBookmarksHandle.resizeArray(numCameraBookmarks);

	for (int idx = 0; idx < numCameraBookmarks; ++idx)
	{
		NvParameterized::Handle cameraBookmarkHandle(cameraBookmarksHandle);
		if (cameraBookmarksHandle.getChildHandle(idx, cameraBookmarkHandle) == NvParameterized::ERROR_NONE)
		{
			NvParameterized::Handle tempHandle(cameraBookmarkHandle);
			CameraBookmark& bookmark = pScene->m_cameraBookmarks[idx];

			if (ParamGetChild(cameraBookmarkHandle, tempHandle, "name"))
			{
				tempHandle.setParamString(bookmark.name.toStdString().c_str());
			}

			bookmark.camera.SaveParameters((void*)&(bookmarks.buf[idx].camera));
		}
	}

	return true;
}

bool BlastPlugin::D3DWidget_resizeEvent(QResizeEvent* e)
{ 
	DeviceManager& deviceManager = SimpleScene::Inst()->GetDeviceManager();

	int w = e->size().width();
	int h = e->size().height();
	WPARAM wParam = MAKEWPARAM(w, h);
	LPARAM lParam = MAKELPARAM(w, h);
	deviceManager.MsgProc(g_hWnd, WM_SIZE, wParam, lParam);

	return true; 
}

bool BlastPlugin::D3DWidget_paintEvent(QPaintEvent* e)
{
	SampleManager& pSampleManager = SimpleScene::Inst()->GetSampleManager();
	pSampleManager.run(); 

	if (pSampleManager.m_bNeedRefreshTree)
	{
		_blastSceneTree->updateValues();
		pSampleManager.m_bNeedRefreshTree = false;
	}

	return true;
}

bool BlastPlugin::D3DWidget_mousePressEvent(QMouseEvent* e)
{
	UINT uMsg = WM_LBUTTONDOWN;
	if (e->button() == Qt::RightButton)
	{
		uMsg = WM_RBUTTONDOWN;
	}
	else if (e->button() == Qt::MidButton)
	{
		uMsg = WM_MBUTTONDOWN;
	}	

	int x = e->x();
	int y = e->y();
	WPARAM wParam = MAKEWPARAM(x, y);
	LPARAM lParam = MAKELPARAM(x, y);
	
	DeviceManager& deviceManager = SimpleScene::Inst()->GetDeviceManager();
	deviceManager.MsgProc(g_hWnd, uMsg, wParam, lParam);

	return true;
}

bool BlastPlugin::D3DWidget_mouseReleaseEvent(QMouseEvent* e)
{
	UINT uMsg = WM_LBUTTONUP;
	if (e->button() == Qt::RightButton)
	{
		uMsg = WM_RBUTTONUP;
	}
	else if (e->button() == Qt::MidButton)
	{
		uMsg = WM_MBUTTONUP;
	}

	int x = e->x();
	int y = e->y();
	WPARAM wParam = MAKEWPARAM(x, y);
	LPARAM lParam = MAKELPARAM(x, y);

	DeviceManager& deviceManager = SimpleScene::Inst()->GetDeviceManager();
	deviceManager.MsgProc(g_hWnd, uMsg, wParam, lParam);

	return true;
}

bool BlastPlugin::D3DWidget_mouseMoveEvent(QMouseEvent* e)
{
	DeviceManager& deviceManager = SimpleScene::Inst()->GetDeviceManager();

	int x = e->x();
	int y = e->y();
	WPARAM wParam = MAKEWPARAM(x, y);
	LPARAM lParam = MAKELPARAM(x, y);
	deviceManager.MsgProc(g_hWnd, WM_MOUSEMOVE, wParam, lParam);

	return true;
}

bool BlastPlugin::D3DWidget_wheelEvent(QWheelEvent * e)
{
	DeviceManager& deviceManager = SimpleScene::Inst()->GetDeviceManager();

	int delta = e->delta();
	WPARAM wParam = MAKEWPARAM(delta, delta);
	LPARAM lParam = MAKELPARAM(delta, delta);
	deviceManager.MsgProc(g_hWnd, WM_MOUSEWHEEL, wParam, lParam);

	return true;
}

bool BlastPlugin::D3DWidget_keyPressEvent(QKeyEvent* e)
{ return true; }
bool BlastPlugin::D3DWidget_keyReleaseEvent(QKeyEvent* e)
{ return true; }
bool BlastPlugin::D3DWidget_dragEnterEvent(QDragEnterEvent *e)
{ return true; }
bool BlastPlugin::D3DWidget_dragMoveEvent(QDragMoveEvent *e)
{ return true; }
bool BlastPlugin::D3DWidget_dragLeaveEvent(QDragLeaveEvent *e)
{ return true; }
bool BlastPlugin::D3DWidget_dropEvent(QDropEvent *e)
{ return true; }

bool BlastPlugin::D3DWidget_contextMenuEvent(QContextMenuEvent *e)
{
	QPoint pos = QCursor::pos();

	std::map<BlastAsset*, std::vector<uint32_t>> selectedAssetChunks = SampleManager::ins()->getSelectedChunks();
	if (1 == selectedAssetChunks.size())
	{
		std::map<BlastAsset*, std::vector<uint32_t>>::iterator itr = selectedAssetChunks.begin();
		BlastAsset* asset = itr->first;
		std::vector<uint32_t> selectChunks = itr->second;

		std::vector<BlastChunkNode*> chunkNodes = BlastTreeData::ins().getChunkNodeByBlastChunk(asset, selectChunks);
		if (1 == chunkNodes.size())
		{
			_chunkContextMenu->exec(pos);
		}
		else if (1 < chunkNodes.size())
		{
			bool allSupportChunk = true;
			for (size_t i = 0; i < chunkNodes.size(); ++i)
			{
				BlastChunkNode* chunkNode = chunkNodes[i];
				if (eChunk != chunkNode->getType())
				{
					allSupportChunk = false;
					break;
				}
			}

			if (allSupportChunk)
			{
				_bondContextMenu->exec(QCursor::pos());
			}
		}
	}
	e->accept();
	return true;
}

#ifdef NV_ARTISTTOOLS
bool BlastPlugin::D3D11Shaders_InitializeShadersD3D11(std::map<int, D3D11RenderShader*>& ShaderMap)
{
	return PluginBlast::Instance()->D3D11Shaders_InitializeShadersD3D11(ShaderMap);
}
#endif // NV_ARTISTTOOLS

bool BlastPlugin::AppMainWindow_AppMainWindow()
{ 
	_mainToolbar = 0;
	_materialLibraryPanel = 0;
	_materialAssignmentsPanel = 0;
	_fileReferencesPanel = 0;
	_generalPanel = 0;
	_defaultDamagePanel = 0;
	_fractureCutoutSettingsPanel = 0;
	_fractureGeneralPanel = 0;
	_fractureShellCutSettingPanel = 0;
	_fractureSliceSettingsPanel = 0;
	_fractureVisualizersPanel = 0;
	_fractureVoronoiSettingsPanel = 0;
	_supportPanel = 0;
	_blastSceneTree = 0;
	_filtersDockWidget = 0;

	return true; 
}

bool BlastPlugin::AppMainWindow_InitMenuItems(QMenuBar* pMenuBar)
{
	QMenu *pMenu = pMenuBar->addMenu("&Blast");
	QAction *act;

	act = new QAction("Open project file", this);
	act->setShortcut(QKeySequence::Open);
	connect(act, SIGNAL(triggered()), this, SLOT(menu_openProject()));
	pMenu->addAction(act);

	act = new QAction("Save project file", this);
	act->setShortcut(QKeySequence::Save);
	connect(act, SIGNAL(triggered()), this, SLOT(menu_saveProject()));
	pMenu->addAction(act);

	act = new QAction("Save project file as...", this);
	connect(act, SIGNAL(triggered()), this, SLOT(menu_saveProjectAs()));
	pMenu->addAction(act);

	pMenu->addSeparator();

	return true;
}

bool BlastPlugin::AppMainWindow_InitMainTab(QWidget *displayScrollAreaContents, QVBoxLayout *displayScrollAreaLayout, int idx)
{
	ExpandablePanel* panel = new ExpandablePanel(displayScrollAreaContents);
	displayScrollAreaLayout->insertWidget(idx++, panel);
	panel->SetTitle("Display Mesh Materials");

	return true;
}

bool BlastPlugin::AppMainWindow_InitPluginTab(QTabWidget* sideBarTab)
{
	{
		QWidget *tabMaterial;
		QGridLayout *gridLayoutMaterial;
		QFrame *materialEditorArea;
		QVBoxLayout *materialEditorAreaLayout;
		QScrollArea *materialScrollArea;
		QWidget *materialScrollAreaContents;
		QVBoxLayout *materialScrollAreaLayout;
		QSpacerItem *verticalSpacerMaterial;

		tabMaterial = new QWidget();
		tabMaterial->setObjectName(QStringLiteral("tabMaterial"));
		gridLayoutMaterial = new QGridLayout(tabMaterial);
		gridLayoutMaterial->setSpacing(6);
		gridLayoutMaterial->setContentsMargins(11, 11, 11, 11);
		gridLayoutMaterial->setObjectName(QStringLiteral("gridLayoutMaterial"));
		gridLayoutMaterial->setContentsMargins(0, 0, 0, 0);
		materialEditorArea = new QFrame(tabMaterial);
		materialEditorArea->setObjectName(QStringLiteral("materialEditorArea"));
		materialEditorAreaLayout = new QVBoxLayout(materialEditorArea);
		materialEditorAreaLayout->setSpacing(6);
		materialEditorAreaLayout->setContentsMargins(11, 11, 11, 11);
		materialEditorAreaLayout->setObjectName(QStringLiteral("materialEditorAreaLayout"));
		materialEditorAreaLayout->setContentsMargins(2, 2, 2, 2);

		gridLayoutMaterial->addWidget(materialEditorArea, 1, 0, 1, 1);

		materialScrollArea = new QScrollArea(tabMaterial);
		materialScrollArea->setObjectName(QStringLiteral("materialScrollArea"));
		materialScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		materialScrollArea->setWidgetResizable(true);
		materialScrollAreaContents = new QWidget();
		materialScrollAreaContents->setObjectName(QStringLiteral("materialScrollAreaContents"));
		materialScrollAreaContents->setGeometry(QRect(0, 0, 359, 481));
		materialScrollAreaLayout = new QVBoxLayout(materialScrollAreaContents);
		materialScrollAreaLayout->setSpacing(3);
		materialScrollAreaLayout->setContentsMargins(11, 11, 11, 11);
		materialScrollAreaLayout->setObjectName(QStringLiteral("materialScrollAreaLayout"));
		materialScrollAreaLayout->setContentsMargins(2, 2, 2, 2);
		verticalSpacerMaterial = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

		materialScrollAreaLayout->addItem(verticalSpacerMaterial);

		materialScrollArea->setWidget(materialScrollAreaContents);

		gridLayoutMaterial->addWidget(materialScrollArea, 0, 0, 1, 1);

		sideBarTab->addTab(tabMaterial, QString());

		sideBarTab->setTabText(sideBarTab->indexOf(tabMaterial), QApplication::translate("AppMainWindowClass", "Materials", 0));

		ExpandablePanel* panel = 0;
		int pannelCnt = 0;

		panel = new ExpandablePanel(materialScrollAreaContents);
		_materialLibraryPanel = new MaterialLibraryPanel(panel);
		panel->AddContent(_materialLibraryPanel);
		materialScrollAreaLayout->insertWidget(pannelCnt++, panel);
		panel->SetTitle("Material Library");

		panel = new ExpandablePanel(materialScrollAreaContents);
		_materialAssignmentsPanel = new MaterialAssignmentsPanel(panel);
		panel->AddContent(_materialAssignmentsPanel);
		materialScrollAreaLayout->insertWidget(pannelCnt++, panel);
		panel->SetTitle("Material Assignments");
	}

	{
		QWidget *tabBlast;
		QGridLayout *gridLayout;
		QFrame *blastMaterialEditorArea;
		QVBoxLayout *blastMaterialEditorAreaLayout;
		QScrollArea *blastScrollArea;
		QWidget *blastScrollAreaContents;
		QVBoxLayout *blastScrollAreaLayout;
		QSpacerItem *verticalSpacer;

		tabBlast = new QWidget();
		tabBlast->setObjectName(QStringLiteral("tabBlast"));
		gridLayout = new QGridLayout(tabBlast);
		gridLayout->setSpacing(6);
		gridLayout->setContentsMargins(11, 11, 11, 11);
		gridLayout->setObjectName(QStringLiteral("gridLayout"));
		gridLayout->setContentsMargins(0, 0, 0, 0);
		blastMaterialEditorArea = new QFrame(tabBlast);
		blastMaterialEditorArea->setObjectName(QStringLiteral("blastMaterialEditorArea"));
		blastMaterialEditorAreaLayout = new QVBoxLayout(blastMaterialEditorArea);
		blastMaterialEditorAreaLayout->setSpacing(6);
		blastMaterialEditorAreaLayout->setContentsMargins(11, 11, 11, 11);
		blastMaterialEditorAreaLayout->setObjectName(QStringLiteral("blastMaterialEditorAreaLayout"));
		blastMaterialEditorAreaLayout->setContentsMargins(2, 2, 2, 2);

		gridLayout->addWidget(blastMaterialEditorArea, 1, 0, 1, 1);

		blastScrollArea = new QScrollArea(tabBlast);
		blastScrollArea->setObjectName(QStringLiteral("blastScrollArea"));
		blastScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		blastScrollArea->setWidgetResizable(true);
		blastScrollAreaContents = new QWidget();
		blastScrollAreaContents->setObjectName(QStringLiteral("blastScrollAreaContents"));
		blastScrollAreaContents->setGeometry(QRect(0, 0, 359, 481));
		blastScrollAreaLayout = new QVBoxLayout(blastScrollAreaContents);
		blastScrollAreaLayout->setSpacing(3);
		blastScrollAreaLayout->setContentsMargins(11, 11, 11, 11);
		blastScrollAreaLayout->setObjectName(QStringLiteral("blastScrollAreaLayout"));
		blastScrollAreaLayout->setContentsMargins(2, 2, 2, 2);
		verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

		blastScrollAreaLayout->addItem(verticalSpacer);

		blastScrollArea->setWidget(blastScrollAreaContents);

		gridLayout->addWidget(blastScrollArea, 0, 0, 1, 1);

		sideBarTab->addTab(tabBlast, QString());

		sideBarTab->setTabText(sideBarTab->indexOf(tabBlast), QApplication::translate("AppMainWindowClass", "Blast", 0));

		ExpandablePanel* panel = 0;
		int pannelCnt = 0;

		panel = new ExpandablePanel(blastScrollAreaContents);
		_fileReferencesPanel = new FileReferencesPanel(panel);
		panel->AddContent(_fileReferencesPanel);
		blastScrollAreaLayout->insertWidget(pannelCnt++, panel);
		panel->SetTitle("File References");

		panel = new ExpandablePanel(blastScrollAreaContents);
		_generalPanel = new GeneralPanel(panel);
		panel->AddContent(_generalPanel);
		blastScrollAreaLayout->insertWidget(pannelCnt++, panel);
		panel->SetTitle("General");

		panel = new ExpandablePanel(blastScrollAreaContents);
		_supportPanel = new SupportPanel(panel);
		panel->AddContent(_supportPanel);
		blastScrollAreaLayout->insertWidget(pannelCnt++, panel);
		panel->SetTitle("Support");

		panel = new ExpandablePanel(blastScrollAreaContents);
		_blastCompositePanel = new BlastCompositePanel(panel);
		panel->AddContent(_blastCompositePanel);
		blastScrollAreaLayout->insertWidget(pannelCnt++, panel);
		panel->SetTitle("Blast Composite");

		panel = new ExpandablePanel(blastScrollAreaContents);
		_fractureGeneralPanel = new FractureGeneralPanel(panel);
		panel->AddContent(_fractureGeneralPanel);
		blastScrollAreaLayout->insertWidget(pannelCnt++, panel);
		panel->SetTitle("Fracture General");

		panel = new ExpandablePanel(blastScrollAreaContents);
		_fractureCutoutSettingsPanel = new FractureCutoutSettingsPanel(panel);
		panel->AddContent(_fractureCutoutSettingsPanel);
		blastScrollAreaLayout->insertWidget(pannelCnt++, panel);
		panel->SetTitle("Cutout Projection Settings");

		panel = new ExpandablePanel(blastScrollAreaContents);
		_fractureShellCutSettingPanel = new FractureShellCutSettingsPanel(panel);
		panel->AddContent(_fractureShellCutSettingPanel);
		blastScrollAreaLayout->insertWidget(pannelCnt++, panel);
		panel->SetTitle("Shell Cut Settings");

		panel = new ExpandablePanel(blastScrollAreaContents);
		_fractureSliceSettingsPanel = new FractureSliceSettingsPanel(panel);
		panel->AddContent(_fractureSliceSettingsPanel);
		blastScrollAreaLayout->insertWidget(pannelCnt++, panel);
		panel->SetTitle("Slice Settings");

		panel = new ExpandablePanel(blastScrollAreaContents);
		_fractureVoronoiSettingsPanel = new FractureVoronoiSettingsPanel(panel);
		panel->AddContent(_fractureVoronoiSettingsPanel);
		blastScrollAreaLayout->insertWidget(pannelCnt++, panel);
		panel->SetTitle("Voronoi Settings");

		panel = new ExpandablePanel(blastScrollAreaContents);
		_fractureVisualizersPanel = new FractureVisualizersPanel(panel);
		panel->AddContent(_fractureVisualizersPanel);
		blastScrollAreaLayout->insertWidget(pannelCnt++, panel);
		panel->SetTitle("Visualizers");
	}

	{
		QWidget *tabDamage;
		QGridLayout *gridLayoutDamage;
		QFrame *damageEditorArea;
		QVBoxLayout *damageEditorAreaLayout;
		QScrollArea *damageScrollArea;
		QWidget *damageScrollAreaContents;
		QVBoxLayout *damageScrollAreaLayout;
		QSpacerItem *verticalSpacerDamage;

		tabDamage = new QWidget();
		tabDamage->setObjectName(QStringLiteral("tabDamage"));
		gridLayoutDamage = new QGridLayout(tabDamage);
		gridLayoutDamage->setSpacing(6);
		gridLayoutDamage->setContentsMargins(11, 11, 11, 11);
		gridLayoutDamage->setObjectName(QStringLiteral("gridLayoutDamage"));
		gridLayoutDamage->setContentsMargins(0, 0, 0, 0);
		damageEditorArea = new QFrame(tabDamage);
		damageEditorArea->setObjectName(QStringLiteral("damageEditorArea"));
		damageEditorAreaLayout = new QVBoxLayout(damageEditorArea);
		damageEditorAreaLayout->setSpacing(6);
		damageEditorAreaLayout->setContentsMargins(11, 11, 11, 11);
		damageEditorAreaLayout->setObjectName(QStringLiteral("damageEditorAreaLayout"));
		damageEditorAreaLayout->setContentsMargins(2, 2, 2, 2);

		gridLayoutDamage->addWidget(damageEditorArea, 1, 0, 1, 1);

		damageScrollArea = new QScrollArea(tabDamage);
		damageScrollArea->setObjectName(QStringLiteral("damageScrollArea"));
		damageScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		damageScrollArea->setWidgetResizable(true);
		damageScrollAreaContents = new QWidget();
		damageScrollAreaContents->setObjectName(QStringLiteral("damageScrollAreaContents"));
		damageScrollAreaContents->setGeometry(QRect(0, 0, 359, 481));
		damageScrollAreaLayout = new QVBoxLayout(damageScrollAreaContents);
		damageScrollAreaLayout->setSpacing(3);
		damageScrollAreaLayout->setContentsMargins(11, 11, 11, 11);
		damageScrollAreaLayout->setObjectName(QStringLiteral("damageScrollAreaLayout"));
		damageScrollAreaLayout->setContentsMargins(2, 2, 2, 2);
		verticalSpacerDamage = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

		damageScrollAreaLayout->addItem(verticalSpacerDamage);

		damageScrollArea->setWidget(damageScrollAreaContents);

		gridLayoutDamage->addWidget(damageScrollArea, 0, 0, 1, 1);

		sideBarTab->addTab(tabDamage, QString());

		sideBarTab->setTabText(sideBarTab->indexOf(tabDamage), QApplication::translate("AppMainWindowClass", "Damage", 0));

		ExpandablePanel* panel = 0;
		int pannelCnt = 0;

		panel = new ExpandablePanel(damageScrollAreaContents);
		_defaultDamagePanel = new DefaultDamagePanel(panel);
		panel->AddContent(_defaultDamagePanel);
		damageScrollAreaLayout->insertWidget(pannelCnt++, panel);
		panel->SetTitle("Default Damage");
	}

	return true;
}

bool BlastPlugin::AppMainWindow_InitUI()
{
	AppMainWindow* mainWindow = &AppMainWindow::Inst();
	_mainToolbar = new BlastToolbar(mainWindow);
	_mainToolbar->setAllowedAreas(Qt::TopDockWidgetArea);
	_mainToolbar->setFeatures(_mainToolbar->features()&~QDockWidget::DockWidgetClosable);
	mainWindow->addDockWidget(Qt::TopDockWidgetArea, _mainToolbar);

	_filtersDockWidget = new FiltersDockWidget(mainWindow);
	_filtersDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea);
	_filtersDockWidget->setFeatures(_filtersDockWidget->features()&~QDockWidget::DockWidgetClosable);
	mainWindow->addDockWidget(Qt::LeftDockWidgetArea, _filtersDockWidget);

	_blastSceneTree = new BlastSceneTree(mainWindow);
	_blastSceneTree->setAllowedAreas(Qt::LeftDockWidgetArea);
	_blastSceneTree->setFeatures(_blastSceneTree->features()&~QDockWidget::DockWidgetClosable);
	_blastSceneTree->addObserver(_defaultDamagePanel);
	mainWindow->addDockWidget(Qt::LeftDockWidgetArea, _blastSceneTree);
	return true;
}

bool BlastPlugin::AppMainWindow_updateUI()
{
	if (_mainToolbar)
		_mainToolbar->updateValues();

	if (_filtersDockWidget)
		_filtersDockWidget->updateValues();

	if (_blastSceneTree)
		_blastSceneTree->updateValues();

	if (_materialLibraryPanel)
		_materialLibraryPanel->updateValues();

	if (_materialAssignmentsPanel)
		_materialAssignmentsPanel->updateValues();

	if (_fileReferencesPanel)
		_fileReferencesPanel->updateValues();

	if (_generalPanel)
		_generalPanel->updateValues();

	if (_blastCompositePanel)
		_blastCompositePanel->updateValues();

	if (_fractureCutoutSettingsPanel)
		_fractureCutoutSettingsPanel->updateValues();

	if (_fractureGeneralPanel)
		_fractureGeneralPanel->updateValues();

	if (_fractureShellCutSettingPanel)
		_fractureShellCutSettingPanel->updateValues();

	if (_fractureSliceSettingsPanel)
		_fractureSliceSettingsPanel->updateValues();

	if (_fractureVisualizersPanel)
		_fractureVisualizersPanel->updateValues();

	if (_fractureVoronoiSettingsPanel)
		_fractureVoronoiSettingsPanel->updateValues();

	if (_supportPanel)
		_supportPanel->updateValues();

	if (_defaultDamagePanel)
		_defaultDamagePanel->updateValues();
	return true; 
}

bool BlastPlugin::AppMainWindow_updatePluginUI()
{
	return true; 
}

bool BlastPlugin::AppMainWindow_processDragAndDrop(QString fname)
{ 
	return true; 
}

bool BlastPlugin::AppMainWindow_closeEvent(QCloseEvent *event)
{
	if (!menu_saveProject())
	{
		return false;
	}

	return true;
}


bool BlastPlugin::AppMainWindow_InitToolbar(QWidget *pQWidget, QVBoxLayout* pLayout)
{ 
	//_mainToolbar = new BlastToolbar(pQWidget);
	//pLayout->insertWidget(0, _mainToolbar);
//	connect(_mainToolbar->getUI().btnFileOpen, SIGNAL(clicked()), this, SLOT(menu_openProject()));

	return true; 
}

bool BlastPlugin::AppMainWindow_shortcut_expert(bool mode)
{ 
	if (_mainToolbar)
		_mainToolbar->setVisible(mode);

	return true; 
}

bool BlastPlugin::AppMainWindow_updateMainToolbar()
{ 
	if (_mainToolbar)
		_mainToolbar->updateValues();

	return true; 
}

bool BlastPlugin::AppMainWindow_menu_about()
{
	return true;
}

bool BlastPlugin::AppMainWindow_menu_opendoc()
{
	QString appDir = QApplication::applicationDirPath();
	QString docsFile = QFileInfo(appDir + "/../../docs/User_Guide/Nvidia Blast.chm").absoluteFilePath();

	QUrl docsUrl = QUrl::fromLocalFile(docsFile);
	QUrl url = QUrl::fromUserInput(QString("http://docs.nvidia.com/gameworks/content/artisttools/blast/index.html"));
	QDesktopServices::openUrl(url);

	return true;
}

#if USE_CURVE_EDITOR
#include "CurveEditorMainWindow.h"

bool BlastPlugin::AppMainWindow_UpdateCurveEditor()
{
	return true;
}
bool BlastPlugin::AppMainWindow_ShowCurveEditor(int paramId)
{
	return true;
}
bool BlastPlugin::AppMainWindow_onCurveAttributeChanged(nvidia::CurveEditor::CurveAttribute* attribute)
{
	return true;
}
bool BlastPlugin::AppMainWindow_onColorAttributeChanged(nvidia::CurveEditor::ColorAttribute* attribute)
{
	return true;
}
bool BlastPlugin::AppMainWindow_onReloadColorAttributeTexture(nvidia::CurveEditor::ColorAttribute* attribute, bool reloadColorTex, int selectedCtrlPntIndex)
{
	return true;
}
#endif

bool BlastPlugin::menu_openProject()
{
	AppMainWindow& window = AppMainWindow::Inst();
	QString lastDir = window._lastFilePath;
	QString fileName = QFileDialog::getOpenFileName(&window, "Open Blast Project File", lastDir, "Blast Project File (*.blastProj)");

	return window.openProject(fileName);
}

bool BlastPlugin::menu_saveProject()
{
	char message[1024];

	std::string projectFilePath = GlobalSettings::Inst().getAbsoluteFilePath();
	std::string projectFileName = GlobalSettings::Inst().m_projectFileName;
	if (projectFileName != "")
	{
		if (SimpleScene::Inst()->SaveProject(
			GlobalSettings::Inst().m_projectFileDir.c_str(),
			GlobalSettings::Inst().m_projectFileName.c_str()
			) == false)
		{
			QMessageBox messageBox;

			sprintf(message, "Project file %s could not be saved!", (const char*)projectFilePath.c_str());
			messageBox.critical(0, "Error", message);
			messageBox.setFixedSize(500, 200);
			char message[1024];
			sprintf(message, "Failed to save project file(\"%s\")", (const char*)projectFilePath.c_str());
			viewer_err(message);
			return false;
		}

		sprintf(message, "Project file %s was saved.", (const char*)projectFilePath.c_str());

		/*
		QMessageBox messageBox;
		messageBox.information(0, "Info", message);
		messageBox.setFixedSize(500,200);
		*/
		viewer_msg(message);
		return true;
	}
	else
	{
		return menu_saveProjectAs();
	}
	return false;
}

bool BlastPlugin::menu_saveProjectAs()
{
	AppMainWindow& window = AppMainWindow::Inst();

	char message[1024];

	QString lastDir = window._lastFilePath;
	QString fileName = QFileDialog::getSaveFileName(&window, "Save Blast Project File", lastDir, "Blast Project File (*.blastProj)");
	if (!fileName.isEmpty())
	{
		QFileInfo fileInfo(fileName);
		QByteArray dir = QDir::toNativeSeparators(fileInfo.absoluteDir().absolutePath()).toLocal8Bit();
		QByteArray file = fileInfo.fileName().toLocal8Bit();

		if (SimpleScene::Inst()->SaveProject(dir, file) == false)
		{
			QMessageBox messageBox;
			sprintf(message, "Project file %s could not be saved!", (const char*)file);
			messageBox.critical(0, "Error", message);
			messageBox.setFixedSize(500, 200);
			return false;
		}

		sprintf(message, "Project file %s was saved.", (const char*)file);

		/*
		QMessageBox messageBox;
		messageBox.information(0, "Info", message);
		messageBox.setFixedSize(500,200);
		*/

		viewer_msg(message);

		window._lastFilePath = fileInfo.absoluteDir().absolutePath();
		return true;
	}
	return false;
}

bool BlastPlugin::shortcut_damagetool()
{
	SampleManager& sampleManager = SimpleScene::Inst()->GetSampleManager();
	sampleManager.setBlastToolType(BTT_Damage);
	return true;
}

bool BlastPlugin::shortcut_selecttool()
{
	SampleManager& sampleManager = SimpleScene::Inst()->GetSampleManager();
	sampleManager.setBlastToolType(BTT_Select);
	return true;
}

bool BlastPlugin::shortcut_Translate()
{
	SampleManager& sampleManager = SimpleScene::Inst()->GetSampleManager();
	sampleManager.setBlastToolType(BTT_Translate);
	return true;
}

bool BlastPlugin::shortcut_Rotation()
{
	SampleManager& sampleManager = SimpleScene::Inst()->GetSampleManager();
	sampleManager.setBlastToolType(BTT_Rotation);
	return true;
}

bool BlastPlugin::shortcut_Scale()
{
	SampleManager& sampleManager = SimpleScene::Inst()->GetSampleManager();
	sampleManager.setBlastToolType(BTT_Scale);
	return true;
}

bool BlastPlugin::shortcut_edittool()
{
	SampleManager& sampleManager = SimpleScene::Inst()->GetSampleManager();
	sampleManager.setBlastToolType(BTT_Edit);
	return true;
}

bool BlastPlugin::slot_Make_Support()
{
	BlastSceneTree::ins()->makeSupport();
	return true;
}

bool BlastPlugin::slot_Make_Static_Support()
{
	BlastSceneTree::ins()->makeStaticSupport();
	return true;
}

bool BlastPlugin::slot_Remove_Support()
{
	BlastSceneTree::ins()->removeSupport();
	return true;
}

bool BlastPlugin::slot_Bond_Chunks()
{
	BlastSceneTree::ins()->bondChunks();
	return true;
}

bool BlastPlugin::slot_Bond_Chunks_with_Joints()
{
	BlastSceneTree::ins()->bondChunksWithJoints();
	return true;
}

bool BlastPlugin::slot_Remove_all_Bonds()
{
	BlastSceneTree::ins()->removeAllBonds();
	return true;
}
