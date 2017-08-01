#ifndef APPMAINWINDOW_H
#define APPMAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QProgressDialog>

#include "ui_AppMainWindow.h"
#include "UIGlobal.h"
#include "XMLHelper.h"

class StyleMaterialPanel;
class AssetControlPanel;
class PhysicalMaterialPanel;
class GraphicalMaterialPanel;
class DisplayFurVisualizersPanel;
class GeneralAttributePanel;
class BlastToolbar;
class LodPanel;
class DisplayMeshesPanel;
class DisplayPreferencesPanel; 
class DisplayScenePanel;
class DisplayLightPanel;
class DisplayMeshMaterialsPanel;

#if USE_CURVE_EDITOR
namespace nvidia {
namespace CurveEditor {

class CurveEditorMainWindow;
class CurveAttribute;
class ColorAttribute;

} // namespace CurveEditor
} // namespace nvidia
#endif

#if USE_MATERIAL_SET
class MaterialSetPanel;
#endif

class D3DWidget;
class MsgPrinter;
class AppMainWindow : public QMainWindow
{
	Q_OBJECT

public:
	AppMainWindow(QWidget *parent = 0, Qt::WindowFlags flags = 0);
	~AppMainWindow();

	CORELIB_EXPORT static AppMainWindow& Inst();

	CORELIB_EXPORT void InitUI();
	CORELIB_EXPORT void updateUI();
	CORELIB_EXPORT void updatePluginUI();

	static bool IsExpertMode();

	D3DWidget* GetRenderWidget() {return _d3dWidget;}
	char TestMouseScheme(Qt::KeyboardModifiers modifiers, Qt::MouseButtons buttons);
	
	CORELIB_EXPORT QString OpenTextureFile(QString title = "");

	void setNavigationStyle(int index) { _navigationStyle = index; }
	int getNavigationStyle() { return _navigationStyle; }

	void closeEvent (QCloseEvent *event);

	CORELIB_EXPORT void startProgress();
	CORELIB_EXPORT void setProgressMaximum(int m);
	CORELIB_EXPORT void setProgress(const char* label, int progress = -1);
	CORELIB_EXPORT void endProgress();
	CORELIB_EXPORT void quit();

	CORELIB_EXPORT void updateMainToolbar();
	CORELIB_EXPORT void processDragAndDrop(const QStringList& fileNames);
	CORELIB_EXPORT bool openProject(QString fileName);

	CORELIB_EXPORT static void setConnectionMode(int);

	DisplayMeshesPanel*	GetDisplayMeshesPanel() { return _displayMeshesPanel; }
	DisplayPreferencesPanel* GetDisplayPreferencesPanel() { return _displayPreferencesPanel; }
	DisplayScenePanel* GetDisplayScenePanel() { return _displayScenePanel; }
	DisplayLightPanel* GetDisplayLightPanel() { return _displayLightPanel; }

	void removeBookmark(const QString& name);
	void renameBookmark(const QString& oldName, const QString& newName);

#if USE_CURVE_EDITOR
	CORELIB_EXPORT nvidia::CurveEditor::CurveEditorMainWindow* GetCurveEditorMainWindow() { return _curveEditorMainWindow; }
	CORELIB_EXPORT void UpdateCurveEditor();
	CORELIB_EXPORT void ShowCurveEditor(int paramId);
#endif

	inline QString GetFilePath()		{ return _lastFilePath; }
signals:
	void aboutToQuit();

	public slots:
		CORELIB_EXPORT void menu_item_triggered(QAction* action);
		CORELIB_EXPORT void menu_clearScene();
		CORELIB_EXPORT bool menu_openfbx();
		CORELIB_EXPORT void menu_addBookmark();
		CORELIB_EXPORT void menu_editBookmarks();
		CORELIB_EXPORT void menu_bookmarkTriggered(QAction* act);
		CORELIB_EXPORT void menu_showOutput();
		CORELIB_EXPORT void menu_showAttributeEditor();
		CORELIB_EXPORT void menu_about();
		CORELIB_EXPORT void menu_opendoc();
		CORELIB_EXPORT void shortcut_frameall();
		CORELIB_EXPORT void shortcut_hud();
		CORELIB_EXPORT void shortcut_statistics();
		CORELIB_EXPORT void shortcut_pause();
		CORELIB_EXPORT void shortcut_reset();
		CORELIB_EXPORT void shortcut_escape();
		CORELIB_EXPORT void shortcut_expert();
		CORELIB_EXPORT void demo_next();
		CORELIB_EXPORT void demo_prev();
		CORELIB_EXPORT void shortcut_output();
		CORELIB_EXPORT void shortcut_meshmat();
		CORELIB_EXPORT void shortcut_fitcamera();

#if USE_CURVE_EDITOR
		CORELIB_EXPORT void menu_showCurveEditor();
		CORELIB_EXPORT void onCurveAttributeChanged(nvidia::CurveEditor::CurveAttribute* attribute);
		CORELIB_EXPORT void onColorAttributeChanged(nvidia::CurveEditor::ColorAttribute* attribute);
		CORELIB_EXPORT void onReloadColorAttributeTexture(nvidia::CurveEditor::ColorAttribute* attribute, bool reloadColorTex, int selectedCtrlPntIndex);
#endif

		CORELIB_EXPORT void addRecentFile(const QString filePath);
private:
	char TestDragCamera(Qt::KeyboardModifiers modifiers, Qt::MouseButtons buttons);

	void _resetRecentFile(const QString filePath);
	void _loadRecentFile();
	void _saveRecentFile();

public:
	void InitMenuItems();
	void InitToolbar();
	void InitPluginTab();
	void InitMainTab();
	void InitMouseSchemes();
	void InitShortCuts();
	void updateBookmarkMenu();

	Ui::AppMainWindowClass ui;

	D3DWidget* _d3dWidget;
	QMenu*					_recentMenu;
	QList<QAction*>			_recentFileActions;
	SingleItemKindFile		_recentFileRecordFile;

	QMenu*					_bookmarksMenu;

	DisplayMeshesPanel*			_displayMeshesPanel;
	DisplayPreferencesPanel*	_displayPreferencesPanel;
	DisplayScenePanel*			_displayScenePanel;
	DisplayLightPanel*			_displayLightPanel;


#if USE_CURVE_EDITOR
	bool					_curveEditorInitizlized;
	nvidia::CurveEditor::CurveEditorMainWindow* _curveEditorMainWindow;
#endif

	typedef QPair<Qt::KeyboardModifiers, Qt::MouseButtons> ShortCut;
	QMap<ShortCut, char>	_mayaScheme;
	QMap<ShortCut, char>	_maxScheme;
	QActionGroup*			_bookmarkActionGroup;
	QAction*				_actionAddBookmark;
	QAction*				_actionEditBookmarks;

	QString					_lastFilePath;

	int						_navigationStyle;

	MsgPrinter*				_printer;

	QProgressDialog			_progressDialog;


	static bool				_expertMode;

	CORELIB_EXPORT static int				_connectionMode;

#ifndef NV_ARTISTTOOLS	
	BlastToolbar* GetMainToolbar() { return _mainToolbar; }
	DisplayFurVisualizersPanel*  GetFurVisualizersPanel() { return _displayFurVisualizersPanel; }

public slots:
	bool menu_openProject();
	bool menu_saveProject();
	bool menu_saveProjectAs();
	bool menu_openHair();
	bool menu_importHair();
	bool menu_saveHair();
	bool menu_saveHairAs();
	bool menu_saveAllHairs();

private:
	BlastToolbar*				_mainToolbar;
	AssetControlPanel*			_assetControlPanel;
	GeneralAttributePanel*		_generalAttributePanel;
	DisplayFurVisualizersPanel* _displayFurVisualizersPanel;
	StyleMaterialPanel*			_styleMaterialPanel;
	PhysicalMaterialPanel*		_physicalMaterialPanel;
	GraphicalMaterialPanel*		_graphicalMaterialPanel;
	LodPanel*					_lodPanel;
	DisplayMeshMaterialsPanel*	_displayMeshMaterialsPanel;
#if USE_MATERIAL_SET
	MaterialSetPanel*		_materialSetPanel;
#endif
#endif // NV_ARTISTTOOLS

	bool m_bGizmoWithLocal;
	bool m_bGizmoWithDepthTest;
	bool m_bShowPlane;
};

#endif // APPMAINWINDOW_H
