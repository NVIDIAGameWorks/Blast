#include "DisplayScenePanel.h"

#include "AppMainWindow.h"

#include "SimpleScene.h"
#include "GlobalSettings.h"

DisplayScenePanel::DisplayScenePanel(QWidget* parent)
	:
	QWidget(parent)
{
	ui.setupUi(this);
}

void DisplayScenePanel::on_btnVisualizeWind_stateChanged(int state)
{
	GlobalSettings::Inst().m_visualizeWind = state;
}

void DisplayScenePanel::on_btnShowGrid_stateChanged(int state)
{
	GlobalSettings::Inst().m_showGrid = state;
}

void DisplayScenePanel::on_btnShowAxis_stateChanged(int state)
{
	GlobalSettings::Inst().m_showAxis = state;
}

void DisplayScenePanel::on_btnShowWireframe_stateChanged(int state)
{
	GlobalSettings::Inst().m_showWireframe = state;
}

void DisplayScenePanel::on_btnUseLighting_stateChanged(int state)
{
	GlobalSettings::Inst().m_useLighting = state;
}

void DisplayScenePanel::on_cbRenderType_currentIndexChanged(int index)
{
	GlobalSettings::Inst().m_renderStyle = index;
}

void DisplayScenePanel::on_btnShowHUD_stateChanged(int state)
{
	GlobalSettings::Inst().m_showHUD = state;
}

void DisplayScenePanel::on_btnComputeStats_stateChanged(int state)
{
	GlobalSettings::Inst().m_computeStatistics = state;
}

void DisplayScenePanel::on_btnComputeProfile_stateChanged(int state)
{
	GlobalSettings::Inst().m_computeProfile = state;
}

void DisplayScenePanel::on_btnShowGraphicsMesh_stateChanged(int state)
{
	GlobalSettings::Inst().m_showGraphicsMesh = state;
}

void DisplayScenePanel::on_btnShowSkinnedOnly_stateChanged(int state)
{
	GlobalSettings::Inst().m_showSkinnedMeshOnly = state;
	AppMainWindow::Inst().updateUI();
}

void DisplayScenePanel::on_btnSkinningDQ_stateChanged(int state)
{
	GlobalSettings::Inst().m_useDQ = state;
}

void DisplayScenePanel::on_checkBoxGizmoWithLocal_stateChanged(int state)
{
	AppMainWindow::Inst().m_bGizmoWithLocal = state;
	CoreLib::Inst()->AppMainWindow_updateMainToolbar();
}

void DisplayScenePanel::on_checkBoxGizmoWithDepthTest_stateChanged(int state)
{
	AppMainWindow::Inst().m_bGizmoWithDepthTest = state;
}

void DisplayScenePanel::on_checkBoxShowPlane_stateChanged(int state)
{
	AppMainWindow::Inst().m_bShowPlane = state;
}

void DisplayScenePanel::updateValues()
{
	GlobalSettings& globalSettings = GlobalSettings::Inst();

	ui.btnShowGrid->setChecked(globalSettings.m_showGrid);
	ui.btnShowAxis->setChecked(globalSettings.m_showAxis);
	ui.cbRenderType->setCurrentIndex(globalSettings.m_renderStyle);
	ui.btnShowHUD->setChecked(globalSettings.m_showHUD);
	ui.btnComputeStats->setChecked(globalSettings.m_computeStatistics);
	ui.btnUseLighting->setChecked(globalSettings.m_useLighting);
	ui.btnShowGraphicsMesh->setChecked( globalSettings.m_showGraphicsMesh);
	ui.btnShowSkinnedOnly->setChecked( globalSettings.m_showSkinnedMeshOnly);
	ui.checkBoxGizmoWithLocal->setChecked(AppMainWindow::Inst().m_bGizmoWithLocal);
	ui.checkBoxGizmoWithDepthTest->setChecked(AppMainWindow::Inst().m_bGizmoWithDepthTest);
}
