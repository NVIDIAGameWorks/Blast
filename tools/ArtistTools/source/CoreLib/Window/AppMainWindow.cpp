#include <QtWidgets/QShortcut>
#include <QtWidgets/QAction>
#include <QtWidgets/QMenu>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QGraphicsEllipseItem>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QProgressDialog>

#include <Windows.h>

#include "AppMainWindow.h"
#include "ExpandablePanel.h"

#include "D3DWidget.h"

#include "DisplayMeshesPanel.h"

#include "DisplayPreferencesPanel.h"
#include "DisplayScenePanel.h"
#include "DisplayLightPanel.h"
#include "ui_DisplayPreferencesPanel.h"
#include "ui_DisplayScenePanel.h"
#include "ui_DisplayLightPanel.h"
#include "CameraBookmarksDialog.h"

#include "SimpleScene.h"
#include "GlobalSettings.h"

#include "ViewerOutput.h"
#include "Gamepad.h"
#if USE_CURVE_EDITOR
#include "CurveEditorMainWindow.h"
#endif
#ifndef NV_ARTISTTOOLS
#include "ui_AppMainToolbar.h"
#include "ui_AssetControlPanel.h"
#include "ui_GeneralAttributesPanel.h"
#include "ui_PhysicalMaterialsPanel.h"
#include "ui_StyleMaterialsPanel.h"
#include "ui_GraphicalMaterialsPanel.h"
#include "BlastToolbar.h"
#include "AssetControlPanel.h"
#include "GraphicalMaterialPanel.h"
#include "PhysicalMaterialPanel.h"
#include "StyleMaterialPanel.h"
#include "GeneralAttributePanel.h"
#include "MaterialSetPanel.h"
#include "LodPanel.h"
#include "DisplayVisualizersPanel.h"
#include "ui_DisplayVisualizersPanel.h"
#include "DisplayMeshMaterialsPanel.h"
#include "FurCharacter.h"
#include "HairParams.h"
#include "AboutDialog.h"
#else
#endif // NV_ARTISTTOOLS

class MsgPrinter : public FrontPrinter
{
public:
	MsgPrinter(QTextBrowser* browser):_browser(browser)
	{
		
	}

	void print(const char* txt, unsigned long color /* = 0 */, Effect e /* = NONE */)
	{
		if(color == 0 && e == FrontPrinter::NONE)
		{
			_browser->insertPlainText(QString(txt)+"\n");
		}
		else
		{
			// output html text
			QString surfix;
			QString postfix;
			surfix.reserve(64);
			postfix.reserve(64);
			if(e & FrontPrinter::BOLD)
			{
				surfix = surfix + "<b>";
				postfix= postfix + "</b>";
			}
			else if(e & FrontPrinter::UNDERLINE)
			{
				surfix = surfix + "<u>";
				postfix= QString("</u>") + postfix;
			}
			else if(e & FrontPrinter::ITALIC)
			{
				surfix = surfix + "<i>";
				postfix= QString("</i>") + postfix;
			}

			if(color != 0)
			{
				int r = GetRValue(color);
				int g = GetGValue(color);
				int b = GetBValue(color);
			
				surfix = QString("<div style=\"color:rgb(%1,%2,%3);\">").arg(r).arg(g).arg(b) + surfix;
				postfix= postfix + QString("</div><br />");
			}

			QString html = surfix + QString(txt) + postfix;
			_browser->insertHtml(html);
		}
		// auto scroll to the bottom
		QScrollBar *sb = _browser->verticalScrollBar();
		if (sb) sb->setValue(sb->maximum());
	}

private:
	QTextBrowser* _browser;
};

int AppMainWindow::_connectionMode = 0;
bool AppMainWindow::_expertMode = false;
AppMainWindow* gAppMainWindow = NV_NULL;

void CreateAppMainWindow()
{
	gAppMainWindow = new AppMainWindow;
}

void ReleaseAppMainWindow()
{
	delete gAppMainWindow;
	gAppMainWindow = NV_NULL;
}

AppMainWindow& AppMainWindow::Inst()
{
	return *gAppMainWindow;
}

AppMainWindow::AppMainWindow(QWidget *parent, Qt::WindowFlags flags)
	: QMainWindow(parent, flags)
		, _bookmarksMenu(0)
		,_displayMeshesPanel(0)
		, _displayPreferencesPanel(0)
		, _displayScenePanel(0)
		,_displayLightPanel(0)
#if USE_CURVE_EDITOR
		,_curveEditorInitizlized(false)
		, _curveEditorMainWindow(nullptr)
#endif
		,_bookmarkActionGroup(0)
		,_actionAddBookmark(0)
		,_actionEditBookmarks(0)
{
	setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
	setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);

	ui.setupUi(this);
	//ui.dockOutputWindow->hide();

#ifndef NV_ARTISTTOOLS
	_mainToolbar = 0;
	_physicalMaterialPanel = 0;
	_styleMaterialPanel = 0;
	_graphicalMaterialPanel = 0;
	_displayFurVisualizersPanel = 0;
	_assetControlPanel = 0;
	_generalAttributePanel = 0;
	_lodPanel = 0;
	_displayMeshMaterialsPanel;
#if USE_MATERIAL_SET
	_materialSetPanel = 0;
#endif
#else
	CoreLib::Inst()->AppMainWindow_AppMainWindow();
#endif // NV_ARTISTTOOLS

	_progressDialog.close();    // prevent show one empty progress dialog when it runs.

	m_bGizmoWithLocal = false;
}

void AppMainWindow::InitUI()
{
	_d3dWidget = new D3DWidget(this);
	_d3dWidget->setMinimumSize(200, 200);
	ui.renderLayout->addWidget(_d3dWidget);

	InitShortCuts();
	InitMenuItems();
	InitToolbar();
	InitPluginTab();
	InitMainTab();

	InitMouseSchemes();

	updateUI();

	if (_connectionMode == 1) // master mode
		ui.sideBarTab->removeTab(1);

	if (_connectionMode == 2)
		ui.sideBarTab->removeTab(0);


	QString defFilePath;

	QString appDir = qApp->applicationDirPath();
	QDir dir(appDir);
	if (dir.cd("../../media"))
		defFilePath = dir.absolutePath();

	_lastFilePath = defFilePath;

	_navigationStyle = 0;

	// initialize the message printer
	_printer = new MsgPrinter(this->ui.outputWindow);
	ViewerOutput::Inst().RegisterPrinter(_printer);

#ifndef NV_ARTISTTOOLS
	
	viewer_info(
		"<a href=\"https://developer.nvidia.com/hairworks\" style=\"color:rgb(118,180,0);\">NVIDIA Blast Version v"
		NV_HAIR_RELEASE_VERSION_STRING
		"</a>");
	
#else
	CoreLib::Inst()->AppMainWindow_InitUI();
#endif // NV_ARTISTTOOLS
}

AppMainWindow::~AppMainWindow()
{
}

void AppMainWindow::setConnectionMode(int m)
{
	_connectionMode = m;
}

void AppMainWindow::startProgress()
{
	_progressDialog.setWindowModality(Qt::WindowModal);
	_progressDialog.show();
}

void AppMainWindow::setProgress(const char* label, int progress)
{
	_progressDialog.setLabelText(label);
	if (progress >= 0)
		_progressDialog.setValue(progress);

	_progressDialog.update();
}

void AppMainWindow::endProgress()
{
	_progressDialog.close();
}

void AppMainWindow::quit()
{
	emit aboutToQuit();
}

void AppMainWindow::setProgressMaximum(int m)
{
	_progressDialog.setMaximum(m);
}

void AppMainWindow::InitMenuItems()
{
	QMenuBar* menu = ui.menuBar;
	QAction* act = NV_NULL;

	// file sub-menu
	QMenu* fileMenu = menu->addMenu("&File");

	if (_connectionMode != 1)
	{
		act = new QAction("Clear scene", this);
		act->setShortcut(QKeySequence::New);
		connect(act, SIGNAL(triggered()), this, SLOT(menu_clearScene()));
		fileMenu->addAction(act);

		fileMenu->addSeparator();

		act = new QAction("Open fbx file", this);
		connect(act, SIGNAL(triggered()), this, SLOT(menu_openfbx()));
		fileMenu->addAction(act);

		fileMenu->addSeparator();

#ifndef NV_ARTISTTOOLS
		fileMenu->addSeparator();
		act = new QAction("Open project file", this);
		act->setShortcut(QKeySequence::Open);
		connect(act, SIGNAL(triggered()), this, SLOT(menu_openProject()));
		fileMenu->addAction(act);

		act = new QAction("Save project file", this);
		act->setShortcut(QKeySequence::Save);
		connect(act, SIGNAL(triggered()), this, SLOT(menu_saveProject()));
		fileMenu->addAction(act);

		act = new QAction("Save project file as...", this);
		connect(act, SIGNAL(triggered()), this, SLOT(menu_saveProjectAs()));
		fileMenu->addAction(act);

		fileMenu->addSeparator();

		act = new QAction("&Open hair file", this);
		connect(act, SIGNAL(triggered()), this, SLOT(menu_openHair()));
		fileMenu->addAction(act);

		act = new QAction("&Save hair file", this);
		connect(act, SIGNAL(triggered()), this, SLOT(menu_saveHair()));
		fileMenu->addAction(act);

		act = new QAction("Save hair file as...", this);
		act->setShortcut(QKeySequence::SaveAs);
		connect(act, SIGNAL(triggered()), this, SLOT(menu_saveHairAs()));
		fileMenu->addAction(act);

		act = new QAction("Save all hairs...", this);
		connect(act, SIGNAL(triggered()), this, SLOT(menu_saveAllHairs()));
		fileMenu->addAction(act);

		fileMenu->addSeparator();
#else
		CoreLib::Inst()->AppMainWindow_InitMenuItems(ui.menuBar);
#endif // NV_ARTISTTOOLS
	}

	act = new QAction("E&xit", this);
	act->setShortcut(QKeySequence::Quit);
	connect(act, SIGNAL(triggered()), this, SLOT(close()));
	fileMenu->addAction(act);

	// view submenu
	QMenu* viewMenu = menu->addMenu("&View");
	act = new QAction("Bookmarks", this);
	//connect(act, SIGNAL(triggered()), this, SLOT(menu_showOutput()));
	viewMenu->addAction(act);
	_bookmarksMenu = new QMenu("Bookmarks", this);
	connect(_bookmarksMenu, SIGNAL(triggered(QAction*)), this, SLOT(menu_bookmarkTriggered(QAction*)));
	act->setMenu(_bookmarksMenu);
	act = new QAction("Add Bookmark", this);
	connect(act, SIGNAL(triggered()), this, SLOT(menu_addBookmark()));
	_bookmarksMenu->addAction(act);
	_actionAddBookmark = act;
	act = new QAction("Edit Bookmarks", this);
	connect(act, SIGNAL(triggered()), this, SLOT(menu_editBookmarks()));
	_bookmarksMenu->addAction(act);
	_actionEditBookmarks = act;
	_bookmarksMenu->addSeparator();
	_bookmarkActionGroup = new QActionGroup(this);

	// window submenu
	QMenu* windowMenu = menu->addMenu("&Window");
	act = new QAction("Output", this);
	connect(act, SIGNAL(triggered()), this, SLOT(menu_showOutput()));
	windowMenu->addAction(act);

	act = new QAction("Attribute Editor", this);
	connect(act, SIGNAL(triggered()), this, SLOT(menu_showAttributeEditor()));
	windowMenu->addAction(act);

#if USE_CURVE_EDITOR
	act = new QAction("Curve Editor", this);
	connect(act, SIGNAL(triggered()), this, SLOT(menu_showCurveEditor()));
	windowMenu->addAction(act);
#endif

	// help submenu
	QMenu* helpMenu = menu->addMenu("&Help");

	act = new QAction("Documentation", this);
	connect(act, SIGNAL(triggered()), this, SLOT(menu_opendoc()));
	helpMenu->addAction(act);

	act = new QAction("&About", this);
	connect(act, SIGNAL(triggered()), this, SLOT(menu_about()));
	helpMenu->addAction(act);
}

void AppMainWindow::InitToolbar()
{
	if (_connectionMode != 1)
	{
#ifndef NV_ARTISTTOOLS
		_mainToolbar = new BlastToolbar(ui.centralWidget);
		ui.verticalLayout->insertWidget(0, _mainToolbar);
		connect(_mainToolbar->getUI().btnFileOpen, SIGNAL(clicked()), this, SLOT(menu_openProject()));
#else
		CoreLib::Inst()->AppMainWindow_InitToolbar(ui.centralWidget, ui.verticalLayout);
#endif // NV_ARTISTTOOLS
	}
}

void AppMainWindow::InitPluginTab()
{
#ifndef NV_ARTISTTOOLS
	QWidget *tabFur;
	QGridLayout *gridLayout;
	QFrame *furMaterialEditorArea;
	QVBoxLayout *furMaterialEditorAreaLayout;
	QScrollArea *furScrollArea;
	QWidget *furScrollAreaContents;
	QVBoxLayout *furScrollAreaLayout;
	QSpacerItem *verticalSpacer;

	tabFur = new QWidget();
	tabFur->setObjectName(QStringLiteral("tabFur"));
	gridLayout = new QGridLayout(tabFur);
	gridLayout->setSpacing(6);
	gridLayout->setContentsMargins(11, 11, 11, 11);
	gridLayout->setObjectName(QStringLiteral("gridLayout"));
	gridLayout->setContentsMargins(0, 0, 0, 0);
	furMaterialEditorArea = new QFrame(tabFur);
	furMaterialEditorArea->setObjectName(QStringLiteral("furMaterialEditorArea"));
	furMaterialEditorAreaLayout = new QVBoxLayout(furMaterialEditorArea);
	furMaterialEditorAreaLayout->setSpacing(6);
	furMaterialEditorAreaLayout->setContentsMargins(11, 11, 11, 11);
	furMaterialEditorAreaLayout->setObjectName(QStringLiteral("furMaterialEditorAreaLayout"));
	furMaterialEditorAreaLayout->setContentsMargins(2, 2, 2, 2);

	gridLayout->addWidget(furMaterialEditorArea, 1, 0, 1, 1);

	furScrollArea = new QScrollArea(tabFur);
	furScrollArea->setObjectName(QStringLiteral("furScrollArea"));
	furScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	furScrollArea->setWidgetResizable(true);
	furScrollAreaContents = new QWidget();
	furScrollAreaContents->setObjectName(QStringLiteral("furScrollAreaContents"));
	furScrollAreaContents->setGeometry(QRect(0, 0, 359, 481));
	furScrollAreaLayout = new QVBoxLayout(furScrollAreaContents);
	furScrollAreaLayout->setSpacing(3);
	furScrollAreaLayout->setContentsMargins(11, 11, 11, 11);
	furScrollAreaLayout->setObjectName(QStringLiteral("furScrollAreaLayout"));
	furScrollAreaLayout->setContentsMargins(2, 2, 2, 2);
	verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

	furScrollAreaLayout->addItem(verticalSpacer);

	furScrollArea->setWidget(furScrollAreaContents);

	gridLayout->addWidget(furScrollArea, 0, 0, 1, 1);

	ui.sideBarTab->addTab(tabFur, QString());

	ui.sideBarTab->setTabText(ui.sideBarTab->indexOf(tabFur), QApplication::translate("AppMainWindowClass", "Hair", 0));

	ExpandablePanel* panel = 0;
	int pannelCnt = 0;

	if (_connectionMode == 0)
	{
		panel = new ExpandablePanel(furScrollAreaContents, false);
		_generalAttributePanel = new GeneralAttributePanel(panel);
		panel->AddContent(_generalAttributePanel);
		furScrollAreaLayout->insertWidget(pannelCnt++, panel);
		panel->SetTitle("Asset Selection");

		panel = new ExpandablePanel(furScrollAreaContents, false);
		_assetControlPanel = new AssetControlPanel(panel);
		panel->AddContent(_assetControlPanel);
		furScrollAreaLayout->insertWidget(pannelCnt++, panel);
		panel->SetTitle("General Settings");

	}

	panel = new ExpandablePanel(furScrollAreaContents);
	_displayFurVisualizersPanel = new DisplayFurVisualizersPanel(panel);
	panel->AddContent(_displayFurVisualizersPanel);
	furScrollAreaLayout->insertWidget(pannelCnt++, panel);
	panel->SetTitle("Visualization");

	panel = new ExpandablePanel(furScrollAreaContents);
	_physicalMaterialPanel = new PhysicalMaterialPanel(panel);
	panel->AddContent(_physicalMaterialPanel);
	furScrollAreaLayout->insertWidget(pannelCnt++, panel);
	panel->SetTitle("Physical");

	panel = new ExpandablePanel(furScrollAreaContents);
	_styleMaterialPanel = new StyleMaterialPanel(panel);
	panel->AddContent(_styleMaterialPanel);
	furScrollAreaLayout->insertWidget(pannelCnt++, panel);
	panel->SetTitle("Style");

	panel = new ExpandablePanel(furScrollAreaContents);
	_graphicalMaterialPanel = new GraphicalMaterialPanel(panel);
	panel->AddContent(_graphicalMaterialPanel);
	furScrollAreaLayout->insertWidget(pannelCnt++, panel);
	panel->SetTitle("Graphics");

	panel = new ExpandablePanel(furScrollAreaContents);
	_lodPanel = new LodPanel(panel);
	panel->AddContent(_lodPanel);
	furScrollAreaLayout->insertWidget(pannelCnt++, panel);
	panel->SetTitle("Level Of Detail");

#if USE_MATERIAL_SET
	panel = new ExpandablePanel(furScrollAreaContents);
	_materialSetPanel = new MaterialSetPanel(panel);
	panel->AddContent(_materialSetPanel);
	furScrollAreaLayout->insertWidget(pannelCnt++, panel);
	panel->SetTitle("Control Texture Channels");
	//_materialSetPanel->hide();
	//panel->SetCollapsed(true);
#else
	ui.furMaterialEditorAreaLayout->setEnabled(false);
#endif
#else
	CoreLib::Inst()->AppMainWindow_InitPluginTab(ui.sideBarTab);
#endif // NV_ARTISTTOOLS

#if USE_CURVE_EDITOR
	_curveEditorMainWindow = new nvidia::CurveEditor::CurveEditorMainWindow( this);//nullptr);
	_curveEditorMainWindow->setResampleEnabled(false);
	ui.dockWidgetCurveEditor->setWidget(_curveEditorMainWindow);
	ui.dockWidgetCurveEditor->setWindowTitle(tr("Curve Editor"));

	connect(_curveEditorMainWindow, SIGNAL(CurveAttributeChanged(nvidia::CurveEditor::CurveAttribute*)), this, SLOT(onCurveAttributeChanged(nvidia::CurveEditor::CurveAttribute*)));
	connect(_curveEditorMainWindow, SIGNAL(ColorAttributeChanged(nvidia::CurveEditor::ColorAttribute*)), this, SLOT(onColorAttributeChanged(nvidia::CurveEditor::ColorAttribute*)));
	connect(_curveEditorMainWindow, SIGNAL(ReloadColorAttributeTexture(nvidia::CurveEditor::ColorAttribute*, bool, int)), this, SLOT(onReloadColorAttributeTexture(nvidia::CurveEditor::ColorAttribute*, bool, int)));
//#else
//	ui.furCurveEditorAreaLayout->setEnabled(false);
#endif
}

void AppMainWindow::InitMainTab()
{
	int idx = 0;

	if (_connectionMode != 1)
	{
		ExpandablePanel* panel = new ExpandablePanel(ui.displayScrollAreaContents);
		_displayPreferencesPanel = new DisplayPreferencesPanel(panel);
		panel->AddContent(_displayPreferencesPanel);
		ui.displayScrollAreaLayout->insertWidget(idx++, panel);
		panel->SetTitle("Preferences");

		panel = new ExpandablePanel(ui.displayScrollAreaContents);
		_displayScenePanel = new DisplayScenePanel(panel);
		panel->AddContent(_displayScenePanel);
		ui.displayScrollAreaLayout->insertWidget(idx++, panel);
		panel->SetTitle("Scene");

		panel = new ExpandablePanel(ui.displayScrollAreaContents);
		_displayLightPanel = new DisplayLightPanel(panel);
		panel->AddContent(_displayLightPanel);
		ui.displayScrollAreaLayout->insertWidget(idx++, panel);
		panel->SetTitle("Light");

		panel = new ExpandablePanel(ui.displayScrollAreaContents);
		_displayMeshesPanel = new DisplayMeshesPanel(panel);
		panel->AddContent(_displayMeshesPanel);
		ui.displayScrollAreaLayout->insertWidget(idx++, panel);
		panel->SetTitle("Display Meshes");
	}

	if (_connectionMode != 1)
	{
#ifndef NV_ARTISTTOOLS
		ExpandablePanel* panel = new ExpandablePanel(ui.displayScrollAreaContents);
		_displayMeshMaterialsPanel = new DisplayMeshMaterialsPanel(panel);
		panel->AddContent(_displayMeshMaterialsPanel);
		ui.displayScrollAreaLayout->insertWidget(idx++, panel);
		panel->SetTitle("Display Mesh Materials");
#else
		CoreLib::Inst()->AppMainWindow_InitMainTab(ui.displayScrollAreaContents, ui.displayScrollAreaLayout, idx);
#endif // NV_ARTISTTOOLS
	}
}

void AppMainWindow::InitShortCuts()
{
	QShortcut* shortCut;
	shortCut = new QShortcut(QKeySequence(Qt::Key_F), this);
	connect(shortCut, SIGNAL(activated()), this, SLOT(shortcut_frameall()));

	shortCut = new QShortcut(QKeySequence(Qt::Key_H), this);
	connect(shortCut, SIGNAL(activated()), this, SLOT(shortcut_hud()));

	shortCut = new QShortcut(QKeySequence(Qt::Key_G), this);
	connect(shortCut, SIGNAL(activated()), this, SLOT(shortcut_statistics()));

	shortCut = new QShortcut(QKeySequence(Qt::Key_Space), this);
	connect(shortCut, SIGNAL(activated()), this, SLOT(shortcut_pause()));

	shortCut = new QShortcut(QKeySequence(Qt::Key_B), this);
	connect(shortCut, SIGNAL(activated()), this, SLOT(shortcut_reset()));

	shortCut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
	connect(shortCut, SIGNAL(activated()), this, SLOT(shortcut_escape()));

	shortCut = new QShortcut(QKeySequence(Qt::Key_V), this);
	connect(shortCut, SIGNAL(activated()), this, SLOT(shortcut_expert()));

	shortCut = new QShortcut(QKeySequence("Ctrl+X"), this);
	connect(shortCut, SIGNAL(activated()), this, SLOT(shortcut_expert()));

	shortCut = new QShortcut(QKeySequence(Qt::Key_D), this);
	connect(shortCut, SIGNAL(activated()), this, SLOT(demo_next()));

	shortCut = new QShortcut(QKeySequence(Qt::Key_Right), this);
	connect(shortCut, SIGNAL(activated()), this, SLOT(demo_next()));

	shortCut = new QShortcut(QKeySequence(Qt::Key_A), this);
	connect(shortCut, SIGNAL(activated()), this, SLOT(demo_prev()));

	shortCut = new QShortcut(QKeySequence(Qt::Key_Left), this);
	connect(shortCut, SIGNAL(activated()), this, SLOT(demo_prev()));

	shortCut = new QShortcut(QKeySequence(Qt::Key_F2), this);
	connect(shortCut, SIGNAL(activated()), this, SLOT(shortcut_output()));

	shortCut = new QShortcut(QKeySequence("Ctrl+F"), this);
	connect(shortCut, SIGNAL(activated()), this, SLOT(shortcut_fitcamera()));
}

//////////////////////////////////////////////////////////////////////////
void AppMainWindow::updateBookmarkMenu()
{
	// clear old menu items of camera bookmarks
	while (_bookmarkActionGroup->actions().size() > 0)
	{
		QAction* act = _bookmarkActionGroup->actions()[0];
		_bookmarkActionGroup->removeAction(act);
		delete act;
	}
	_bookmarksMenu->clear();
	_bookmarksMenu->addAction(_actionAddBookmark);
	_bookmarksMenu->addAction(_actionEditBookmarks);
	_bookmarksMenu->addSeparator();
	
	QList<QString> bookmarks = SimpleScene::Inst()->getBookmarkNames();
	int numBookmarks = bookmarks.size();
	for (int i = 0; i < numBookmarks; ++i)
	{
		QString bookmark = bookmarks[i];

		QAction* act = new QAction(bookmark, this);
		act->setCheckable(true);
		_bookmarksMenu->addAction(act);
		_bookmarkActionGroup->addAction(act);
	}

}

//////////////////////////////////////////////////////////////////////////
void AppMainWindow::updateUI()
{
	if (_bookmarksMenu && _bookmarkActionGroup)
	{
		updateBookmarkMenu();
	}

#ifndef NV_ARTISTTOOLS
	if (_mainToolbar)
		_mainToolbar->updateValues();

	if (_graphicalMaterialPanel)
		_graphicalMaterialPanel->updateValues();

	if (_physicalMaterialPanel)
		_physicalMaterialPanel->updateValues();

	if (_styleMaterialPanel)
		_styleMaterialPanel->updateValues();

	if (_displayFurVisualizersPanel)
		_displayFurVisualizersPanel->updateValues();

	if (_lodPanel)
		_lodPanel->updateValues();

	if (_generalAttributePanel)
		_generalAttributePanel->updateValues();

	if (_assetControlPanel)
		_assetControlPanel->updateValues();

	if (_displayMeshMaterialsPanel)
		_displayMeshMaterialsPanel->updateValues();

#if USE_MATERIAL_SET
	if (_materialSetPanel)
		_materialSetPanel->updateValues();
#endif
#else
	CoreLib::Inst()->AppMainWindow_updateUI();
#endif // NV_ARTISTTOOLS

	if (_displayPreferencesPanel)
		_displayPreferencesPanel->updateValues();

	if (_displayScenePanel)
		_displayScenePanel->updateValues();

	if (_displayLightPanel)	
		_displayLightPanel->updateValues();

	if (_displayMeshesPanel)	
		_displayMeshesPanel->updateValues();

#if USE_CURVE_EDITOR
	if (_curveEditorMainWindow)
		UpdateCurveEditor();
#endif
}

//////////////////////////////////////////////////////////////////////////
void AppMainWindow::updatePluginUI()
{
	SimpleScene::Inst()->setIsUpdatingUI(true);

#ifndef NV_ARTISTTOOLS
	if (_graphicalMaterialPanel)
		_graphicalMaterialPanel->updateValues();

	if (_physicalMaterialPanel)
		_physicalMaterialPanel->updateValues();

	if (_styleMaterialPanel)
		_styleMaterialPanel->updateValues();

	if (_lodPanel)
		_lodPanel->updateValues();

	if (_displayFurVisualizersPanel)
		_displayFurVisualizersPanel->updateValues();

	if (_generalAttributePanel)
		_generalAttributePanel->updateValues();

	if (_assetControlPanel)
		_assetControlPanel->updateValues();

#if USE_MATERIAL_SET
	if (_materialSetPanel)
		_materialSetPanel->updateValues();
#endif
#else
	CoreLib::Inst()->AppMainWindow_updatePluginUI();
#endif // NV_ARTISTTOOLS

#if USE_CURVE_EDITOR
	if (_curveEditorMainWindow)
		UpdateCurveEditor();
#endif

	SimpleScene::Inst()->setIsUpdatingUI(false);
}


//////////////////////////////////////////////////////////////////////////
// event handlers


//////////////////////////////////////////////////////////////////////////
// event handlers
bool AppMainWindow::openProject(QString fileName)
{
	if (!fileName.isEmpty())
	{
		QFileInfo fileInfo(fileName);
		QByteArray dir = QDir::toNativeSeparators(fileInfo.absoluteDir().absolutePath()).toLocal8Bit();
		QByteArray file= fileInfo.fileName().toLocal8Bit();

		if (SimpleScene::Inst()->LoadProject(dir, file) == false)
		{
			QMessageBox messageBox;
			messageBox.critical(0,"Error","File open error!");
			messageBox.setFixedSize(500,200);

			AppMainWindow::Inst().endProgress();
			char message[1024];
			sprintf(message, "Failed to open project file(\"%s\")", (const char*)file);
			viewer_err(message);
			return false;
		}

		AppMainWindow::Inst().endProgress();
		_lastFilePath = fileInfo.absoluteDir().absolutePath();

		updateUI();
		return true;
	}
	return false;
}

void AppMainWindow::processDragAndDrop(QString fname)
{
	openProject(fname);
}

void AppMainWindow::removeBookmark(const QString& name)
{
	QList<QAction*> bookmarks = _bookmarkActionGroup->actions();
	int bookmarkCount = bookmarks.size();
	for (int i = 0; i < bookmarkCount; ++i)
	{
		QAction* act = bookmarks.at(i);
		if (act->text() == name)
		{
			_bookmarkActionGroup->removeAction(act);
			_bookmarksMenu->removeAction(act);
			delete act;
		}
	}
}

void AppMainWindow::renameBookmark(const QString& oldName, const QString& newName)
{
	QList<QAction*> bookmarks = _bookmarkActionGroup->actions();
	int bookmarkCount = bookmarks.size();
	for (int i = 0; i < bookmarkCount; ++i)
	{
		QAction* act = bookmarks.at(i);
		if (act->text() == oldName)
		{
			act->setText(newName);
		}
	}
}

#if USE_CURVE_EDITOR
void AppMainWindow::UpdateCurveEditor()
{
#ifndef NV_ARTISTTOOLS
	_curveEditorMainWindow->setCurveAttributes(SimpleScene::Inst()->GetFurCharacter().GetCurveAttributes());
	_curveEditorMainWindow->setColorCurveAttributes(SimpleScene::Inst()->GetFurCharacter().GetColorAttributes());
#else
	CoreLib::Inst()->AppMainWindow_UpdateCurveEditor();
#endif // NV_ARTISTTOOLS
	
	_curveEditorMainWindow->update();
}

void AppMainWindow::ShowCurveEditor(int paramId)
{
#ifndef NV_ARTISTTOOLS	
	_curveEditorMainWindow->setCurveAttributes(SimpleScene::Inst()->GetFurCharacter().GetCurveAttributes());
	_curveEditorMainWindow->setColorCurveAttributes(SimpleScene::Inst()->GetFurCharacter().GetColorAttributes());
	if (HAIR_PARAMS_ROOT_COLOR == paramId || HAIR_PARAMS_TIP_COLOR == paramId)
	{
		std::vector<nvidia::CurveEditor::ColorAttribute*> attributes = SimpleScene::Inst()->GetFurCharacter().GetColorAttributesByParamId(paramId);
		_curveEditorMainWindow->setSelectedColorAttribute(attributes.size() > 0 ? attributes[0] : nullptr);
	}
	else
		_curveEditorMainWindow->setSelectedCurveAttributes(SimpleScene::Inst()->GetFurCharacter().GetCurveAttributesByParamId(paramId));

#else
	CoreLib::Inst()->AppMainWindow_ShowCurveEditor(paramId);
#endif // NV_ARTISTTOOLS

	_curveEditorMainWindow->update();
}
#endif

void AppMainWindow::menu_clearScene()
{
	SimpleScene::Inst()->Clear();
	updateUI();
}

bool AppMainWindow::menu_openfbx()
{
	/*
	QString lastDir = _lastFilePath;
	QString fileName = QFileDialog::getOpenFileName(this, "Open FBX File", lastDir, "FBX File (*.fbx)");
	if (!fileName.isEmpty())
	{
		QFileInfo fileInfo(fileName);
		QByteArray dir = QDir::toNativeSeparators(fileInfo.absoluteDir().absolutePath()).toLocal8Bit();
		QByteArray file= fileInfo.fileName().toLocal8Bit();

		if (SimpleScene::Inst()->LoadSceneFromFbx(dir, file) == false)
		{
			QMessageBox messageBox;
			messageBox.critical(0,"Error","File open error!");
			messageBox.setFixedSize(500,200);

			AppMainWindow::Inst().endProgress();

			char message[1024];
			sprintf(message, "Failed to open fbx file(\"%s\")", (const char*)file);
			viewer_err(message);

			return false;
		}

		AppMainWindow::Inst().endProgress();

		_lastFilePath = fileInfo.absoluteDir().absolutePath();

		updateUI();

		return true;
	}
	return true;
	*/

	// dir and file will get in blast open asset dialog
	return SimpleScene::Inst()->LoadSceneFromFbx("", "");
}

void AppMainWindow::menu_addBookmark()
{
	QString bookmark = SimpleScene::Inst()->createBookmark();

	QAction* act = new QAction(bookmark, this);
	act->setCheckable(true);
	_bookmarksMenu->addAction(act);
	_bookmarkActionGroup->addAction(act);
	act->setChecked(true);
}

void AppMainWindow::menu_editBookmarks()
{
	CameraBookmarksDialog dlg(this);
	dlg.exec(); 
}

void AppMainWindow::menu_bookmarkTriggered(QAction* act)
{
	if (_actionAddBookmark != act && _actionEditBookmarks != act)
	{
		SimpleScene::Inst()->activateBookmark(act->text());
	}
}

void AppMainWindow::menu_showOutput()
{
	bool bVisibal = ui.dockOutputWindow->isVisible();
	ui.dockOutputWindow->setVisible(!bVisibal);
}

void AppMainWindow::menu_showAttributeEditor()
{
	bool bVisibal = ui.dockWidget->isVisible();
	ui.dockWidget->setVisible(!bVisibal);
}
#if USE_CURVE_EDITOR
void AppMainWindow::menu_showCurveEditor()
{
	bool bVisibal = ui.dockWidgetCurveEditor->isVisible();
	ui.dockWidgetCurveEditor->setVisible(!bVisibal);
}
#endif
void AppMainWindow::menu_about()
{
	qDebug("%s", __FUNCTION__);
#ifndef NV_ARTISTTOOLS
	AboutDialog::ShowAboutDialog();
#else
	CoreLib::Inst()->AppMainWindow_menu_about();
#endif // NV_ARTISTTOOLS
}

void AppMainWindow::menu_opendoc()
{
	qDebug("%s", __FUNCTION__);

#ifndef NV_ARTISTTOOLS
	QString appDir = QApplication::applicationDirPath();
	QString docsFile = QFileInfo(appDir + "/../../docs/User_Guide/Nvidia Blast.chm").absoluteFilePath();

	QUrl docsUrl = QUrl::fromLocalFile(docsFile);
	QUrl url = QUrl::fromUserInput(QString("http://docs.nvidia.com/gameworks/content/artisttools/hairworks/index.html"));
	QDesktopServices::openUrl(url);
#else
	CoreLib::Inst()->AppMainWindow_menu_opendoc();
#endif // NV_ARTISTTOOLS
}

void AppMainWindow::shortcut_frameall()
{
	qDebug("ShortCut_F: frame all");
}

void AppMainWindow::shortcut_hud()
{
	Gamepad::ShowHideHud();
	qDebug("ShortCut_S: statistics on/off");
}

void AppMainWindow::shortcut_statistics()
{
	Gamepad::ShowHideStats();
	qDebug("ShortCut_S: statistics on/off");
}

void AppMainWindow::shortcut_reset()
{
	Gamepad::ResetAnimation();
}

void AppMainWindow::shortcut_pause()
{
	//_mainToolbar->on_btnPlayAnimation_clicked();  // this one has some delay
	//qDebug("ShortCut_Space: play/pause simualtion");
	Gamepad::PlayPauseAnimation();
}

bool AppMainWindow::IsExpertMode()
{
	return _expertMode;
}

void AppMainWindow::demo_next()
{
	Gamepad::DemoNext();
}

void AppMainWindow::demo_prev()
{
	Gamepad::DemoPrev();
}

void AppMainWindow::shortcut_escape()
{
	Gamepad::DemoEscape();
}

void AppMainWindow::shortcut_expert()
{
	qDebug("ShortCut_F: expert mode on/off");

	_expertMode = !_expertMode;
	bool mode = !_expertMode;

	ui.menuBar->setVisible(mode);
	ui.dockWidget->setVisible(mode);
	ui.dockOutputWindow->setVisible(mode);
#if USE_CURVE_EDITOR
	ui.dockWidgetCurveEditor->setVisible(mode);
#endif
	ui.statusBar->setVisible(mode);

#ifndef NV_ARTISTTOOLS
	if (_mainToolbar)
		_mainToolbar->setVisible(mode);
#else
	CoreLib::Inst()->AppMainWindow_shortcut_expert(mode);
#endif // NV_ARTISTTOOLS

	//bool bDemoMode = AppMainWindow::IsExpertMode();
	bool bMaxSized = AppMainWindow::Inst().isMaximized();
	if (!bMaxSized)
	{
		// resize before change to demo mode
		AppMainWindow::Inst().showMaximized();
	}

	_d3dWidget->update();
	Gamepad::ShowProjectName();
}

void AppMainWindow::shortcut_output()
{
	ui.dockOutputWindow->show();
	_d3dWidget->update();
}

void AppMainWindow::shortcut_meshmat()
{
}

void AppMainWindow::shortcut_fitcamera()
{
	qDebug("ShortCut_Ctrl+F: fit camera");
	SimpleScene::Inst()->FitCamera();
}

#if USE_CURVE_EDITOR
void AppMainWindow::onCurveAttributeChanged(nvidia::CurveEditor::CurveAttribute* attribute)
{
#ifndef NV_ARTISTTOOLS
	SimpleScene::Inst()->GetFurCharacter().updateCurveAttribute(attribute);
#else
	CoreLib::Inst()->AppMainWindow_onCurveAttributeChanged(attribute);
#endif // NV_ARTISTTOOLS
}

void AppMainWindow::onColorAttributeChanged(nvidia::CurveEditor::ColorAttribute* attribute)
{
#ifndef NV_ARTISTTOOLS
	SimpleScene::Inst()->GetFurCharacter().updateColorAttribute(attribute);
	_graphicalMaterialPanel->updateValues();
#else
	CoreLib::Inst()->AppMainWindow_onColorAttributeChanged(attribute);
#endif // NV_ARTISTTOOLS
}

void AppMainWindow::onReloadColorAttributeTexture(nvidia::CurveEditor::ColorAttribute* attribute, bool reloadColorTex, int selectedCtrlPntIndex)
{
#ifndef NV_ARTISTTOOLS
	SimpleScene::Inst()->GetFurCharacter().reloadColorAttributeTexture(attribute, reloadColorTex, selectedCtrlPntIndex);
#else
	CoreLib::Inst()->AppMainWindow_onReloadColorAttributeTexture(attribute, reloadColorTex, selectedCtrlPntIndex);
#endif // NV_ARTISTTOOLS
}
#endif

/*
	Maya Scheme:
		ALT + LMB -> Rotate
		ALT + MMB -> Pan
		ALT + RMB -> Zoom

		M-wheel -> Zoom

	3dsMax Scheme:
		ALT + MMB -> Rotate
		N/A + MMB -> Pan
		ALT + SHFT + MMB -> Zoom

		M-wheel -> Zoom
*/

void AppMainWindow::InitMouseSchemes()
{
	ShortCut alt_lmb = qMakePair(Qt::KeyboardModifiers(Qt::AltModifier), Qt::MouseButtons(Qt::LeftButton));
	ShortCut alt_mmb = qMakePair(Qt::KeyboardModifiers(Qt::AltModifier), Qt::MouseButtons(Qt::MiddleButton));
	ShortCut alt_rmb = qMakePair(Qt::KeyboardModifiers(Qt::AltModifier), Qt::MouseButtons(Qt::RightButton));
	ShortCut non_mmb = qMakePair(Qt::KeyboardModifiers(Qt::NoModifier), Qt::MouseButtons(Qt::MiddleButton));
	ShortCut alt_shft_mmb = qMakePair(Qt::KeyboardModifiers(Qt::AltModifier|Qt::ShiftModifier), Qt::MouseButtons(Qt::MiddleButton));

	_mayaScheme[alt_lmb] = 'R';
	_mayaScheme[alt_mmb] = 'P';
	_mayaScheme[alt_rmb] = 'Z';

	_maxScheme[alt_mmb] = 'R';
	_maxScheme[non_mmb] = 'P';
	_maxScheme[alt_shft_mmb] = 'Z';
}

char AppMainWindow::TestMouseScheme( Qt::KeyboardModifiers modifiers, Qt::MouseButtons buttons )
{
	char op = TestDragCamera(modifiers, buttons);

	if (op != 0)
		return op;

	if (modifiers == Qt::KeyboardModifier(Qt::ControlModifier))
	{
		if (buttons == Qt::MouseButton(Qt::LeftButton))
			return 'L';
		else if (buttons == Qt::MouseButton(Qt::MiddleButton))
			return 'K';
	}

	if (modifiers == Qt::KeyboardModifier(Qt::ShiftModifier))
		return 'W';	

	return 0;
}

char AppMainWindow::TestDragCamera( Qt::KeyboardModifiers modifiers, Qt::MouseButtons buttons )
{
	if(modifiers != Qt::NoModifier && modifiers != Qt::AltModifier) return 0;

	ShortCut input = qMakePair(modifiers, buttons);

	int scheme = _navigationStyle;

	// !! MUST MATCH THE ORDER OF ITEMS IN 'cbNavigationStyle'
	const int MAYA_SCHEME = 0;
	const int MAX_SCHEME = 1;
	if(scheme == MAYA_SCHEME)
	{
		auto itr = _mayaScheme.find(input);
		if(itr != _mayaScheme.end()) return itr.value();
	}
	else
	{		
		auto itr = _maxScheme.find(input);
		if(itr != _maxScheme.end()) return itr.value();
	}
	return 0;
}

QString AppMainWindow::OpenTextureFile(QString title)
{
	QString lastDir = _lastFilePath;
	QString titleStr = "Open Texture File";
	if(!title.isEmpty())
		titleStr = title;

	QString fileName = QFileDialog::getOpenFileName(this, titleStr, lastDir, "Images (*.dds *.png *.bmp *.jpg *.tga)");
	if(!fileName.isEmpty())
	{
		QFileInfo fileInfo(fileName);
		_lastFilePath = fileInfo.absoluteDir().absolutePath();
	}

	return fileName;
}

void AppMainWindow::closeEvent (QCloseEvent *event)
{
	ViewerOutput::Inst().UnRegisterPrinter(_printer);

	if (1)
	{
#if USE_CURVE_EDITOR
		_curveEditorMainWindow->setParent(NULL);
#endif
		event->accept();
		emit aboutToQuit();
		return;
	}
	if (SimpleScene::Inst()->IsProjectModified() || SimpleScene::Inst()->IsFurModified())
	{
		QMessageBox::StandardButton resBtn = QMessageBox::warning(
			this, tr("Blast Viewer"),
			tr("Save changes?\n"),
			QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
			QMessageBox::Yes);
		switch (resBtn)
		{
		case QMessageBox::Yes:
#ifndef NV_ARTISTTOOLS
			if (!menu_saveHair())
			{
				event->ignore();
				return;
			}
			if (!menu_saveProject())
			{
				event->ignore();
				return;
			}
#else
			if (!CoreLib::Inst()->AppMainWindow_closeEvent(event))
			{
				event->ignore();
				return;
			}
#endif // NV_ARTISTTOOLS
			break;
		case QMessageBox::No:
			break;
		default:
			event->ignore();
			return;
		}
	}
	event->accept();
	emit aboutToQuit();
}

void AppMainWindow::updateMainToolbar()
{
#ifndef NV_ARTISTTOOLS
	if (_mainToolbar)
		_mainToolbar->updateValues();
#else
	CoreLib::Inst()->AppMainWindow_updateMainToolbar();
#endif // NV_ARTISTTOOLS
}

#ifndef NV_ARTISTTOOLS
bool AppMainWindow::menu_openProject()
{
	QString lastDir = _lastFilePath;
	QString fileName = QFileDialog::getOpenFileName(this, "Open Hair Project File", lastDir, "Hair Project File (*.furproj)");

	return openProject(fileName);
}

bool AppMainWindow::menu_saveProject()
{
	char message[1024];

	std::string projectFilePath = GlobalSettings::Inst().getAbsoluteFilePath();
	if (projectFilePath != "")
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

bool AppMainWindow::menu_saveProjectAs()
{
	char message[1024];

	QString lastDir = _lastFilePath;
	QString fileName = QFileDialog::getSaveFileName(this, "Save Hair Project File", lastDir, "Hair Project File (*.furproj)");
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

		_lastFilePath = fileInfo.absoluteDir().absolutePath();
		return true;
	}
	return false;
}

bool AppMainWindow::menu_openHair()
{
	AppMainWindow& window = AppMainWindow::Inst();
	QString lastDir = window._lastFilePath;
	QString fileName = QFileDialog::getOpenFileName(&window, "Open Hair File", lastDir, "Apex Hair File (*.apx;*.apb)");
	if (!fileName.isEmpty())
	{
		QFileInfo fileInfo(fileName);
		QByteArray dir = QDir::toNativeSeparators(fileInfo.absoluteDir().absolutePath()).toLocal8Bit();
		QByteArray file = fileInfo.fileName().toLocal8Bit();

		if (SimpleScene::Inst()->GetFurCharacter().LoadHairAsset(dir, file) == false)
		{
			QMessageBox messageBox;
			messageBox.critical(0, "Error", "File open error!");
			messageBox.setFixedSize(500, 200);
			char message[1024];
			sprintf(message, "Failed to open hair file(\"%s\")", (const char*)file);
			viewer_err(message);
			return false;
		}

		window._lastFilePath = fileInfo.absoluteDir().absolutePath();

		window.updateUI();
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
// event handlers
bool AppMainWindow::menu_importHair()
{
	AppMainWindow& window = AppMainWindow::Inst();
	QString lastDir = window._lastFilePath;
	QString fileName = QFileDialog::getOpenFileName(&window, "Import Hair File", lastDir, "Apex Hair File (*.apx;*.apb)");
	if (!fileName.isEmpty())
	{
		QFileInfo fileInfo(fileName);
		QByteArray dir = QDir::toNativeSeparators(fileInfo.absoluteDir().absolutePath()).toLocal8Bit();
		QByteArray file = fileInfo.fileName().toLocal8Bit();

		if (SimpleScene::Inst()->GetFurCharacter().ImportSelectedHairAsset(dir, file) == false)
		{
			QMessageBox messageBox;
			messageBox.critical(0, "Error", "File open error!");
			messageBox.setFixedSize(500, 200);
			char message[1024];
			sprintf(message, "Failed to import hair file(\"%s\")", (const char*)file);
			viewer_err(message);
			return false;
		}

		window._lastFilePath = fileInfo.absoluteDir().absolutePath();

		window.updateUI();
		return true;
	}
	return false;
}


bool AppMainWindow::menu_saveHair()
{
	return SimpleScene::Inst()->GetFurCharacter().SaveHairAsset();
}

bool AppMainWindow::menu_saveHairAs()
{
	AppMainWindow& window = AppMainWindow::Inst();
	QString lastDir = window._lastFilePath;
	QString fileName = QFileDialog::getSaveFileName(&window, "Save as", lastDir, "Apex Hair File (*.apx;*.apb)");
	if (!fileName.isEmpty())
	{
		QFileInfo fileInfo(fileName);
		QByteArray dir = QDir::toNativeSeparators(fileInfo.absoluteDir().absolutePath()).toLocal8Bit();
		QByteArray file = fileInfo.fileName().toLocal8Bit();

		if (SimpleScene::Inst()->GetFurCharacter().SaveHairAssetAs(dir, file))
		{
			window.updateUI();
		}

		window._lastFilePath = fileInfo.absoluteDir().absolutePath();
		return true;
	}
	return false;
}

bool AppMainWindow::menu_saveAllHairs()
{
	return SimpleScene::Inst()->GetFurCharacter().SaveAllHairs();
}
#endif // NV_ARTISTTOOLS