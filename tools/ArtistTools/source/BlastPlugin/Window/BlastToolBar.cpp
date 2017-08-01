#include "BlastToolbar.h"

#include <QtWidgets/QFileDialog>
#include "AppMainWindow.h"
#include "PhysXController.h"
#include "QtUtil.h"

#include "SampleManager.h"
#include "SceneController.h"
#include "DamageToolController.h"
#include "SelectionToolController.h"
#include "ExplodeToolController.h"
#include "GizmoToolController.h"
#include "BlastController.h"
#include "SourceAssetOpenDlg.h"
#include "BlastPlugin.h"
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QActionGroup>
#include "Shlwapi.h"
#include "BlastSceneTree.h"
#include "BlastFamily.h"
#include "FileReferencesPanel.h"
#include "GlobalSettings.h"
#include "PxScene.h"
#include "PxRigidDynamic.h"
#include "ViewerOutput.h"

#include <QtCore/QTimer>
QTimer gDropTimer;
int nExplodedViewState = 0;

BlastToolbar::BlastToolbar(QWidget* parent)
	: QDockWidget(parent)
	, m_fullCoverage(false)
{
	// to hide the title bar completely must replace the default widget with a generic one 
	QWidget* titleWidget = new QWidget(this);
	this->setTitleBarWidget(titleWidget);
	this->setObjectName(QString::fromUtf8("AppMainToolbar"));
	this->setMinimumSize(QSize(0, 50));
	this->setMaximumSize(QSize(16777215, 80));
	
	if (this->objectName().isEmpty())
		this->setObjectName(QStringLiteral("AppMainToolbar"));
	this->resize(1330, 54);

	QWidget* widget = new QWidget();
	hLayout = new QHBoxLayout(widget);
	hLayout->setObjectName(QStringLiteral("hLayout"));
	hLayout->setContentsMargins(-1, 3, -1, 3);

	QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
	sizePolicy1.setHorizontalStretch(0);
	sizePolicy1.setVerticalStretch(0);

	btnOpenProject = new QPushButton(widget);
	setStyledToolTip(btnOpenProject, "Open Project");
	const QFont& font = btnOpenProject->font();
	QFont fontCopy(font);
	fontCopy.setPixelSize(9);

	btnOpenProject->setObjectName(QStringLiteral("btnOpenProject"));
	sizePolicy1.setHeightForWidth(btnOpenProject->sizePolicy().hasHeightForWidth());
	btnOpenProject->setSizePolicy(sizePolicy1);
	btnOpenProject->setMinimumSize(QSize(40, 40));
	btnOpenProject->setMaximumSize(QSize(40, 40));
	btnOpenProject->setText(QApplication::translate("AppMainToolbar", "Open", 0));
	hLayout->addWidget(btnOpenProject);

	btnSaveProject = new QPushButton(widget);
	setStyledToolTip(btnSaveProject, "Save Project and assets");
	btnSaveProject->setObjectName(QStringLiteral("btnSaveProject"));
	sizePolicy1.setHeightForWidth(btnOpenProject->sizePolicy().hasHeightForWidth());
	btnSaveProject->setSizePolicy(sizePolicy1);
	btnSaveProject->setMinimumSize(QSize(40, 40));
	btnSaveProject->setMaximumSize(QSize(40, 40));
	btnSaveProject->setText(QApplication::translate("AppMainToolbar", "Save\nAll", 0));
	hLayout->addWidget(btnSaveProject);

	btnExportAssets = new QPushButton(widget);
	setStyledToolTip(btnExportAssets, "Export Blast assets");
	btnExportAssets->setObjectName(QStringLiteral("btnExportAssets"));
	sizePolicy1.setHeightForWidth(btnExportAssets->sizePolicy().hasHeightForWidth());
	btnExportAssets->setSizePolicy(sizePolicy1);
	btnExportAssets->setMinimumSize(QSize(40, 40));
	btnExportAssets->setMaximumSize(QSize(40, 40));
	btnExportAssets->setText(QApplication::translate("AppMainToolbar", "Export", 0));
	hLayout->addWidget(btnExportAssets);

	vLayoutExport = new QVBoxLayout();
	vLayoutExport->setObjectName(QStringLiteral("vLayoutExport"));

	hLayoutExport = new QHBoxLayout();
	hLayoutExport->setObjectName(QStringLiteral("hLayoutExport"));

	lExportFilepath = new QLabel(widget);
	lExportFilepath->setObjectName(QStringLiteral("lExportFilepath"));
	lExportFilepath->setText(QApplication::translate("AppMainToolbar", "Export Path", 0));
	hLayoutExport->addWidget(lExportFilepath);

	btnExportFilepath = new QPushButton(widget);
	btnExportFilepath->setObjectName(QStringLiteral("btnExportFilepath"));
	sizePolicy1.setHeightForWidth(btnExportFilepath->sizePolicy().hasHeightForWidth());
	btnExportFilepath->setSizePolicy(sizePolicy1);
	btnExportFilepath->setMinimumSize(QSize(14, 14));
	btnExportFilepath->setMaximumSize(QSize(14, 14));
	btnExportFilepath->setText(QApplication::translate("AppMainToolbar", "", 0));
	btnExportFilepath->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnExportFilepath.png"));
	btnExportFilepath->setIconSize(QSize(14, 14));
	hLayoutExport->addWidget(btnExportFilepath);

	vLayoutExport->addLayout(hLayoutExport);
	
	leExportFilepath = new QLineEdit(widget);
	leExportFilepath->setObjectName(QStringLiteral("leExportFilepath"));
	sizePolicy1.setHeightForWidth(leExportFilepath->sizePolicy().hasHeightForWidth());
	leExportFilepath->setSizePolicy(sizePolicy1);
	leExportFilepath->setMinimumSize(QSize(150, 20));
	leExportFilepath->setMaximumSize(QSize(150, 20));
	leExportFilepath->setText(QApplication::translate("AppMainToolbar", "", 0));
	leExportFilepath->setEnabled(false); // do not allow direct change to make sure the folder exists.
	vLayoutExport->addWidget(leExportFilepath);

	hLayout->addLayout(vLayoutExport);

	fSeparate = new QFrame(widget);
	fSeparate->setObjectName(QStringLiteral("fSeparate"));
	fSeparate->setFrameShape(QFrame::VLine);
	fSeparate->setFrameShadow(QFrame::Sunken);
	hLayout->addWidget(fSeparate);

	vLayoutDepthCoverage = new QVBoxLayout();
	vLayoutDepthCoverage->setObjectName(QStringLiteral("vLayoutDepthCoverage"));

	hlDepthPreview = new QHBoxLayout();
	hlDepthPreview->setObjectName(QStringLiteral("hlDepthPreview"));

	lbDepthPreview = new QLabel(widget);
	lbDepthPreview->setObjectName(QStringLiteral("lbDepthPreview"));
	lbDepthPreview->setText(QApplication::translate("AppMainToolbar", "Depth Preview", 0));
	hlDepthPreview->addWidget(lbDepthPreview);

	ssbiDepthPreview = new SlideSpinBoxInt(widget);
	ssbiDepthPreview->setObjectName(QStringLiteral("ssbiDepthPreview"));
	QSizePolicy sizePolicy2(QSizePolicy::Minimum, QSizePolicy::Fixed);
	sizePolicy2.setHorizontalStretch(0);
	sizePolicy2.setVerticalStretch(0);
	sizePolicy2.setHeightForWidth(ssbiDepthPreview->sizePolicy().hasHeightForWidth());
	ssbiDepthPreview->setSizePolicy(sizePolicy2);
	ssbiDepthPreview->setMinimumSize(QSize(40, 20));
	ssbiDepthPreview->setMaximumSize(QSize(100, 16777215));
	hlDepthPreview->addWidget(ssbiDepthPreview);

	vLayoutDepthCoverage->addLayout(hlDepthPreview);

	hlExactCoverage = new QHBoxLayout();
	hlExactCoverage->setObjectName(QStringLiteral("hlExactCoverage"));

	lbExactCoverage = new QLabel(widget);
	lbExactCoverage->setObjectName(QStringLiteral("lbExactCoverage"));
	lbExactCoverage->setText(QApplication::translate("AppMainToolbar", "Full Coverage", 0));
	hlExactCoverage->addWidget(lbExactCoverage);

	cbExactCoverage = new QCheckBox(widget);
	cbExactCoverage->setObjectName(QStringLiteral("cbExactCoverage"));
	sizePolicy1.setHeightForWidth(cbExactCoverage->sizePolicy().hasHeightForWidth());
	cbExactCoverage->setSizePolicy(sizePolicy1);
	cbExactCoverage->setLayoutDirection(Qt::RightToLeft);
	hlExactCoverage->addWidget(cbExactCoverage);

	vLayoutDepthCoverage->addLayout(hlExactCoverage);

	hLayout->addLayout(vLayoutDepthCoverage);

	fSeparate = new QFrame(widget);
	fSeparate->setObjectName(QStringLiteral("fSeparate"));
	fSeparate->setFrameShape(QFrame::VLine);
	fSeparate->setFrameShadow(QFrame::Sunken);
	hLayout->addWidget(fSeparate);

	QButtonGroup* gizmoGroup = new QButtonGroup(this);

	btnSelectTool = new QPushButton(widget);
	setStyledToolTip(btnSelectTool, "Switch to Selection Mode");
	btnSelectTool->setObjectName(QStringLiteral("btnSelectTool"));
	sizePolicy1.setHeightForWidth(btnSelectTool->sizePolicy().hasHeightForWidth());
	btnSelectTool->setSizePolicy(sizePolicy1);
	btnSelectTool->setMinimumSize(QSize(40, 40));
	btnSelectTool->setMaximumSize(QSize(40, 40));
	btnSelectTool->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnSelectTool.png"));
	btnSelectTool->setIconSize(QSize(36, 36));
	// because we can detect click or draw rect. we do not need menu by now.
	//QAction* pointselect_action = new QAction(tr("point select"), this);
	//QAction* rectselect_action = new QAction(tr("rect select"), this);
	//QAction* drawselect_action = new QAction(tr("draw select"), this);
	//pointselect_action->setCheckable(true);
	//rectselect_action->setCheckable(true);
	//drawselect_action->setCheckable(true);
	//QActionGroup* selectGroup = new QActionGroup(this);
	//selectGroup->addAction(pointselect_action);
	//selectGroup->addAction(rectselect_action);
	//selectGroup->addAction(drawselect_action);
	//connect(pointselect_action, SIGNAL(triggered()), this, SLOT(on_pointselect_action()));
	//connect(rectselect_action, SIGNAL(triggered()), this, SLOT(on_rectselect_action()));
	//connect(drawselect_action, SIGNAL(triggered()), this, SLOT(on_drawselect_action()));
	//QMenu* menu = new QMenu(btnSelectTool);
	//menu->addAction(pointselect_action);
	//menu->addAction(rectselect_action);
	//menu->addAction(drawselect_action);
	//btnSelectTool->setMenu(menu);
	hLayout->addWidget(btnSelectTool);
	btnSelectTool->setCheckable(true);
	gizmoGroup->addButton(btnSelectTool);

	btnExplodedViewTool = new QPushButton(widget);
	setStyledToolTip(btnExplodedViewTool, "Exploded View Tool");
	btnExplodedViewTool->setObjectName(QStringLiteral("btnExplodedViewTool"));
	sizePolicy1.setHeightForWidth(btnExplodedViewTool->sizePolicy().hasHeightForWidth());
	btnExplodedViewTool->setSizePolicy(sizePolicy1);
	btnExplodedViewTool->setMinimumSize(QSize(40, 40));
	btnExplodedViewTool->setMaximumSize(QSize(40, 40));
	btnExplodedViewTool->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnExplodedViewTool.png"));
	btnExplodedViewTool->setIconSize(QSize(36, 36));
	btnExplodedViewTool->setCheckable(true);
	hLayout->addWidget(btnExplodedViewTool);
	gizmoGroup->addButton(btnExplodedViewTool);

	btnTranslate = new QPushButton(widget);
	setStyledToolTip(btnTranslate, "Translate Tool");
	btnTranslate->setObjectName(QStringLiteral("btnTranslate"));
	sizePolicy1.setHeightForWidth(btnTranslate->sizePolicy().hasHeightForWidth());
	btnTranslate->setSizePolicy(sizePolicy1);
	btnTranslate->setMinimumSize(QSize(40, 40));
	btnTranslate->setMaximumSize(QSize(40, 40));
	btnTranslate->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnTranslate.png"));
	btnTranslate->setIconSize(QSize(36, 36));
	hLayout->addWidget(btnTranslate);
	btnTranslate->setCheckable(true);
	gizmoGroup->addButton(btnTranslate);

	btnRotate = new QPushButton(widget);
	setStyledToolTip(btnRotate, "Rotate Tool");
	btnRotate->setObjectName(QStringLiteral("btnRotate"));
	sizePolicy1.setHeightForWidth(btnRotate->sizePolicy().hasHeightForWidth());
	btnRotate->setSizePolicy(sizePolicy1);
	btnRotate->setMinimumSize(QSize(40, 40));
	btnRotate->setMaximumSize(QSize(40, 40));
	btnRotate->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnRotate.png"));
	btnRotate->setIconSize(QSize(36, 36));
	hLayout->addWidget(btnRotate);
	btnRotate->setCheckable(true);
	gizmoGroup->addButton(btnRotate);

	btnScale = new QPushButton(widget);
	setStyledToolTip(btnScale, "Scale Tool");
	btnScale->setObjectName(QStringLiteral("btnScale"));
	sizePolicy1.setHeightForWidth(btnScale->sizePolicy().hasHeightForWidth());
	btnScale->setSizePolicy(sizePolicy1);
	btnScale->setMinimumSize(QSize(40, 40));
	btnScale->setMaximumSize(QSize(40, 40));
	btnScale->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnScale.png"));
	btnScale->setIconSize(QSize(36, 36));
	hLayout->addWidget(btnScale);
	btnScale->setCheckable(true);
	gizmoGroup->addButton(btnScale);

	btnGizmoWithLocal = new QPushButton(widget);
	setStyledToolTip(btnGizmoWithLocal, "Gizmo in Global Mode");
	btnGizmoWithLocal->setObjectName(QStringLiteral("btnGizmoWithLocal"));
	sizePolicy1.setHeightForWidth(btnGizmoWithLocal->sizePolicy().hasHeightForWidth());
	btnGizmoWithLocal->setSizePolicy(sizePolicy1);
	btnGizmoWithLocal->setMinimumSize(QSize(40, 40));
	btnGizmoWithLocal->setMaximumSize(QSize(40, 40));
	btnGizmoWithLocal->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnGizmoWithGlobal.png"));
	btnGizmoWithLocal->setIconSize(QSize(36, 36));
	hLayout->addWidget(btnGizmoWithLocal);
	btnGizmoWithLocal->setCheckable(true);

	//fSeparate = new QFrame(widget);
	//fSeparate->setObjectName(QStringLiteral("fSeparate"));
	//fSeparate->setFrameShape(QFrame::VLine);
	//fSeparate->setFrameShadow(QFrame::Sunken);
	//hLayout->addWidget(fSeparate);

	btnPaintbrush = new QPushButton(widget);
	setStyledToolTip(btnPaintbrush, "Not Implement");
	btnPaintbrush->setObjectName(QStringLiteral("btnPaintbrush"));
	sizePolicy1.setHeightForWidth(btnPaintbrush->sizePolicy().hasHeightForWidth());
	btnPaintbrush->setSizePolicy(sizePolicy1);
	btnPaintbrush->setMinimumSize(QSize(40, 40));
	btnPaintbrush->setMaximumSize(QSize(40, 40));
	btnPaintbrush->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnPaintbrush.png"));
	btnPaintbrush->setIconSize(QSize(36, 36));
	hLayout->addWidget(btnPaintbrush);

	btnFractureTool = new QPushButton(widget);
	setStyledToolTip(btnFractureTool, "Not Implement");
	btnFractureTool->setObjectName(QStringLiteral("btnFractureTool"));
	sizePolicy1.setHeightForWidth(btnFractureTool->sizePolicy().hasHeightForWidth());
	btnFractureTool->setSizePolicy(sizePolicy1);
	btnFractureTool->setMinimumSize(QSize(40, 40));
	btnFractureTool->setMaximumSize(QSize(40, 40));
	btnFractureTool->setText(QApplication::translate("AppMainToolbar", "Fracture", 0));
	btnFractureTool->setFont(fontCopy);
	hLayout->addWidget(btnFractureTool);

	btnJointsTool = new QPushButton(widget);
	setStyledToolTip(btnJointsTool, "Not Implement");
	btnJointsTool->setObjectName(QStringLiteral("btnJointsTool"));
	sizePolicy1.setHeightForWidth(btnJointsTool->sizePolicy().hasHeightForWidth());
	btnJointsTool->setSizePolicy(sizePolicy1);
	btnJointsTool->setMinimumSize(QSize(40, 40));
	btnJointsTool->setMaximumSize(QSize(40, 40));
	btnJointsTool->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnJointsTool.png"));
	btnJointsTool->setIconSize(QSize(36, 36));
	hLayout->addWidget(btnJointsTool);

	btnFuseSelectedChunks = new QPushButton(widget);
	setStyledToolTip(btnFuseSelectedChunks, "Not Implement");
	btnFuseSelectedChunks->setObjectName(QStringLiteral("btnFuseSelectedChunks"));
	sizePolicy1.setHeightForWidth(btnFuseSelectedChunks->sizePolicy().hasHeightForWidth());
	btnFuseSelectedChunks->setSizePolicy(sizePolicy1);
	btnFuseSelectedChunks->setMinimumSize(QSize(40, 40));
	btnFuseSelectedChunks->setMaximumSize(QSize(40, 40));
	btnFuseSelectedChunks->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnFuseSelectedChunks.png"));
	btnFuseSelectedChunks->setIconSize(QSize(36, 36));
	hLayout->addWidget(btnFuseSelectedChunks);

	fSeparate = new QFrame(widget);
	fSeparate->setObjectName(QStringLiteral("fSeparate"));
	fSeparate->setFrameShape(QFrame::VLine);
	fSeparate->setFrameShadow(QFrame::Sunken);
	hLayout->addWidget(fSeparate);

	QButtonGroup* editSimModeGroup = new QButtonGroup(this);

	btnReset = new QPushButton(widget);
	setStyledToolTip(btnReset, "Reset Chunks and Switch to Edition Mode");
	btnReset->setObjectName(QStringLiteral("btnReset"));
	sizePolicy1.setHeightForWidth(btnReset->sizePolicy().hasHeightForWidth());
	btnReset->setSizePolicy(sizePolicy1);
	btnReset->setMinimumSize(QSize(40, 40));
	btnReset->setMaximumSize(QSize(40, 40));
	btnReset->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnReset.png"));
	btnReset->setIconSize(QSize(36, 36));
	hLayout->addWidget(btnReset);
	btnReset->setCheckable(true);
	editSimModeGroup->addButton(btnReset);

	btnSimulatePlay = new QPushButton(widget);
	setStyledToolTip(btnSimulatePlay, "Switch to Simulate Mode");
	btnSimulatePlay->setObjectName(QStringLiteral("btnSimulatePlay"));
	sizePolicy1.setHeightForWidth(btnSimulatePlay->sizePolicy().hasHeightForWidth());
	btnSimulatePlay->setSizePolicy(sizePolicy1);
	btnSimulatePlay->setMinimumSize(QSize(40, 40));
	btnSimulatePlay->setMaximumSize(QSize(40, 40));
	btnSimulatePlay->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnSimulatePlay.png"));
	btnSimulatePlay->setIconSize(QSize(36, 36));
	hLayout->addWidget(btnSimulatePlay);
	btnSimulatePlay->setCheckable(true);
	editSimModeGroup->addButton(btnSimulatePlay);

	btnFrameStepForward = new QPushButton(widget);
	setStyledToolTip(btnFrameStepForward, "Switch to StepForward Mode");
	btnFrameStepForward->setObjectName(QStringLiteral("btnFrameStepForward"));
	sizePolicy1.setHeightForWidth(btnFrameStepForward->sizePolicy().hasHeightForWidth());
	btnFrameStepForward->setSizePolicy(sizePolicy1);
	btnFrameStepForward->setMinimumSize(QSize(40, 40));
	btnFrameStepForward->setMaximumSize(QSize(40, 40));
	btnFrameStepForward->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnFrameStepForward.png"));
	btnFrameStepForward->setIconSize(QSize(36, 36));
	hLayout->addWidget(btnFrameStepForward);
	btnFrameStepForward->setCheckable(true);
	editSimModeGroup->addButton(btnFrameStepForward);

	fSeparate = new QFrame(widget);
	fSeparate->setObjectName(QStringLiteral("fSeparate"));
	fSeparate->setFrameShape(QFrame::VLine);
	fSeparate->setFrameShadow(QFrame::Sunken);
	hLayout->addWidget(fSeparate);

	// use btnDamage for Damage function.
	btnDamage = new QPushButton(widget);
	setStyledToolTip(btnDamage, "Switch on/off Damage Mode");
	btnDamage->setObjectName(QStringLiteral("btnDamage"));
	sizePolicy1.setHeightForWidth(btnDamage->sizePolicy().hasHeightForWidth());
	btnDamage->setSizePolicy(sizePolicy1);
	btnDamage->setMinimumSize(QSize(40, 40));
	btnDamage->setMaximumSize(QSize(40, 40));
	btnDamage->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnDamage.png"));
	btnDamage->setIconSize(QSize(36, 36));
	btnDamage->setCheckable(true);
	hLayout->addWidget(btnDamage);

	btnProjectile = new QPushButton(widget);
	setStyledToolTip(btnProjectile, "Throw a Box to Chunks");
	btnProjectile->setObjectName(QStringLiteral("btnProjectile"));
	sizePolicy1.setHeightForWidth(btnProjectile->sizePolicy().hasHeightForWidth());
	btnProjectile->setSizePolicy(sizePolicy1);
	btnProjectile->setMinimumSize(QSize(40, 40));
	btnProjectile->setMaximumSize(QSize(40, 40));
	btnProjectile->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnProjectile.png"));
	btnProjectile->setIconSize(QSize(36, 36));
	hLayout->addWidget(btnProjectile);

	btnDropObject = new QPushButton(widget);
	setStyledToolTip(btnDropObject, "Drop the object and simulate");
	btnDropObject->setObjectName(QStringLiteral("btnDropObject"));
	sizePolicy1.setHeightForWidth(btnDropObject->sizePolicy().hasHeightForWidth());
	btnDropObject->setSizePolicy(sizePolicy1);
	btnDropObject->setMinimumSize(QSize(40, 40));
	btnDropObject->setMaximumSize(QSize(40, 40));
	btnDropObject->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnDropObject.png"));
	btnDropObject->setIconSize(QSize(36, 36));
	hLayout->addWidget(btnDropObject);

	fSeparate = new QFrame(widget);
	fSeparate->setObjectName(QStringLiteral("fSeparate"));
	fSeparate->setFrameShape(QFrame::VLine);
	fSeparate->setFrameShadow(QFrame::Sunken);
	hLayout->addWidget(fSeparate);

	btnPreferences = new QPushButton(widget);
	setStyledToolTip(btnPreferences, "Save Blast Asset");
	btnPreferences->setObjectName(QStringLiteral("btnPreferences"));
	sizePolicy1.setHeightForWidth(btnPreferences->sizePolicy().hasHeightForWidth());
	btnPreferences->setSizePolicy(sizePolicy1);
	btnPreferences->setMinimumSize(QSize(40, 40));
	btnPreferences->setMaximumSize(QSize(40, 40));
	btnPreferences->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnPreferences.png"));
	btnPreferences->setIconSize(QSize(36, 36));
	hLayout->addWidget(btnPreferences);

	QSpacerItem *horizontalSpacer;
	horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
	hLayout->addItem(horizontalSpacer);

	this->setWidget(widget);

	connect(&gDropTimer, SIGNAL(timeout()), this, SLOT(on_btnDropObject_clicked()));
	connect(btnOpenProject, SIGNAL(clicked()), this, SLOT(on_btnOpenProject_clicked()));
	connect(btnSaveProject, SIGNAL(clicked()), this, SLOT(on_btnSaveProject_clicked()));
	connect(btnExportAssets, SIGNAL(clicked()), this, SLOT(on_btnExportAssets_clicked()));
	connect(btnExportFilepath, SIGNAL(clicked()), this, SLOT(on_btnExportFilepath_clicked()));
	connect(ssbiDepthPreview, SIGNAL(valueChanged(int)), this, SLOT(on_ssbiDepthPreview_valueChanged(int)));
	connect(cbExactCoverage, SIGNAL(stateChanged(int)), this, SLOT(on_cbExactCoverage_stateChanged(int)));
	connect(btnSelectTool, SIGNAL(clicked()), this, SLOT(on_btnSelectTool_clicked()));
	connect(btnTranslate, SIGNAL(clicked()), this, SLOT(on_Translate_clicked()));
	connect(btnRotate, SIGNAL(clicked()), this, SLOT(on_Rotation_clicked()));
	connect(btnScale, SIGNAL(clicked()), this, SLOT(on_Scale_clicked()));
	connect(btnGizmoWithLocal, SIGNAL(clicked()), this, SLOT(on_btnGizmoWithLocal_clicked()));
	connect(btnPaintbrush, SIGNAL(clicked()), this, SLOT(on_btnPaintbrush_clicked()));
	connect(btnFractureTool, SIGNAL(clicked()), this, SLOT(on_btnFractureTool_clicked()));
	connect(btnExplodedViewTool, SIGNAL(clicked()), this, SLOT(on_btnExplodedViewTool_clicked()));
	connect(btnJointsTool, SIGNAL(clicked()), this, SLOT(on_btnJointsTool_clicked()));
	connect(btnFuseSelectedChunks, SIGNAL(clicked()), this, SLOT(on_btnFuseSelectedChunks_clicked()));
	connect(btnReset, SIGNAL(clicked()), this, SLOT(on_btnReset_clicked()));
	connect(btnSimulatePlay, SIGNAL(clicked()), this, SLOT(on_btnSimulatePlay_clicked()));
	connect(btnFrameStepForward, SIGNAL(clicked()), this, SLOT(on_btnFrameStepForward_clicked()));
	connect(btnDamage, SIGNAL(clicked()), this, SLOT(on_btnDamage_clicked()));
	connect(btnProjectile, SIGNAL(clicked()), this, SLOT(on_btnProjectile_clicked()));
	connect(btnDropObject, SIGNAL(clicked()), this, SLOT(on_btnDropObject_clicked()));
	connect(btnPreferences, SIGNAL(clicked()), this, SLOT(on_btnPreferences_clicked()));

	QPushButton* unImplementButtons[] =
	{
		btnPaintbrush, btnFractureTool, btnJointsTool, btnFuseSelectedChunks, btnPreferences
	};
	int buttonSize = sizeof(unImplementButtons) / sizeof(QPushButton*);
	for (int bs = 0; bs < buttonSize; bs++)
	{
		unImplementButtons[bs]->setVisible(false);
	}
}

void BlastToolbar::updateValues()
{
	leExportFilepath->setText(GlobalSettings::Inst().m_projectFileDir.c_str());
	btnGizmoWithLocal->setChecked(AppMainWindow::Inst().m_bGizmoWithLocal);

	SampleManager* pSampleManager = SampleManager::ins();
	DamageToolController& damageToolController = pSampleManager->getDamageToolController();
	bool bChecked = damageToolController.IsEnabled() && damageToolController.IsEnabled() && damageToolController.isDamageMode();
	btnDamage->setChecked(bChecked);
}

void BlastToolbar::on_btnOpenProject_clicked()
{
	//BlastPlugin::OpenBpxa("", "");
	BlastPlugin::Inst().menu_openProject();
	// show project path in toolbar
	updateValues();
}

void BlastToolbar::on_btnSaveProject_clicked()
{
	qDebug("%s", __FUNCTION__);
	on_btnExportAssets_clicked();
	BlastPlugin::Inst().menu_saveProject();
}

void BlastToolbar::on_btnExportAssets_clicked()
{
	qDebug("%s", __FUNCTION__);
	FileReferencesPanel* pPanel = BlastPlugin::Inst().GetFileReferencesPanel();
	if (pPanel)
	{
		BlastAssetInstancesNode* assetInstancesNode = BlastTreeData::ins().getBlastAssetInstancesNode();
		if (assetInstancesNode != nullptr)
		{
			size_t count = assetInstancesNode->children.size();
			for (size_t i = 0; i < count; ++i)
			{
				BlastAssetInstanceNode* assetInstanceNode = dynamic_cast<BlastAssetInstanceNode*>(assetInstancesNode->children[i]);
				if (assetInstanceNode)
				{
					assetInstanceNode->setSelected(true);
					pPanel->on_btnSave_clicked();
				}
			}
		}
	}
}

void BlastToolbar::on_btnExportFilepath_clicked()
{
	qDebug("%s", __FUNCTION__);
	AppMainWindow& window = AppMainWindow::Inst();
	QString lastDir = leExportFilepath->text();
	if (lastDir.length() < 1)
	{
		lastDir = GlobalSettings::Inst().m_projectFileDir.c_str();
	}
	QString pathName = QFileDialog::getExistingDirectory(&window, "Exporting Path", lastDir);
	if (!pathName.isEmpty())
	{
		GlobalSettings::Inst().m_projectFileDir = pathName.toUtf8().data();
		leExportFilepath->setText(pathName);
	}
}

void BlastToolbar::on_ssbiDepthPreview_valueChanged(int v)
{
	qDebug("%s", __FUNCTION__);
	BlastSceneTree* pBlastSceneTree = BlastSceneTree::ins();
	pBlastSceneTree->hideAllChunks();
	if (m_fullCoverage)
	{
		pBlastSceneTree->setChunkVisibleFullCoverage(v);
	}
	else
	{
		std::vector<uint32_t> depths(1, v);
		pBlastSceneTree->setChunkVisible(depths, true);
	}
	// refresh display in scene tree
	//pBlastSceneTree->updateValues(false);
	SampleManager::ins()->m_bNeedRefreshTree = true;
}

void BlastToolbar::on_cbExactCoverage_stateChanged(int state)
{
	qDebug("%s", __FUNCTION__);
	m_fullCoverage = (state != 0);
	//lbExactCoverage->setText(QApplication::translate("AppMainToolbar", (m_fullCoverage? "Full Coverage" : "Exact Coverage"), 0));
	int depth = ssbiDepthPreview->value();
	on_ssbiDepthPreview_valueChanged(depth);
}

void BlastToolbar::on_btnSelectTool_clicked()
{
	SampleManager* pSampleManager = SampleManager::ins();
	SelectionToolController& selectionToolController = pSampleManager->getSelectionToolController();
	ExplodeToolController& expolodeController = pSampleManager->getExplodeToolController();
	expolodeController.DisableController();
	GizmoToolController& gizmoToolController = pSampleManager->getGizmoToolController();
	gizmoToolController.showAxisRenderables(false);

	if (gizmoToolController.IsEnabled())
	{
		gizmoToolController.DisableController();
		selectionToolController.setTargetActor(gizmoToolController.getTargetActor());
	}
	selectionToolController.EnableController();

}

void BlastToolbar::on_pointselect_action()
{
	qDebug("%s", __FUNCTION__);
}

void BlastToolbar::on_rectselect_action()
{
	qDebug("%s", __FUNCTION__);
}

void BlastToolbar::on_drawselect_action()
{
	qDebug("%s", __FUNCTION__);
}

bool BlastToolbar::on_Translate_clicked()
{
	SampleManager* pSampleManager = SampleManager::ins();
	SelectionToolController& selectionToolController = pSampleManager->getSelectionToolController();
	ExplodeToolController& expolodeController = pSampleManager->getExplodeToolController();
	expolodeController.DisableController();
	GizmoToolController& gizmoToolController = pSampleManager->getGizmoToolController();
	gizmoToolController.showAxisRenderables(false);

	if (selectionToolController.IsEnabled())
	{
		selectionToolController.DisableController();
		gizmoToolController.setTargetActor(selectionToolController.getTargetActor());
	}
	gizmoToolController.EnableController();
	gizmoToolController.setGizmoToolMode(GTM_Translate);

	return true;
}

bool BlastToolbar::on_Rotation_clicked()
{
	SampleManager* pSampleManager = SampleManager::ins();
	SelectionToolController& selectionToolController = pSampleManager->getSelectionToolController();
	ExplodeToolController& expolodeController = pSampleManager->getExplodeToolController();
	expolodeController.DisableController();
	GizmoToolController& gizmoToolController = pSampleManager->getGizmoToolController();
	gizmoToolController.showAxisRenderables(false);

	if (selectionToolController.IsEnabled())
	{
		selectionToolController.DisableController();
		gizmoToolController.setTargetActor(selectionToolController.getTargetActor());
	}
	gizmoToolController.EnableController();
	gizmoToolController.setGizmoToolMode(GTM_Rotation);

	return true;
}

bool BlastToolbar::on_Scale_clicked()
{
	SampleManager* pSampleManager = SampleManager::ins();
	SelectionToolController& selectionToolController = pSampleManager->getSelectionToolController();
	ExplodeToolController& expolodeController = pSampleManager->getExplodeToolController();
	expolodeController.DisableController();
	GizmoToolController& gizmoToolController = pSampleManager->getGizmoToolController();
	gizmoToolController.showAxisRenderables(false);

	if (selectionToolController.IsEnabled())
	{
		selectionToolController.DisableController();
		gizmoToolController.setTargetActor(selectionToolController.getTargetActor());
	}
	gizmoToolController.EnableController();
	gizmoToolController.setGizmoToolMode(GTM_Scale);

	return true;
}

void BlastToolbar::on_btnGizmoWithLocal_clicked()
{
	AppMainWindow& app = AppMainWindow::Inst();
	app.m_bGizmoWithLocal = !app.m_bGizmoWithLocal;
	if (app.m_bGizmoWithLocal)
	{
		btnGizmoWithLocal->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnGizmoWithLocal.png"));
		setStyledToolTip(btnGizmoWithLocal, "Gizmo in Local Mode");
	}
	else
	{
		btnGizmoWithLocal->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnGizmoWithGlobal.png"));
		setStyledToolTip(btnGizmoWithLocal, "Gizmo in Global Mode");
	}
	app.updateUI();
}

void BlastToolbar::on_btnPaintbrush_clicked()
{
	qDebug("%s", __FUNCTION__);
}

void BlastToolbar::on_btnFractureTool_clicked()
{
	qDebug("%s", __FUNCTION__);
}

void BlastToolbar::on_btnExplodedViewTool_clicked()
{
#if (1)
	SampleManager* pSampleManager = SampleManager::ins();
	ExplodeToolController& explodeToolController = pSampleManager->getExplodeToolController();
	GizmoToolController& gizmoToolController = pSampleManager->getGizmoToolController();
	gizmoToolController.showAxisRenderables(false);

	pSampleManager->getSelectionToolController().DisableController();

	if (gizmoToolController.IsEnabled())
	{
		gizmoToolController.DisableController();
	}
	explodeToolController.EnableController();
#endif

#if (0)
	SampleManager* pSampleManager = SampleManager::ins();
	if (pSampleManager->IsSimulating())
	{
		return;
	}

	BlastAsset* pBlastAsset = nullptr;
	int nFamilyIndex = -1;
	pSampleManager->getCurrentSelectedInstance(&pBlastAsset, nFamilyIndex);
	if (pBlastAsset == nullptr)
	{
		std::map<BlastAsset*, AssetList::ModelAsset>& AssetDescMap = pSampleManager->getAssetDescMap();
		if (AssetDescMap.size() == 1)
		{
			std::map<BlastAsset*, AssetList::ModelAsset>::iterator itAssetDescMap = AssetDescMap.begin();
			pSampleManager->setCurrentSelectedInstance(itAssetDescMap->first, -1);
			pBlastAsset = pSampleManager->getCurBlastAsset();
			viewer_msg("no asset selected, use the only one in current scene.");
		}
	}
	if (pBlastAsset == nullptr)
	{
		viewer_msg("please select one asset before explode.");
		return;
	}

	std::map<BlastAsset*, std::vector<BlastFamily*>>& AssetFamiliesMap = pSampleManager->getAssetFamiliesMap();
	std::map<BlastAsset*, std::vector<BlastFamily*>>::iterator itAFM = AssetFamiliesMap.find(pBlastAsset);
	if (itAFM == AssetFamiliesMap.end())
	{
		return;
	}

	std::vector<BlastFamily*> families = itAFM->second;
	int familySize = families.size();
	if (familySize == 0)
	{
		viewer_msg("no instance for current asset.");
		return;
	}

	if (nFamilyIndex == -1 || nFamilyIndex >= familySize)
	{
		nFamilyIndex = 0;
		viewer_msg("no instance selected, use the first one of current asset.");
	}

	BlastFamily* pFamily = families[nFamilyIndex];	

	PxScene& scene = pSampleManager->getPhysXController().getEditPhysXScene();
	const PxU32 actorsCountTotal = scene.getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
	if (actorsCountTotal == 0)
	{
		return;
	}

	std::vector<PxActor*> actorsTotal(actorsCountTotal);
	PxU32 nbActors = scene.getActors(PxActorTypeFlag::eRIGID_DYNAMIC, &actorsTotal[0], actorsCountTotal, 0);
	PX_ASSERT(actorsCountTotal == nbActors);

	std::vector<PxActor*> actors;
	PxActor* pRootActor = nullptr;
	for (int act = 0; act < actorsCountTotal; act++)
	{
		if (pFamily->find(*actorsTotal[act]))
		{
			if (pRootActor == nullptr)
			{
				uint32_t chunkIndex = pFamily->getChunkIndexByPxActor(*actorsTotal[act]);
				std::vector<uint32_t> chunkIndexes;
				chunkIndexes.push_back(chunkIndex);
				std::vector<BlastChunkNode*> chunkNodes = BlastTreeData::ins().getChunkNodeByBlastChunk(pBlastAsset, chunkIndexes);
				if (chunkNodes.size() > 0 && BlastTreeData::isRoot(chunkNodes[0]))
				{
					pRootActor = actorsTotal[act];
				}
				else
				{
					actors.push_back(actorsTotal[act]);
				}
			}
			else
			{
				actors.push_back(actorsTotal[act]);
			}			
		}
	}

	if (pRootActor == nullptr)
	{
		return;
	}

	++nExplodedViewState;

	BlastController& blastController = pSampleManager->getBlastController();	

	PxVec3 origin = pRootActor->getWorldBounds().getCenter();

	int actorsCount = actors.size();
	for (int ac = 0; ac < actorsCount; ac++)
	{
		PxActor* actor = actors[ac];
		PxRigidDynamic* dynamic = actor->is<PxRigidDynamic>();
		PX_ASSERT(dynamic != nullptr);
		PxTransform transformOld = dynamic->getGlobalPose();
		PxTransform transformNew = transformOld;

		PxBounds3 bound = actor->getWorldBounds();
		PxVec3 target = bound.getCenter();
		PxVec3 tChange = (target - origin) * 0.5;
		if (nExplodedViewState > 5)
		{
			tChange = (origin - target) / 3;
		}
		PxVec3 newTarget = target + tChange;
		transformNew.p = transformOld.p + tChange;
		dynamic->setGlobalPose(transformNew);
		blastController.updateActorRenderableTransform(*actor, transformNew, false);
	}
	if (nExplodedViewState > 9)
		nExplodedViewState = 0;

#endif
}

void BlastToolbar::on_btnJointsTool_clicked()
{
	qDebug("%s", __FUNCTION__);
}

void BlastToolbar::on_btnFuseSelectedChunks_clicked()
{
	qDebug("%s", __FUNCTION__);
}

void BlastToolbar::on_btnReset_clicked()
{
	nExplodedViewState = 0;

	SampleManager* pSampleManager = SampleManager::ins();
	bool bStopSimulation = pSampleManager->IsSimulating();
	pSampleManager->resetScene();
	// only show depth 0 if stop from simulation. Otherwise, just show as previous.
	if (bStopSimulation)
	{
		BlastSceneTree* pBlastSceneTree = BlastSceneTree::ins();
		pBlastSceneTree->hideAllChunks();
		std::vector<uint32_t> depths(1, 0);
		pBlastSceneTree->setChunkVisible(depths, true);
		// refresh display in scene tree
		//pBlastSceneTree->updateValues(false);
		SampleManager::ins()->m_bNeedRefreshTree = true;
	}
	updateValues();
}

void BlastToolbar::on_btnSimulatePlay_clicked()
{
	qDebug("%s", __FUNCTION__);

	SampleManager* pSampleManager = SampleManager::ins();
	if (!pSampleManager->IsSimulating())
	{
		// force to recreate BlastFamilyModelSimple from project data
		pSampleManager->resetScene();
		pSampleManager->EnableSimulating(true);
		// set right damage mode
		DamageToolController& damageToolController = pSampleManager->getDamageToolController();
		bool bChecked = damageToolController.IsEnabled() && damageToolController.IsEnabled() && damageToolController.isDamageMode();
		if (damageToolController.isDamageMode() && !bChecked)
		{
			on_btnDamage_clicked();
		}
	}
	else
	{
		// pause it or continue
		PhysXController& physx = pSampleManager->getPhysXController();
		bool bState = physx.isPaused();
		physx.setPaused(!bState);
	}
}

void BlastToolbar::on_btnFrameStepForward_clicked()
{
	qDebug("%s", __FUNCTION__);

	SampleManager* pSampleManager = SampleManager::ins();
	if (!pSampleManager->IsSimulating())
	{
		// force to recreate BlastFamilyModelSimple from project data
		pSampleManager->resetScene();
		// set right damage mode
		DamageToolController& damageToolController = pSampleManager->getDamageToolController();
		bool bChecked = damageToolController.IsEnabled() && damageToolController.IsEnabled() && damageToolController.isDamageMode();
		if (damageToolController.isDamageMode() && !bChecked)
		{
			on_btnDamage_clicked();
		}
	}
	pSampleManager->EnableSimulating(true);
	pSampleManager->EnableStepforward(true);
}

void BlastToolbar::on_btnDamage_clicked()
{
	qDebug("%s", __FUNCTION__);
	SampleManager* pSampleManager = SampleManager::ins();
	DamageToolController& damageToolController = pSampleManager->getDamageToolController();
	if (damageToolController.IsEnabled())
	{
		damageToolController.DisableController();
		damageToolController.setDamageMode(false);
		btnDamage->setChecked(false);
	}
	else
	{
		damageToolController.EnableController();
		damageToolController.setDamageMode(true);
		btnDamage->setChecked(true);
	}
//	pSampleManager->getDamageToolController().setDamageMode(!pSampleManager->getDamageToolController().isDamageMode());
}

void BlastToolbar::on_btnProjectile_clicked()
{
	qDebug("%s", __FUNCTION__);

	SampleManager* pSampleManager = SampleManager::ins();	
	if (pSampleManager->IsSimulating())
	{
		SceneController& sceneController = pSampleManager->getSceneController();
		sceneController.addProjectile();
	}
	else
	{
		viewer_msg("Please use it when simulation runs.");
	}
}

void BlastToolbar::on_btnDropObject_clicked()
{
	qDebug("%s", __FUNCTION__);
	SampleManager* pSampleManager = SampleManager::ins();
	if (pSampleManager->IsSimulating())
	{
		on_btnReset_clicked();
		// if not use timer, it could go to strange paused state.
		gDropTimer.start(10);
		return;
	}
	gDropTimer.stop();
	BlastAssetInstancesNode* assetInstancesNode = BlastTreeData::ins().getBlastAssetInstancesNode();
	if (assetInstancesNode != nullptr)
	{
		physx::PxVec3 size = pSampleManager->getAssetExtent();
		float fChange = size.magnitude();
		size_t count = assetInstancesNode->children.size();
		// find the min height first
		float minHeight = NV_MAX_F32;
		for (size_t i = 0; i < count; ++i)
		{
			BlastAssetInstanceNode* assetInstanceNode = (BlastAssetInstanceNode*)assetInstancesNode->children[i];
			BPPAssetInstance* bppInstance = (BPPAssetInstance*)assetInstanceNode->getData();
			if (bppInstance)
			{
				BlastFamily* family = SampleManager::ins()->getFamilyByInstance(bppInstance);
				if (family)
				{
					physx::PxTransform t = family->getSettings().transform;
					if(t.p.y < minHeight)
						minHeight = t.p.y;
				}
			}
		}
		// make fChange
		while ((minHeight + fChange) < 0.0f)
		{
			fChange += fChange;
		}
		// change position
		for (size_t i = 0; i < count; ++i)
		{
			BlastAssetInstanceNode* assetInstanceNode = (BlastAssetInstanceNode*)assetInstancesNode->children[i];
			BPPAssetInstance* bppInstance = (BPPAssetInstance*)assetInstanceNode->getData();
			if (bppInstance)
			{
				BlastFamily* family = SampleManager::ins()->getFamilyByInstance(bppInstance);
				if (family)
				{
					physx::PxTransform t = family->getSettings().transform;
					t.p.y += fChange;
					family->initTransform(t);
				}
			}
		}
		// have to reset scene to make the new positions used.
		on_btnReset_clicked();
		pSampleManager->EnableSimulating(true);
		// restore original position
		for (size_t i = 0; i < count; ++i)
		{
			BlastAssetInstanceNode* assetInstanceNode = (BlastAssetInstanceNode*)assetInstancesNode->children[i];
			BPPAssetInstance* bppInstance = (BPPAssetInstance*)assetInstanceNode->getData();
			if (bppInstance)
			{
				BlastFamily* family = SampleManager::ins()->getFamilyByInstance(bppInstance);
				if (family)
				{
					physx::PxTransform t = family->getSettings().transform;
					t.p.y -= fChange;
					family->initTransform(t);
				}
			}
		}
		// set right damage mode
		DamageToolController& damageToolController = pSampleManager->getDamageToolController();
		bool bChecked = damageToolController.IsEnabled() && damageToolController.IsEnabled() && damageToolController.isDamageMode();
		if (damageToolController.isDamageMode() && !bChecked)
		{
			on_btnDamage_clicked();
		}
	}
}

void BlastToolbar::on_btnPreferences_clicked()
{
	qDebug("%s", __FUNCTION__);

	SampleManager* pSampleManager = SampleManager::ins();
	BlastAsset* pBlastAsset = pSampleManager->getCurBlastAsset();
	pSampleManager->saveAsset(pBlastAsset);
}

void BlastToolbar::updateCheckIconsStates()
{
	SampleManager* pSampleManager = SampleManager::ins();
	SelectionToolController& selectionToolController = pSampleManager->getSelectionToolController();
	btnSelectTool->setChecked(selectionToolController.IsEnabled());

	GizmoToolController& gizmoToolController = pSampleManager->getGizmoToolController();
	bool bGizmo = gizmoToolController.IsEnabled();
	GizmoToolMode mode = gizmoToolController.getGizmoToolMode();
	btnTranslate->setChecked(mode == GTM_Translate && bGizmo);
	btnRotate->setChecked(mode == GTM_Rotation && bGizmo);
	btnScale->setChecked(mode == GTM_Scale && bGizmo);
}