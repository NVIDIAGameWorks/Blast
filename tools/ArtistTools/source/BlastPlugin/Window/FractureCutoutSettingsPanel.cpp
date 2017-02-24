#include "FractureCutoutSettingsPanel.h"
#include "ui_FractureCutoutSettingsPanel.h"
#include "ProjectParams.h"
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtCore/QFileInfo>
#include "AppMainWindow.h"

FractureCutoutSettingsPanel::FractureCutoutSettingsPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FractureCutoutSettingsPanel)
{
    ui->setupUi(this);
}

FractureCutoutSettingsPanel::~FractureCutoutSettingsPanel()
{
    delete ui;
}

void FractureCutoutSettingsPanel::updateValues()
{
	BPPCutoutProjection& cutoutProjection = BlastProject::ins().getParams().fracture.cutoutProjection;

	_updateTextureListWidget();

	ui->comboBoxCutoutType->setCurrentIndex(cutoutProjection.cutoutType);
	ui->spinBoxPixelThreshold->setValue(cutoutProjection.pixelThreshold);
	ui->checkBoxTiled->setChecked(cutoutProjection.tiled);
	ui->checkBoxInvertU->setChecked(cutoutProjection.invertU);
	ui->checkBoxInvertV->setChecked(cutoutProjection.invertV);
}

void FractureCutoutSettingsPanel::on_btnAddTexture_clicked()
{
	QString texName = AppMainWindow::Inst().OpenTextureFile();

	if (texName.isEmpty())
		return;

	QFileInfo fileInfo(texName);
	QByteArray ba = fileInfo.absoluteFilePath().toLocal8Bit();
	const char* filePath = (const char*)(ba);

	if (!BlastProject::ins().isCutoutTextureNameExist(texName.toUtf8().data()))
	{
		BlastProject::ins().addCutoutTexture(filePath);
		_updateTextureListWidget();
		ui->listWidget->setCurrentRow(ui->listWidget->count() - 1);
	}
	else
	{
		QMessageBox::warning(this, "Blast Tool", "The texture you selected is already exist!");
	}
}

void FractureCutoutSettingsPanel::on_btnReloadTexture_clicked()
{

}

void FractureCutoutSettingsPanel::on_btnRemoveTexture_clicked()
{
	if (ui->listWidget->currentRow() != -1)
	{
		QListWidgetItem *item = ui->listWidget->currentItem();
		QString texture = _getTexturePathByName(item->text());
		QByteArray ba = texture.toLocal8Bit();
		BlastProject::ins().removeCutoutTexture(ba.data());
		_updateTextureListWidget();
	}
}

void FractureCutoutSettingsPanel::on_listWidget_currentRowChanged(int currentRow)
{

}

void FractureCutoutSettingsPanel::on_btnTextureMap_clicked()
{

}

void FractureCutoutSettingsPanel::on_comboBoxCutoutType_currentIndexChanged(int index)
{
	BPPCutoutProjection& cutoutProjection = BlastProject::ins().getParams().fracture.cutoutProjection;
	cutoutProjection.cutoutType = index;
}

void FractureCutoutSettingsPanel::on_spinBoxPixelThreshold_valueChanged(int arg1)
{
	BPPCutoutProjection& cutoutProjection = BlastProject::ins().getParams().fracture.cutoutProjection;
	cutoutProjection.pixelThreshold = arg1;
}

void FractureCutoutSettingsPanel::on_checkBoxTiled_stateChanged(int arg1)
{
	BPPCutoutProjection& cutoutProjection = BlastProject::ins().getParams().fracture.cutoutProjection;
	cutoutProjection.tiled = (arg1 != 0 ? true : false);
}

void FractureCutoutSettingsPanel::on_checkBoxInvertU_stateChanged(int arg1)
{
	BPPCutoutProjection& cutoutProjection = BlastProject::ins().getParams().fracture.cutoutProjection;
	cutoutProjection.invertU = (arg1 != 0 ? true : false);
}

void FractureCutoutSettingsPanel::on_checkBoxInvertV_stateChanged(int arg1)
{
	BPPCutoutProjection& cutoutProjection = BlastProject::ins().getParams().fracture.cutoutProjection;
	cutoutProjection.invertV = (arg1 != 0 ? true : false);
}

void FractureCutoutSettingsPanel::on_btnFitToObject_clicked()
{

}

void FractureCutoutSettingsPanel::on_btnApplyFracture_clicked()
{

}

QString FractureCutoutSettingsPanel::_getTexturePathByName(const QString& name)
{
	BPPCutoutProjection& cutoutProjection = BlastProject::ins().getParams().fracture.cutoutProjection;
	BPPStringArray& textureArray = cutoutProjection.textures;

	int count = textureArray.arraySizes[0];
	for (int i = 0; i < count; ++i)
	{
		QFileInfo fileInfo(textureArray.buf[i].buf);
		if (fileInfo.baseName() == name)
			return textureArray.buf[i].buf;
	}

	return "";
}

void FractureCutoutSettingsPanel::_updateTextureListWidget()
{
	BPPCutoutProjection& cutoutProjection = BlastProject::ins().getParams().fracture.cutoutProjection;

	ui->listWidget->clear();
	QStringList items;
	for (int i = 0; i < cutoutProjection.textures.arraySizes[0]; ++i)
	{
		QFileInfo fileInfo(cutoutProjection.textures.buf[i].buf);
		QByteArray ba = fileInfo.baseName().toLocal8Bit();
		const char* texture = (const char*)(ba);
		items.append(texture);
	}
	ui->listWidget->addItems(items);
}
