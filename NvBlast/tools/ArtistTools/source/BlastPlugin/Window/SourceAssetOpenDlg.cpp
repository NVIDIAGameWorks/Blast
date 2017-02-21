#include "SourceAssetOpenDlg.h"
#include "ui_SourceAssetOpenDlg.h"
#include <QtWidgets/QFileDialog>
#include "AppMainWindow.h"

SourceAssetOpenDlg::SourceAssetOpenDlg(bool bOpenBlastFile, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SourceAssetOpenDlg)
{
    ui->setupUi(this);

	m_bOpenBlastFile = bOpenBlastFile;
	ui->buttonBox->button(QDialogButtonBox::Ok)->setFixedWidth(100);
	ui->buttonBox->button(QDialogButtonBox::Cancel)->setFixedWidth(100);
	ui->spinBoxDegree->setMaximum(180);
	ui->spinBoxDegree->setMinimum(-180);
}

SourceAssetOpenDlg::~SourceAssetOpenDlg()
{
    delete ui;
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

bool SourceAssetOpenDlg::isAppend()
{
	return ui->checkBoxAppend->isChecked();
}

void SourceAssetOpenDlg::on_btnOpenFile_clicked()
{
	QString lastDir = AppMainWindow::Inst()._lastFilePath;
	QString titleStr = "Open Source Asset File";

	QString filetype = "Source Asset (*.fbx)";
	if (m_bOpenBlastFile)
	{
		filetype = "Source Asset (*.bpxa)";
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
