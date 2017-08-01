#ifndef BlastInterface_H
#define BlastInterface_H

#include <QtCore/QString>
#include <QtCore/QObject>
#if USE_CURVE_EDITOR
#include <Attribute.h>
#endif
#include <Nv/NvBlastCommon.h>
class QMenuBar;
class QHBoxLayout;
class QVBoxLayout;
class QTabWidget;

class Camera;
class Light;
enum RenderApi;
namespace NvParameterized
{
	class Interface;
	class Handle;
}
class D3D11RenderShader;
class QCloseEvent;
#define NV_AT_UNUSED

class QResizeEvent;
class QPaintEvent;
class QMouseEvent;
class QWheelEvent;
class QKeyEvent;
class QDragEnterEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
class QDropEvent;
class QContextMenuEvent;
class QAction;

class PluginInterface
{
public:
	virtual ~PluginInterface() {}

	virtual QString GetPluginName() = 0;

	virtual bool CoreLib_RunApp() = 0;

	virtual bool LoadRenderPlugin(std::string api) = 0;

	virtual bool GetBoneNames(std::vector<std::string>& BoneNames) = 0;

	NV_AT_UNUSED virtual bool MainToolbar_updateValues() = 0;

	NV_AT_UNUSED virtual bool CurveEditor_updateValues(int _paramId, float* _values) = 0;
	NV_AT_UNUSED virtual bool CurveEditor_onUpdateValues(int _paramId, float* _values) = 0;

	virtual bool DisplayMeshesPanel_updateValues() = 0;
	virtual bool DisplayMeshesPanel_EmitToggleSignal(unsigned int id, bool visible) = 0;
	
	virtual bool Camera_LoadParameters(void* ptr, Camera* pCamera) = 0;
	virtual bool Camera_SaveParameters(void * ptr, Camera* pCamera) = 0;

	virtual bool Gamepad_ToggleSimulation() = 0;
	virtual bool Gamepad_ResetScene() = 0;
	NV_AT_UNUSED virtual bool Gamepad_LoadSamples(QString fn) = 0;
	virtual bool Gamepad_PlaySample() = 0;
	virtual bool GamepadHandler_ShowHair() = 0;
	virtual bool GamepadHandler_SpinWindStrength(float windStrength) = 0;
	virtual bool Gamepad_ResetAnimation() = 0;
	virtual bool Gamepad_PlayPauseAnimation() = 0;
	
	virtual bool Light_loadParameters(NvParameterized::Handle& handle, Light* pLight) = 0;
	virtual bool Light_saveParameters(NvParameterized::Handle& handle, Light* pLight) = 0;

	virtual void SimpleScene_OpenFilesByDrop(const QStringList& fileNames) = 0;
	virtual bool SimpleScene_SimpleScene() = 0;
	virtual bool SimpleScene_Initialize(int backdoor) = 0;
	virtual bool SimpleScene_Shutdown() = 0;
	virtual bool SimpleScene_Clear() = 0;
	virtual bool SimpleScene_Draw_DX12() = 0;
	virtual bool SimpleScene_Draw_DX11() = 0;
	virtual bool SimpleScene_FitCamera(atcore_float3& center, atcore_float3& extents) = 0;
	virtual bool SimpleScene_UpdateCamera() = 0;
	virtual bool SimpleScene_LoadSceneFromFbx(const char* dir, const char* fbxName) = 0;
	virtual bool SimpleScene_LoadProject(const char* dir, const char* file) = 0;
	virtual bool SimpleScene_SaveProject(const char* dir, const char* file) = 0;
	virtual bool SimpleScene_LoadParameters(NvParameterized::Interface* iface) = 0;
	virtual bool SimpleScene_SaveParameters(NvParameterized::Interface* iface) = 0;
	virtual bool SimpleScene_LoadCameraBookmarks(NvParameterized::Interface* iface) = 0;
	virtual bool SimpleScene_SaveCameraBookmarks(NvParameterized::Interface* iface) = 0;
	NV_AT_UNUSED virtual bool SimpleScene_DrawGround() = 0;
	NV_AT_UNUSED virtual bool SimpleScene_DrawWind() = 0;
	NV_AT_UNUSED virtual bool SimpleScene_DrawAxis() = 0;

	virtual bool D3DWidget_resizeEvent(QResizeEvent* e) = 0;
	virtual bool D3DWidget_paintEvent(QPaintEvent* e) = 0;
	virtual bool D3DWidget_mousePressEvent(QMouseEvent* e) = 0;
	virtual bool D3DWidget_mouseReleaseEvent(QMouseEvent* e) = 0;
	virtual bool D3DWidget_mouseMoveEvent(QMouseEvent* e) = 0;
	virtual bool D3DWidget_wheelEvent(QWheelEvent * e) = 0;
	virtual bool D3DWidget_keyPressEvent(QKeyEvent* e) = 0;
	virtual bool D3DWidget_keyReleaseEvent(QKeyEvent* e) = 0;
	virtual bool D3DWidget_dragEnterEvent(QDragEnterEvent *e) = 0;
	virtual bool D3DWidget_dragMoveEvent(QDragMoveEvent *e) = 0;
	virtual bool D3DWidget_dragLeaveEvent(QDragLeaveEvent *e) = 0;
	virtual bool D3DWidget_dropEvent(QDropEvent *e) = 0;
	virtual bool D3DWidget_contextMenuEvent(QContextMenuEvent *e) = 0;

	virtual bool D3D11Shaders_InitializeShadersD3D11(std::map<int, D3D11RenderShader*>& ShaderMap) = 0;

	virtual bool AppMainWindow_AppMainWindow() = 0;
	virtual bool AppMainWindow_InitMenuItems(QMenuBar* pMenuBar) = 0;
	virtual bool AppMainWindow_InitMainTab(QWidget *displayScrollAreaContents, QVBoxLayout *displayScrollAreaLayout, int idx) = 0;
	virtual bool AppMainWindow_InitPluginTab(QTabWidget* pTabWidget) = 0;
	virtual bool AppMainWindow_InitUI() = 0;
	virtual bool AppMainWindow_updateUI() = 0;
	virtual bool AppMainWindow_updatePluginUI() = 0;
	NV_AT_UNUSED virtual bool AppMainWindow_processDragAndDrop(QString fname) = 0;
	virtual bool AppMainWindow_closeEvent(QCloseEvent *event) = 0;
	virtual bool AppMainWindow_InitToolbar(QWidget *pQWidget, QVBoxLayout* pLayout) = 0;
	virtual bool AppMainWindow_shortcut_expert(bool mode) = 0;
	virtual bool AppMainWindow_updateMainToolbar() = 0;

	virtual bool AppMainWindow_menu_item_triggered(QAction* action) = 0;
	virtual bool AppMainWindow_menu_about() = 0;
	virtual bool AppMainWindow_menu_opendoc() = 0;
#if USE_CURVE_EDITOR
	virtual bool AppMainWindow_UpdateCurveEditor() = 0;
	virtual bool AppMainWindow_ShowCurveEditor(int paramId) = 0;
	virtual bool AppMainWindow_onCurveAttributeChanged(nvidia::CurveEditor::CurveAttribute* attribute) = 0;
	virtual bool AppMainWindow_onColorAttributeChanged(nvidia::CurveEditor::ColorAttribute* attribute) = 0;
	virtual bool AppMainWindow_onReloadColorAttributeTexture(nvidia::CurveEditor::ColorAttribute* attribute, bool reloadColorTex, int selectedCtrlPntIndex) = 0;
#endif
};

QT_BEGIN_NAMESPACE
#define PluginInterface_iid "com.nvidia.PluginInterface"
Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)
QT_END_NAMESPACE

#endif