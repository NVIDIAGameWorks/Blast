#include "FractureVisualizersPanel.h"
#include "ui_FractureVisualizersPanel.h"
#include "ProjectParams.h"
#include "FractureGeneralPanel.h"
#include "SampleManager.h"

FractureVisualizersPanel::FractureVisualizersPanel(QWidget *parent) :
	QWidget(parent),
	_updateData(true),
    ui(new Ui::FractureVisualizersPanel)
{
    ui->setupUi(this);
}

FractureVisualizersPanel::~FractureVisualizersPanel()
{
    delete ui;
}

void FractureVisualizersPanel::updateValues()
{
	_updateData = false;
/*
	BPPFractureVisualization* fractureVisualization = _getBPPVisualization();

	ui->checkBoxFracturePreview->setChecked(fractureVisualization->fracturePreview);
	ui->checkBoxDisplayFractureWidget->setChecked(fractureVisualization->displayFractureWidget);
*/
	bool checked = BlastProject::ins().getParams().fracture.general.selectionDepthTest;
	ui->checkBoxSelectionDepthTest->setChecked(checked);
	_updateData = true;
}
/*
void FractureVisualizersPanel::on_checkBoxFracturePreview_stateChanged(int arg1)
{
	if (!_updateData)
		return;

	BPPFractureVisualization* fractureVisualization = _getBPPVisualization();
	fractureVisualization->fracturePreview = (arg1 != 0 ? true : false);
}

void FractureVisualizersPanel::on_checkBoxDisplayFractureWidget_stateChanged(int arg1)
{
	if (!_updateData)
		return;

	BPPFractureVisualization* fractureVisualization = _getBPPVisualization();
	fractureVisualization->displayFractureWidget = (arg1 != 0 ? true : false);
}
*/
void FractureVisualizersPanel::on_checkBoxSelectionDepthTest_stateChanged(int arg1)
{
	BlastProject::ins().getParams().fracture.general.selectionDepthTest = arg1;
	SampleManager* pSampleManager = SampleManager::ins();
	if (nullptr != pSampleManager)
	{
		SampleManager::ins()->ApplySelectionDepthTest();
	}
}

BPPFractureVisualization* FractureVisualizersPanel::_getBPPVisualization()
{
	BPPFractureVisualization* visualization = nullptr;
	FracturePreset* preset = _generalPanel->getCurrentFracturePreset();
	if (nullptr != preset)
	{
		visualization = &(preset->visualization);
	}
	else
	{
		visualization = &(BlastProject::ins().getParams().fracture.visualization);
	}
	return visualization;
}
