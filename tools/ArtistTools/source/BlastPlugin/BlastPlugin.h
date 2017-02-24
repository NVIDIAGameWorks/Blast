#ifndef BLASTPLUGIN_H
#define BLASTPLUGIN_H

#include <QtCore/QObject>
#include <QtCore/QtPlugin>
#include "PluginInterface.h"
#include "UIGlobal.h"

class QMenu;
class QAction;
class QDoubleSpinBox;
class QComboBox;
class QCheckBox;
class QVBoxLayout;
class BlastToolbar;
class FileReferencesPanel;
class GeneralPanel;
class BlastCompositePanel;
class DefaultDamagePanel;
class MaterialLibraryPanel;
class MaterialAssignmentsPanel;
class FractureCutoutSettingsPanel;
class FractureGeneralPanel;
class FractureShellCutSettingsPanel;
class FractureSliceSettingsPanel;
class FractureVisualizersPanel;
class FractureVoronoiSettingsPanel;
class SupportPanel;
class BlastSceneTree;
class FiltersDockWidget;

class Camera;
class Light;
namespace NvParameterized
{
	class Interface;
	class Handle;
}
class D3D11RenderShader;

#define BlastPluginName "BlastPlugin"

class BlastPlugin : public QObject, public PluginInterface
{
	Q_OBJECT
		Q_PLUGIN_METADATA(IID PluginInterface_iid FILE "blastplugin.json")
		Q_INTERFACES(PluginInterface)

public:
	virtual QString GetPluginName();

	virtual bool CoreLib_RunApp();

	virtual bool LoadRenderPlugin(std::string api);

	virtual bool GetBoneNames(std::vector<std::string>& BoneNames);

	virtual bool MainToolbar_updateValues();

	virtual bool CurveEditor_updateValues(int _paramId, float* _values);
	virtual bool CurveEditor_onUpdateValues(int _paramId, float* _values);

	virtual bool DisplayMeshesPanel_updateValues();
	virtual bool DisplayMeshesPanel_EmitToggleSignal(unsigned int id, bool visible);

	virtual bool Camera_LoadParameters(void* ptr, Camera* pCamera);
	virtual bool Camera_SaveParameters(void * ptr, Camera* pCamera);

	virtual bool Gamepad_ToggleSimulation();
	virtual bool Gamepad_LoadSamples(QString fn);
	virtual bool Gamepad_ResetScene();
	virtual bool Gamepad_StartAnimation();
	virtual bool GamepadHandler_ShowHair();
	virtual bool GamepadHandler_SpinWindStrength(float windStrength);
	virtual bool Gamepad_ResetAnimation();
	virtual bool Gamepad_PlayPauseAnimation();
	
	virtual bool Light_loadParameters(NvParameterized::Handle& handle, Light* pLight);
	virtual bool Light_saveParameters(NvParameterized::Handle& handle, Light* pLight);

	virtual bool SimpleScene_SimpleScene();
	virtual bool SimpleScene_Initialize(int backdoor);
	virtual bool SimpleScene_Shutdown();
	virtual bool SimpleScene_Clear();
	virtual bool SimpleScene_Draw_DX12();
	virtual bool SimpleScene_Draw_DX11();
	virtual bool SimpleScene_FitCamera(atcore_float3& center, atcore_float3& extents);
	virtual bool SimpleScene_DrawGround();
	virtual bool SimpleScene_DrawWind();
	virtual bool SimpleScene_DrawAxis();
	virtual bool SimpleScene_LoadSceneFromFbx(const char* dir, const char* fbxName);
	virtual bool SimpleScene_LoadProject(const char* dir, const char* file);
	virtual bool SimpleScene_SaveProject(const char* dir, const char* file);
	virtual bool SimpleScene_LoadParameters(NvParameterized::Interface* iface);
	virtual bool SimpleScene_SaveParameters(NvParameterized::Interface* iface);
	virtual bool SimpleScene_LoadCameraBookmarks(NvParameterized::Interface* iface);
	virtual bool SimpleScene_SaveCameraBookmarks(NvParameterized::Interface* iface);
	
	virtual bool D3DWidget_resizeEvent(QResizeEvent* e);
	virtual bool D3DWidget_paintEvent(QPaintEvent* e);
	virtual bool D3DWidget_mousePressEvent(QMouseEvent* e);
	virtual bool D3DWidget_mouseReleaseEvent(QMouseEvent* e);
	virtual bool D3DWidget_mouseMoveEvent(QMouseEvent* e);
	virtual bool D3DWidget_wheelEvent(QWheelEvent * e);
	virtual bool D3DWidget_keyPressEvent(QKeyEvent* e);
	virtual bool D3DWidget_keyReleaseEvent(QKeyEvent* e);
	virtual bool D3DWidget_dragEnterEvent(QDragEnterEvent *e);
	virtual bool D3DWidget_dragMoveEvent(QDragMoveEvent *e);
	virtual bool D3DWidget_dragLeaveEvent(QDragLeaveEvent *e);
	virtual bool D3DWidget_dropEvent(QDropEvent *e);
	virtual bool D3DWidget_contextMenuEvent(QContextMenuEvent *e);

	virtual bool D3D11Shaders_InitializeShadersD3D11(std::map<int, D3D11RenderShader*>& ShaderMap);

	virtual bool AppMainWindow_AppMainWindow();
	virtual bool AppMainWindow_InitMenuItems(QMenuBar* pMenuBar);
	virtual bool AppMainWindow_InitMainTab(QWidget *displayScrollAreaContents, QVBoxLayout *displayScrollAreaLayout, int idx);
	virtual bool AppMainWindow_InitPluginTab(QTabWidget* pTabWidget);
	virtual bool AppMainWindow_InitUI();
	virtual bool AppMainWindow_updateUI();
	virtual bool AppMainWindow_updatePluginUI();
	virtual bool AppMainWindow_processDragAndDrop(QString fname);
	virtual bool AppMainWindow_closeEvent(QCloseEvent *event);
	virtual bool AppMainWindow_InitToolbar(QWidget *pQWidget, QVBoxLayout* pLayout);
	virtual bool AppMainWindow_shortcut_expert(bool mode);
	virtual bool AppMainWindow_updateMainToolbar();

	virtual bool AppMainWindow_menu_about();
	virtual bool AppMainWindow_menu_opendoc();
#if USE_CURVE_EDITOR
	virtual bool AppMainWindow_UpdateCurveEditor();
	virtual bool AppMainWindow_ShowCurveEditor(int paramId);
	virtual bool AppMainWindow_onCurveAttributeChanged(nvidia::CurveEditor::CurveAttribute* attribute);
	virtual bool AppMainWindow_onColorAttributeChanged(nvidia::CurveEditor::ColorAttribute* attribute);
	virtual bool AppMainWindow_onReloadColorAttributeTexture(nvidia::CurveEditor::ColorAttribute* attribute, bool reloadColorTex, int selectedCtrlPntIndex);
#endif
	BlastToolbar* GetMainToolbar() { return _mainToolbar; }

public:
	static void DrawHUD();

	/////////////////////////////////////////////////////////////////////
	// profiler and timer
	static void ResetFrameTimer();
	
public slots:
	bool menu_openProject();
	bool menu_saveProject();
	bool menu_saveProjectAs();
	bool shortcut_damagetool();
	bool shortcut_selecttool();
	bool shortcut_Translate();
	bool shortcut_Rotation();
	bool shortcut_Scale();
	bool shortcut_edittool();

	bool slot_Make_Support();
	bool slot_Make_Static_Support();
	bool slot_Remove_Support();
	bool slot_Bond_Chunks();
	bool slot_Bond_Chunks_with_Joints();
	bool slot_Remove_all_Bonds();

private:
	BlastToolbar*					_mainToolbar;
	MaterialLibraryPanel*			_materialLibraryPanel;
	MaterialAssignmentsPanel*		_materialAssignmentsPanel;
	FileReferencesPanel*			_fileReferencesPanel;
	GeneralPanel*					_generalPanel;
	BlastCompositePanel*			_blastCompositePanel;
	DefaultDamagePanel*				_defaultDamagePanel;
	FractureCutoutSettingsPanel*	_fractureCutoutSettingsPanel;
	FractureGeneralPanel*			_fractureGeneralPanel;
	FractureShellCutSettingsPanel*	_fractureShellCutSettingPanel;
	FractureSliceSettingsPanel*		_fractureSliceSettingsPanel;
	FractureVisualizersPanel*		_fractureVisualizersPanel;
	FractureVoronoiSettingsPanel*	_fractureVoronoiSettingsPanel;
	SupportPanel*					_supportPanel;
	BlastSceneTree*					_blastSceneTree;
	FiltersDockWidget*				_filtersDockWidget;

	QMenu*	 _chunkContextMenu;
	QMenu*	 _bondContextMenu;
	QAction* action_Make_Support;
	QAction* action_Make_Static_Support;
	QAction* action_Remove_Support;
	QAction* action_Bond_Chunks;
	QAction* action_Bond_Chunks_with_Joints;
	QAction* action_Remove_all_Bonds;
};

#endif // HAIRWORKSPLUGIN_H
