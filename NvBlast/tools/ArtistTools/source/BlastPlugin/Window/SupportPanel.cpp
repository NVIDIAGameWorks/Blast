#include "SupportPanel.h"
#include "ui_SupportPanel.h"

SupportPanel::SupportPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SupportPanel)
{
    ui->setupUi(this);
}

SupportPanel::~SupportPanel()
{
    delete ui;
}

void SupportPanel::updateValues()
{

}

void SupportPanel::on_comboBoxHealthMask_currentIndexChanged(int index)
{

}

void SupportPanel::on_btnAddHealthMask_clicked()
{

}

void SupportPanel::on_btnPen_clicked()
{

}

void SupportPanel::on_btnRemove_clicked()
{

}

void SupportPanel::on_spinBoxBondStrength_valueChanged(double arg1)
{

}

void SupportPanel::on_checkBoxEnableJoint_stateChanged(int arg1)
{

}
