#include "CollisionToolsDlg.h"
#include "ui_CollisionToolsDlg.h"

CollisionToolsDlg::CollisionToolsDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CollisionToolsDlg)
{
    ui->setupUi(this);
}

CollisionToolsDlg::~CollisionToolsDlg()
{
    delete ui;
}

void CollisionToolsDlg::on_comboBoxApplyFilter_currentIndexChanged(int index)
{

}

void CollisionToolsDlg::on_comboBoxCollisionShape_currentIndexChanged(int index)
{

}

void CollisionToolsDlg::on_spinBoxQuality_valueChanged(int arg1)
{

}

void CollisionToolsDlg::on_spinBoxMaxHulls_valueChanged(int arg1)
{

}

void CollisionToolsDlg::on_spinBoxTrimHulls_valueChanged(int arg1)
{

}

void CollisionToolsDlg::on_comboBoxTargetPlatform_currentIndexChanged(int index)
{

}

void CollisionToolsDlg::on_btnReset_clicked()
{

}

void CollisionToolsDlg::on_btnApply_clicked()
{

}
