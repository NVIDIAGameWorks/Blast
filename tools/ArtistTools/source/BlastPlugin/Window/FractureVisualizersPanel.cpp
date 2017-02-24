#include "FractureVisualizersPanel.h"
#include "ui_FractureVisualizersPanel.h"
#include "ProjectParams.h"

FractureVisualizersPanel::FractureVisualizersPanel(QWidget *parent) :
    QWidget(parent),
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
	BPPFractureVisualization& fractureVisualization = BlastProject::ins().getParams().fracture.visualization;

	ui->checkBoxFracturePreview->setChecked(fractureVisualization.fracturePreview);
	ui->checkBoxDisplayFractureWidget->setChecked(fractureVisualization.displayFractureWidget);
}

void FractureVisualizersPanel::on_checkBoxFracturePreview_stateChanged(int arg1)
{
	BPPFractureVisualization& fractureVisualization = BlastProject::ins().getParams().fracture.visualization;
	fractureVisualization.fracturePreview = (arg1 != 0 ? true : false);
}

void FractureVisualizersPanel::on_checkBoxDisplayFractureWidget_stateChanged(int arg1)
{
	BPPFractureVisualization& fractureVisualization = BlastProject::ins().getParams().fracture.visualization;
	fractureVisualization.displayFractureWidget = (arg1 != 0 ? true : false);
}

