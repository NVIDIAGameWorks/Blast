#ifndef CORELIB_H
#define CORELIB_H

#include "corelib_global.h"
#include "PluginInterface.h"
#include "NvParameterized.h"
#if USE_CURVE_EDITOR
#include <Attribute.h>
#endif

class QCloseEvent;
class Camera;
class Light;
namespace NvParameterized
{
	class Interface;
	class Handle;
}
class D3D11RenderShader;

class CORELIB_EXPORT CoreLib
{
public:
	CoreLib();
	~CoreLib();

	static CoreLib*	Inst();
#ifndef NV_ARTISTTOOLS
	int CoreMain(int argc, char *argv[]);
#else
	int CoreMain(int argc, char *argv[], bool withPlugin = true);

	bool CoreLib_RunApp();
	void LoadPlugins(QApplication& app);
	PluginInterface* GetPluginInterface(QString name);

	bool GetBoneNames(std::vector<std::string>& BoneNames);
	bool MainToolbar_updateValues();

	bool CurveEditor_updateValues(int _paramId, float* _values);
	bool CurveEditor_onUpdateValues(int _paramId, float* _values);

	bool DisplayMeshesPanel_updateValues();
	bool DisplayMeshesPanel_EmitToggleSignal(unsigned int id, bool visible);

	bool Camera_LoadParameters(void* ptr, Camera* pCamera);
	bool Camera_SaveParameters(void* ptr, Camera* pCamera);
	bool Light_loadParameters(NvParameterized::Handle& handle, Light* pLight);
	bool Light_saveParameters(NvParameterized::Handle& handle, Light* pLight);

	bool Gamepad_ToggleSimulation();
	bool Gamepad_LoadSamples(QString fn);
	bool Gamepad_ResetScene();
	bool Gamepad_PlaySample();
	bool GamepadHandler_ShowHair();
	bool GamepadHandler_SpinWindStrength(float windStrength);
	bool Gamepad_ResetAnimation();
	bool Gamepad_PlayPauseAnimation();

	void SimpleScene_OpenFilesByDrop(const QStringList& fileNames);
	bool SimpleScene_SimpleScene();
	bool SimpleScene_Initialize(int backdoor);
	bool SimpleScene_Shutdown();
	bool SimpleScene_Clear();
	bool SimpleScene_Draw_DX12();
	bool SimpleScene_Draw_DX11();
	bool SimpleScene_FitCamera(atcore_float3& center, atcore_float3& extents);
	bool SimpleScene_ResetUpDir(bool zup);
	bool SimpleScene_UpdateCamera();
	bool SimpleScene_DrawGround();
	bool SimpleScene_DrawWind();
	bool SimpleScene_DrawAxis();
	bool SimpleScene_LoadSceneFromFbx(const char* dir, const char* fbxName);
	bool SimpleScene_LoadProject(const char* dir, const char* file);
	bool SimpleScene_SaveProject(const char* dir, const char* file);
	bool SimpleScene_LoadParameters(NvParameterized::Interface* iface);
	bool SimpleScene_SaveParameters(NvParameterized::Interface* iface);
	bool SimpleScene_LoadCameraBookmarks(NvParameterized::Interface* iface);
	bool SimpleScene_SaveCameraBookmarks(NvParameterized::Interface* iface);
	
	bool D3DWidget_resizeEvent(QResizeEvent* e);
	bool D3DWidget_paintEvent(QPaintEvent* e);
	bool D3DWidget_mousePressEvent(QMouseEvent* e);
	bool D3DWidget_mouseReleaseEvent(QMouseEvent* e);
	bool D3DWidget_mouseMoveEvent(QMouseEvent* e);
	bool D3DWidget_wheelEvent(QWheelEvent * e);
	bool D3DWidget_keyPressEvent(QKeyEvent* e);
	bool D3DWidget_keyReleaseEvent(QKeyEvent* e);
	bool D3DWidget_dragEnterEvent(QDragEnterEvent *e);
	bool D3DWidget_dragMoveEvent(QDragMoveEvent *e);
	bool D3DWidget_dragLeaveEvent(QDragLeaveEvent *e);
	bool D3DWidget_dropEvent(QDropEvent *e);
	bool D3DWidget_contextMenuEvent(QContextMenuEvent *e);

	bool D3D11Shaders_InitializeShadersD3D11(std::map<int, D3D11RenderShader*>& ShaderMap);

	bool AppMainWindow_AppMainWindow();
	bool AppMainWindow_InitMenuItems(QMenuBar* pMenuBar);
	bool AppMainWindow_InitMainTab(QWidget *displayScrollAreaContents, QVBoxLayout *displayScrollAreaLayout, int idx);
	bool AppMainWindow_InitPluginTab(QTabWidget* pTabWidget);
	bool AppMainWindow_InitUI();
	bool AppMainWindow_updateUI();
	bool AppMainWindow_updatePluginUI();
	bool AppMainWindow_processDragAndDrop(QString fname);
	bool AppMainWindow_closeEvent(QCloseEvent *event);
	bool AppMainWindow_InitToolbar(QWidget *pQWidget, QVBoxLayout* pLayout);
	bool AppMainWindow_shortcut_expert(bool mode);
	bool AppMainWindow_updateMainToolbar();

	bool menu_item_triggered(QAction* action);
	bool AppMainWindow_menu_about();
	bool AppMainWindow_menu_opendoc();
#if USE_CURVE_EDITOR
	bool AppMainWindow_UpdateCurveEditor();
	bool AppMainWindow_ShowCurveEditor(int paramId);
	bool AppMainWindow_onCurveAttributeChanged(nvidia::CurveEditor::CurveAttribute* attribute);
	bool AppMainWindow_onColorAttributeChanged(nvidia::CurveEditor::ColorAttribute* attribute);
	bool AppMainWindow_onReloadColorAttributeTexture(nvidia::CurveEditor::ColorAttribute* attribute, bool reloadColorTex, int selectedCtrlPntIndex);
#endif
private:
	std::map<QString, PluginInterface*> m_PluginInterfaces;
#endif // NV_ARTISTTOOLS
};

#endif // CORELIB_H
