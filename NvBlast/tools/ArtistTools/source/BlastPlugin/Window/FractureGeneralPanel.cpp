#include "FractureGeneralPanel.h"
#include "ui_FractureGeneralPanel.h"
#include "ProjectParams.h"

FractureGeneralPanel::FractureGeneralPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FractureGeneralPanel)
{
    ui->setupUi(this);
}

FractureGeneralPanel::~FractureGeneralPanel()
{
    delete ui;
}

void FractureGeneralPanel::updateValues()
{
	BPPFractureGeneral& fractureGeneral = BlastProject::ins().getParams().fracture.general;

	ui->comboBoxFracturePreset->setCurrentIndex(fractureGeneral.fracturePreset);
	ui->comboBoxFractureType->setCurrentIndex(fractureGeneral.fractureType);
	ui->checkBoxAddDepth->setChecked(fractureGeneral.addDepth);
	ui->checkBoxPerChunk->setChecked(fractureGeneral.perChunk);
	ui->checkBoxNewMatID->setChecked(fractureGeneral.newMatID);
	ui->comboBoxApplyMaterial->setCurrentIndex(fractureGeneral.applyMaterial);
}

void FractureGeneralPanel::on_comboBoxFracturePreset_currentIndexChanged(int index)
{
	BPPFractureGeneral& fractureGeneral = BlastProject::ins().getParams().fracture.general;
	fractureGeneral.fracturePreset = index;
}

void FractureGeneralPanel::on_comboBoxFractureType_currentIndexChanged(int index)
{
	BPPFractureGeneral& fractureGeneral = BlastProject::ins().getParams().fracture.general;
	fractureGeneral.fractureType = index;
}

void FractureGeneralPanel::on_checkBoxAddDepth_stateChanged(int arg1)
{
	BPPFractureGeneral& fractureGeneral = BlastProject::ins().getParams().fracture.general;
	fractureGeneral.addDepth = (arg1 != 0 ? true : false);
}

void FractureGeneralPanel::on_checkBoxPerChunk_stateChanged(int arg1)
{
	BPPFractureGeneral& fractureGeneral = BlastProject::ins().getParams().fracture.general;
	fractureGeneral.perChunk = (arg1 != 0 ? true : false);
}

void FractureGeneralPanel::on_checkBoxNewMatID_stateChanged(int arg1)
{
	BPPFractureGeneral& fractureGeneral = BlastProject::ins().getParams().fracture.general;
	fractureGeneral.newMatID = (arg1 != 0 ? true:false);
}

void FractureGeneralPanel::on_comboBoxApplyMaterial_currentIndexChanged(int index)
{
	BPPFractureGeneral& fractureGeneral = BlastProject::ins().getParams().fracture.general;
	fractureGeneral.applyMaterial = index;
}
