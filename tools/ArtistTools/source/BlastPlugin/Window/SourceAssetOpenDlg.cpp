#include "SourceAssetOpenDlg.h"
#include "ui_SourceAssetOpenDlg.h"
#include <QtWidgets/QFileDialog>
#include "AppMainWindow.h"
#include "GlobalSettings.h"

SourceAssetOpenDlg::SourceAssetOpenDlg(int usefor, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SourceAssetOpenDlg)
{
    ui->setupUi(this);

	m_usefor = usefor;
	ui->buttonBox->button(QDialogButtonBox::Ok)->setFixedWidth(100);
	ui->buttonBox->button(QDialogButtonBox::Cancel)->setFixedWidth(100);
	ui->spinBoxDegree->setMaximum(180);
	ui->spinBoxDegree->setMinimum(-180);

	ui->spinBoxXPosition->setRange(-DBL_MAX, DBL_MAX);
	ui->spinBoxYPosition->setRange(-DBL_MAX, DBL_MAX);
	ui->spinBoxZPosition->setRange(-DBL_MAX, DBL_MAX);
	ui->spinBoxXAxis->setRange(-DBL_MAX, DBL_MAX);
	ui->spinBoxYAxis->setRange(-DBL_MAX, DBL_MAX);
	ui->spinBoxZAxis->setRange(-DBL_MAX, DBL_MAX);

	if (m_usefor == 2)
	{
		ui->fileLabel->setVisible(false);
		ui->lineEditFile->setVisible(false);
		ui->btnOpenFile->setVisible(false);

		ui->skinnedLabel->setVisible(false);
		ui->checkBoxSkinned->setVisible(false);

		ui->appendLabel->setVisible(false);
		ui->checkBoxAppend->setVisible(false);

		ui->preFracturedLabel->setVisible(false);
		ui->checkBoxPreFractured->setVisible(false);
	}

	GlobalSettings& globalSettings = GlobalSettings::Inst();
	ui->cbSceneUnit->setCurrentIndex(globalSettings.m_sceneUnitIndex);
	
	if (m_usefor != 0)
	{
		ui->autoComputeLabel->setVisible(false);
		ui->checkBoxAutoCompute->setVisible(false);
	}
}

SourceAssetOpenDlg::~SourceAssetOpenDlg()
{
    delete ui;
}

void SourceAssetOpenDlg::setDefaultFile(const QString& fn)
{
	ui->lineEditFile->setText(fn);
}

QString SourceAssetOpenDlg::getFile()
{
    return ui->lineEditFile->text();
}

bool SourceAssetOpenDlg::getSkinned()
{
    return ui->checkBoxSkinned->isChecked();
}

QVector3D SourceAssetOpenDlg::getPosition()
{
    return QVector3D(ui->spinBoxXPosition->value(), ui->spinBoxYPosition->value(), ui->spinBoxZPosition->value());
}

QVector3D SourceAssetOpenDlg::getRotationAxis()
{
    return QVector3D(ui->spinBoxXAxis->value(), ui->spinBoxYAxis->value(), ui->spinBoxZAxis->value());
}

double SourceAssetOpenDlg::getRotationDegree()
{
	return ui->spinBoxDegree->value();
}

int  SourceAssetOpenDlg::sceneUnitIndex()
{
	return ui->cbSceneUnit->currentIndex();
}

bool SourceAssetOpenDlg::isAppend()
{
	return ui->checkBoxAppend->isChecked();
}

bool SourceAssetOpenDlg::isPreFractured()
{
	return ui->checkBoxPreFractured->isChecked();
}

bool SourceAssetOpenDlg::isAutoCompute()
{
	return ui->checkBoxAutoCompute->isChecked();
}

void SourceAssetOpenDlg::on_btnOpenFile_clicked()
{
	QString lastDir = AppMainWindow::Inst()._lastFilePath;
	QString titleStr = "Open Source Asset File";

	QString filetype = "Source Asset (*.fbx)";
	if (m_usefor == 1)
	{
		filetype = "Source Asset (*.blast)";
	}
	QString fileName = QFileDialog::getOpenFileName(this, titleStr, lastDir, filetype);
	if (!fileName.isEmpty())
	{
		QFileInfo fileInfo(fileName);
		AppMainWindow::Inst()._lastFilePath = fileInfo.absoluteDir().absolutePath();
	}

	ui->lineEditFile->setText(fileName);
}

void SourceAssetOpenDlg::on_buttonBox_accepted()
{

}

void SourceAssetOpenDlg::on_buttonBox_rejected()
{

}

void SourceAssetOpenDlg::on_checkBoxPreFractured_stateChanged(int arg1)
{
	if (!ui->checkBoxPreFractured->isChecked())
	{
		ui->checkBoxAutoCompute->setChecked(false);
	}
}

void SourceAssetOpenDlg::on_checkBoxAutoCompute_stateChanged(int arg1)
{
	if (ui->checkBoxAutoCompute->isChecked())
	{
		ui->checkBoxPreFractured->setChecked(true);
	}
}