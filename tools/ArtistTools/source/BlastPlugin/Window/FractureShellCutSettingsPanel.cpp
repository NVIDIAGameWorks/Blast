#include "FractureShellCutSettingsPanel.h"
#include "ui_FractureShellCutSettingsPanel.h"
#include "ProjectParams.h"

FractureShellCutSettingsPanel::FractureShellCutSettingsPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FractureShellCutSettingsPanel)
{
    ui->setupUi(this);
}

FractureShellCutSettingsPanel::~FractureShellCutSettingsPanel()
{
    delete ui;
}

void FractureShellCutSettingsPanel::updateValues()
{
	BPPShellCut& shellCut = BlastProject::ins().getParams().fracture.shellCut;

	ui->spinBoxThickness->setValue(shellCut.thickness);
	ui->spinBoxThicknessVariation->setValue(shellCut.thicknessVariation);
}

void FractureShellCutSettingsPanel::on_spinBoxThickness_valueChanged(double arg1)
{
	BPPShellCut& shellCut = BlastProject::ins().getParams().fracture.shellCut;
	shellCut.thickness = arg1;
}

void FractureShellCutSettingsPanel::on_spinBoxThicknessVariation_valueChanged(double arg1)
{
	BPPShellCut& shellCut = BlastProject::ins().getParams().fracture.shellCut;
	shellCut.thicknessVariation = arg1;
}

void FractureShellCutSettingsPanel::on_btnApplyFracture_clicked()
{

}
