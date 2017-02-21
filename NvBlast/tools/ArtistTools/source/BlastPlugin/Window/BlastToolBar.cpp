#include "BlastToolbar.h"

#include <QtWidgets/QFileDialog>
#include "AppMainWindow.h"
#include "PhysXController.h"
#include "QtUtil.h"

BlastToolbar::BlastToolbar(QWidget* parent)
	: QDockWidget(parent)
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
	setStyledToolTip(btnOpenProject, "Open Blast Asset");
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
	setStyledToolTip(btnSaveProject, "Not Implement");
	btnSaveProject->setObjectName(QStringLiteral("btnSaveProject"));
	sizePolicy1.setHeightForWidth(btnOpenProject->sizePolicy().hasHeightForWidth());
	btnSaveProject->setSizePolicy(sizePolicy1);
	btnSaveProject->setMinimumSize(QSize(40, 40));
	btnSaveProject->setMaximumSize(QSize(40, 40));
	btnSaveProject->setText(QApplication::translate("AppMainToolbar", "Save\nAll", 0));
	hLayout->addWidget(btnSaveProject);

	btnExportAssets = new QPushButton(widget);
	setStyledToolTip(btnExportAssets, "Not Implement");
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

	lbDepthPreview = new QLabel(widget);
	lbDepthPreview->setObjectName(QStringLiteral("hlExactCoverage"));
	lbDepthPreview->setText(QApplication::translate("AppMainToolbar", "Exact Coverage", 0));
	hlExactCoverage->addWidget(lbDepthPreview);

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
	
	btnSelectTool = new QPushButton(widget);
	setStyledToolTip(btnSelectTool, "Switch to Selection Mode");
	btnSelectTool->setObjectName(QStringLiteral("btnSelectTool"));
	sizePolicy1.setHeightForWidth(btnSelectTool->sizePolicy().hasHeightForWidth());
	btnSelectTool->setSizePolicy(sizePolicy1);
	btnSelectTool->setMinimumSize(QSize(40, 40));
	btnSelectTool->setMaximumSize(QSize(40, 40));
	btnSelectTool->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnSelectTool.png"));
	btnSelectTool->setIconSize(QSize(40, 40));
	QAction* pointselect_action = new QAction(tr("point select"), this);
	QAction* rectselect_action = new QAction(tr("rect select"), this);
	QAction* drawselect_action = new QAction(tr("draw select"), this);
	connect(pointselect_action, SIGNAL(triggered()), this, SLOT(on_pointselect_action()));
	connect(rectselect_action, SIGNAL(triggered()), this, SLOT(on_rectselect_action()));
	connect(drawselect_action, SIGNAL(triggered()), this, SLOT(on_drawselect_action()));
	QMenu* menu = new QMenu(btnSelectTool);
	menu->addAction(pointselect_action);
	menu->addAction(rectselect_action);
	menu->addAction(drawselect_action);
	btnSelectTool->setMenu(menu);
	hLayout->addWidget(btnSelectTool);

	btnPaintbrush = new QPushButton(widget);
	setStyledToolTip(btnPaintbrush, "Not Implement");
	btnPaintbrush->setObjectName(QStringLiteral("btnPaintbrush"));
	sizePolicy1.setHeightForWidth(btnPaintbrush->sizePolicy().hasHeightForWidth());
	btnPaintbrush->setSizePolicy(sizePolicy1);
	btnPaintbrush->setMinimumSize(QSize(40, 40));
	btnPaintbrush->setMaximumSize(QSize(40, 40));
	btnPaintbrush->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnPaintbrush.png"));
	btnPaintbrush->setIconSize(QSize(40, 40));
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

	btnExplodedViewTool = new QPushButton(widget);
	setStyledToolTip(btnExplodedViewTool, "Not Implement");
	btnExplodedViewTool->setObjectName(QStringLiteral("btnExplodedViewTool"));
	sizePolicy1.setHeightForWidth(btnExplodedViewTool->sizePolicy().hasHeightForWidth());
	btnExplodedViewTool->setSizePolicy(sizePolicy1);
	btnExplodedViewTool->setMinimumSize(QSize(40, 40));
	btnExplodedViewTool->setMaximumSize(QSize(40, 40));
	btnExplodedViewTool->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnExplodedViewTool.png"));
	btnExplodedViewTool->setIconSize(QSize(40, 40));
	hLayout->addWidget(btnExplodedViewTool);

	btnJointsTool = new QPushButton(widget);
	setStyledToolTip(btnJointsTool, "Not Implement");
	btnJointsTool->setObjectName(QStringLiteral("btnJointsTool"));
	sizePolicy1.setHeightForWidth(btnJointsTool->sizePolicy().hasHeightForWidth());
	btnJointsTool->setSizePolicy(sizePolicy1);
	btnJointsTool->setMinimumSize(QSize(40, 40));
	btnJointsTool->setMaximumSize(QSize(40, 40));
	btnJointsTool->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnJointsTool.png"));
	btnJointsTool->setIconSize(QSize(40, 40));
	hLayout->addWidget(btnJointsTool);

	btnFuseSelectedChunks = new QPushButton(widget);
	setStyledToolTip(btnFuseSelectedChunks, "Not Implement");
	btnFuseSelectedChunks->setObjectName(QStringLiteral("btnFuseSelectedChunks"));
	sizePolicy1.setHeightForWidth(btnFuseSelectedChunks->sizePolicy().hasHeightForWidth());
	btnFuseSelectedChunks->setSizePolicy(sizePolicy1);
	btnFuseSelectedChunks->setMinimumSize(QSize(40, 40));
	btnFuseSelectedChunks->setMaximumSize(QSize(40, 40));
	btnFuseSelectedChunks->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnFuseSelectedChunks.png"));
	btnFuseSelectedChunks->setIconSize(QSize(40, 40));
	hLayout->addWidget(btnFuseSelectedChunks);

	fSeparate = new QFrame(widget);
	fSeparate->setObjectName(QStringLiteral("fSeparate"));
	fSeparate->setFrameShape(QFrame::VLine);
	fSeparate->setFrameShadow(QFrame::Sunken);
	hLayout->addWidget(fSeparate);

	btnReset = new QPushButton(widget);
	setStyledToolTip(btnReset, "Reset Chunks and Switch to Edition Mode");
	btnReset->setObjectName(QStringLiteral("btnReset"));
	sizePolicy1.setHeightForWidth(btnReset->sizePolicy().hasHeightForWidth());
	btnReset->setSizePolicy(sizePolicy1);
	btnReset->setMinimumSize(QSize(40, 40));
	btnReset->setMaximumSize(QSize(40, 40));
	btnReset->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnReset.png"));
	btnReset->setIconSize(QSize(40, 40));
	hLayout->addWidget(btnReset);

	btnSimulatePlay = new QPushButton(widget);
	setStyledToolTip(btnSimulatePlay, "Switch to Simulate Mode");
	btnSimulatePlay->setObjectName(QStringLiteral("btnSimulatePlay"));
	sizePolicy1.setHeightForWidth(btnSimulatePlay->sizePolicy().hasHeightForWidth());
	btnSimulatePlay->setSizePolicy(sizePolicy1);
	btnSimulatePlay->setMinimumSize(QSize(40, 40));
	btnSimulatePlay->setMaximumSize(QSize(40, 40));
	btnSimulatePlay->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnSimulatePlay.png"));
	btnSimulatePlay->setIconSize(QSize(40, 40));
	hLayout->addWidget(btnSimulatePlay);

	btnFrameStepForward = new QPushButton(widget);
	setStyledToolTip(btnFrameStepForward, "Switch to StepForward Mode");
	btnFrameStepForward->setObjectName(QStringLiteral("btnFrameStepForward"));
	sizePolicy1.setHeightForWidth(btnFrameStepForward->sizePolicy().hasHeightForWidth());
	btnFrameStepForward->setSizePolicy(sizePolicy1);
	btnFrameStepForward->setMinimumSize(QSize(40, 40));
	btnFrameStepForward->setMaximumSize(QSize(40, 40));
	btnFrameStepForward->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnFrameStepForward.png"));
	btnFrameStepForward->setIconSize(QSize(40, 40));
	hLayout->addWidget(btnFrameStepForward);

	fSeparate = new QFrame(widget);
	fSeparate->setObjectName(QStringLiteral("fSeparate"));
	fSeparate->setFrameShape(QFrame::VLine);
	fSeparate->setFrameShadow(QFrame::Sunken);
	hLayout->addWidget(fSeparate);

	btnBomb = new QPushButton(widget);
	setStyledToolTip(btnBomb, "Not Implement");
	btnBomb->setObjectName(QStringLiteral("btnBomb"));
	sizePolicy1.setHeightForWidth(btnBomb->sizePolicy().hasHeightForWidth());
	btnBomb->setSizePolicy(sizePolicy1);
	btnBomb->setMinimumSize(QSize(40, 40));
	btnBomb->setMaximumSize(QSize(40, 40));
	btnBomb->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnBomb.png"));
	btnBomb->setIconSize(QSize(40, 40));
	hLayout->addWidget(btnBomb);

	btnProjectile = new QPushButton(widget);
	setStyledToolTip(btnProjectile, "Throw a Box to Chunks");
	btnProjectile->setObjectName(QStringLiteral("btnProjectile"));
	sizePolicy1.setHeightForWidth(btnProjectile->sizePolicy().hasHeightForWidth());
	btnProjectile->setSizePolicy(sizePolicy1);
	btnProjectile->setMinimumSize(QSize(40, 40));
	btnProjectile->setMaximumSize(QSize(40, 40));
	btnProjectile->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnProjectile.png"));
	btnProjectile->setIconSize(QSize(40, 40));
	hLayout->addWidget(btnProjectile);

	btnDropObject = new QPushButton(widget);
	setStyledToolTip(btnDropObject, "Not Implement");
	btnDropObject->setObjectName(QStringLiteral("btnDropObject"));
	sizePolicy1.setHeightForWidth(btnDropObject->sizePolicy().hasHeightForWidth());
	btnDropObject->setSizePolicy(sizePolicy1);
	btnDropObject->setMinimumSize(QSize(40, 40));
	btnDropObject->setMaximumSize(QSize(40, 40));
	btnDropObject->setIcon(QIcon(":/AppMainWindow/images/Blast_ToolBar_btnDropObject.png"));
	btnDropObject->setIconSize(QSize(40, 40));
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
	btnPreferences->setIconSize(QSize(40, 40));
	hLayout->addWidget(btnPreferences);

	QSpacerItem *horizontalSpacer;
	horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
	hLayout->addItem(horizontalSpacer);

	this->setWidget(widget);

	connect(btnOpenProject, SIGNAL(clicked()), this, SLOT(on_btnOpenProject_clicked()));
	connect(btnSaveProject, SIGNAL(clicked()), this, SLOT(on_btnSaveProject_clicked()));
	connect(btnExportAssets, SIGNAL(clicked()), this, SLOT(on_btnExportAssets_clicked()));
	connect(btnExportFilepath, SIGNAL(clicked()), this, SLOT(on_btnExportFilepath_clicked()));
	connect(ssbiDepthPreview, SIGNAL(valueChanged(int)), this, SLOT(on_ssbiDepthPreview_valueChanged(int)));
	connect(cbExactCoverage, SIGNAL(stateChanged(int)), this, SLOT(on_cbExactCoverage_stateChanged(int)));
	connect(btnSelectTool, SIGNAL(clicked()), this, SLOT(on_btnSelectTool_clicked()));
	connect(btnPaintbrush, SIGNAL(clicked()), this, SLOT(on_btnPaintbrush_clicked()));
	connect(btnFractureTool, SIGNAL(clicked()), this, SLOT(on_btnFractureTool_clicked()));
	connect(btnExplodedViewTool, SIGNAL(clicked()), this, SLOT(on_btnExplodedViewTool_clicked()));
	connect(btnJointsTool, SIGNAL(clicked()), this, SLOT(on_btnJointsTool_clicked()));
	connect(btnFuseSelectedChunks, SIGNAL(clicked()), this, SLOT(on_btnFuseSelectedChunks_clicked()));
	connect(btnReset, SIGNAL(clicked()), this, SLOT(on_btnReset_clicked()));
	connect(btnSimulatePlay, SIGNAL(clicked()), this, SLOT(on_btnSimulatePlay_clicked()));
	connect(btnFrameStepForward, SIGNAL(clicked()), this, SLOT(on_btnFrameStepForward_clicked()));
	connect(btnBomb, SIGNAL(clicked()), this, SLOT(on_btnBomb_clicked()));
	connect(btnProjectile, SIGNAL(clicked()), this, SLOT(on_btnProjectile_clicked()));
	connect(btnDropObject, SIGNAL(clicked()), this, SLOT(on_btnDropObject_clicked()));
	connect(btnPreferences, SIGNAL(clicked()), this, SLOT(on_btnPreferences_clicked()));
}

void BlastToolbar::updateValues()
{
}

#include <Sample.h>
#include <SimpleScene.h>
#include <SampleManager.h>
#include <SceneController.h>
#include <SourceAssetOpenDlg.h>

void BlastToolbar::on_btnOpenProject_clicked()
{
	qDebug("%s", __FUNCTION__);

	SourceAssetOpenDlg dlg(true, &AppMainWindow::Inst());
	int res = dlg.exec();
	if (res != QDialog::Accepted || dlg.getFile().isEmpty())
		return;

	QFileInfo fileInfo(dlg.getFile());
	std::string dir = QDir::toNativeSeparators(fileInfo.absoluteDir().absolutePath()).toLocal8Bit();
	std::string file = fileInfo.baseName().toLocal8Bit();

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

	SimpleScene::Inst()->GetSampleManager().addModelAsset(file, dlg.getSkinned(), t, !dlg.isAppend());
}

void BlastToolbar::on_btnSaveProject_clicked()
{
	qDebug("%s", __FUNCTION__);
}

void BlastToolbar::on_btnExportAssets_clicked()
{
	qDebug("%s", __FUNCTION__);
}

void BlastToolbar::on_btnExportFilepath_clicked()
{
	qDebug("%s", __FUNCTION__);
}

void BlastToolbar::on_ssbiDepthPreview_valueChanged(int v)
{
	qDebug("%s", __FUNCTION__);
}

void BlastToolbar::on_cbExactCoverage_stateChanged(int state)
{
	qDebug("%s", __FUNCTION__);
}

void BlastToolbar::on_btnSelectTool_clicked()
{
	qDebug("%s", __FUNCTION__);
}

void BlastToolbar::on_pointselect_action()
{
	qDebug("%s", __FUNCTION__);

	SampleManager& sampleManager = SimpleScene::Inst()->GetSampleManager();
	sampleManager.setBlastToolType(BTT_Select);
}

void BlastToolbar::on_rectselect_action()
{
	qDebug("%s", __FUNCTION__);

	SampleManager& sampleManager = SimpleScene::Inst()->GetSampleManager();
	sampleManager.setBlastToolType(BTT_Select);
}

void BlastToolbar::on_drawselect_action()
{
	qDebug("%s", __FUNCTION__);
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
	qDebug("%s", __FUNCTION__);
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
	qDebug("%s", __FUNCTION__);

	SampleManager* pSampleManager = SampleManager::ins();
	SceneController& sceneController = pSampleManager->getSceneController();
	sceneController.ResetScene();
	pSampleManager->setBlastToolType(BTT_Edit);
}

void BlastToolbar::on_btnSimulatePlay_clicked()
{
	qDebug("%s", __FUNCTION__);

	SampleManager* pSampleManager = SampleManager::ins();
	pSampleManager->setBlastToolType(BTT_Damage);
}

void BlastToolbar::on_btnFrameStepForward_clicked()
{
	qDebug("%s", __FUNCTION__);

	SampleManager* pSampleManager = SampleManager::ins();
	pSampleManager->setBlastToolType(BTT_Damage);
	PhysXController& physXController = pSampleManager->getPhysXController();
	physXController.m_bForce = true;
}

void BlastToolbar::on_btnBomb_clicked()
{
	qDebug("%s", __FUNCTION__);
}

void BlastToolbar::on_btnProjectile_clicked()
{
	qDebug("%s", __FUNCTION__);

	SampleManager& sampleManager = SimpleScene::Inst()->GetSampleManager();
	SceneController& sceneController = sampleManager.getSceneController();
	sceneController.addProjectile();
}

void BlastToolbar::on_btnDropObject_clicked()
{
	qDebug("%s", __FUNCTION__);
}

void BlastToolbar::on_btnPreferences_clicked()
{
	qDebug("%s", __FUNCTION__);

	SampleManager* pSampleManager = SampleManager::ins();
	pSampleManager->saveAsset();
}