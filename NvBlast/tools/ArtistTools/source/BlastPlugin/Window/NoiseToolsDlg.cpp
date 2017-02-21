#include "NoiseToolsDlg.h"
#include "ui_NoiseToolsDlg.h"

NoiseToolsDlg::NoiseToolsDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NoiseToolsDlg)
{
    ui->setupUi(this);
}

NoiseToolsDlg::~NoiseToolsDlg()
{
    delete ui;
}

void NoiseToolsDlg::on_comboBoxApplyByMaterial_currentIndexChanged(int index)
{

}

void NoiseToolsDlg::on_spinBoxAmplitude_valueChanged(double arg1)
{

}

void NoiseToolsDlg::on_spinBoxFrequency_valueChanged(double arg1)
{

}

void NoiseToolsDlg::on_spinBoxAmplitude_3_valueChanged(double arg1)
{

}

void NoiseToolsDlg::on_btnApply_clicked()
{

}
