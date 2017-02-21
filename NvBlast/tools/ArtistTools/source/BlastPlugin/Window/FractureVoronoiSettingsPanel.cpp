#include "FractureVoronoiSettingsPanel.h"
#include "ui_FractureVoronoiSettingsPanel.h"
#include "ProjectParams.h"
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtCore/QFileInfo>
#include "AppMainWindow.h"
#include "ProjectParams.h"
#include "SimpleScene.h"
#include "SampleManager.h"

FractureVoronoiSettingsPanel::FractureVoronoiSettingsPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FractureVoronoiSettingsPanel),
	_updateData(true)
{
    ui->setupUi(this);

    ui->groupBoxVisualizers->hide();
}

FractureVoronoiSettingsPanel::~FractureVoronoiSettingsPanel()
{
    delete ui;
}

void FractureVoronoiSettingsPanel::updateValues()
{
	_updateData = false;
	BPPVoronoi& voronoi = BlastProject::ins().getParams().fracture.voronoi;

	ui->spinBoxNumberOfSites->setValue(voronoi.numSites);
	ui->comboBoxSiteGeneration->setCurrentIndex(voronoi.siteGeneration);
	ui->spinBoxGridSize->setValue(voronoi.gridSize);
	ui->spinBoxGridScale->setValue(voronoi.gridScale);
	ui->spinBoxAmplitude->setValue(voronoi.amplitude);
	ui->spinBoxFrequency->setValue(voronoi.frequency);

	_updatePaintMaskComboBox();

	_updateMeshCutterComboBox();
	ui->checkBoxFractureInsideCutter->setChecked(voronoi.fractureInsideCutter);
	ui->checkBoxFractureOutsideCutter->setChecked(voronoi.fractureOutsideCutter);

	ui->spinBoxTextureSites->setValue(voronoi.numTextureSites);

	_updateTextureListWidget();
	_updateData = true;
}

void FractureVoronoiSettingsPanel::on_checkBoxGridPreview_stateChanged(int arg1)
{

}

void FractureVoronoiSettingsPanel::on_checkBoxFracturePreview_stateChanged(int arg1)
{

}

void FractureVoronoiSettingsPanel::on_checkBoxCutterMesh_stateChanged(int arg1)
{

}

void FractureVoronoiSettingsPanel::on_spinBoxNumberOfSites_valueChanged(int arg1)
{
	BPPVoronoi& voronoi = BlastProject::ins().getParams().fracture.voronoi;
	voronoi.numSites = arg1;
}

void FractureVoronoiSettingsPanel::on_comboBoxSiteGeneration_currentIndexChanged(int index)
{
	if (!_updateData)
		return;

	BPPVoronoi& voronoi = BlastProject::ins().getParams().fracture.voronoi;
	voronoi.siteGeneration = index;
}

void FractureVoronoiSettingsPanel::on_spinBoxGridSize_valueChanged(int arg1)
{
	BPPVoronoi& voronoi = BlastProject::ins().getParams().fracture.voronoi;
	voronoi.gridSize = arg1;
}

void FractureVoronoiSettingsPanel::on_spinBoxGridScale_valueChanged(double arg1)
{
	BPPVoronoi& voronoi = BlastProject::ins().getParams().fracture.voronoi;
	voronoi.gridScale = arg1;
}

void FractureVoronoiSettingsPanel::on_spinBoxAmplitude_valueChanged(double arg1)
{
	BPPVoronoi& voronoi = BlastProject::ins().getParams().fracture.voronoi;
	voronoi.amplitude = arg1;
}

void FractureVoronoiSettingsPanel::on_spinBoxFrequency_valueChanged(int arg1)
{
	BPPVoronoi& voronoi = BlastProject::ins().getParams().fracture.voronoi;
	voronoi.frequency = arg1;
}

void FractureVoronoiSettingsPanel::on_comboBoxPaintMasks_currentIndexChanged(int index)
{
	if (!_updateData)
		return;

	BPPVoronoi& voronoi = BlastProject::ins().getParams().fracture.voronoi;
	voronoi.activePaintMask = index;
}

void FractureVoronoiSettingsPanel::on_btnAddPaintMasks_clicked()
{
	bool ok = false;
	QString name = QInputDialog::getText(this,
		tr("Blast Tool"),
		tr("Please input name for new paint mask:"),
		QLineEdit::Normal,
		"",
		&ok);
	bool nameExist = BlastProject::ins().isPaintMaskNameExist(name.toUtf8().data());
	if (ok && !name.isEmpty() && !nameExist)
	{
		BlastProject::ins().addPaintMasks(name.toUtf8().data());
		_updatePaintMaskComboBox();
		ui->comboBoxPaintMasks->setCurrentIndex(ui->comboBoxPaintMasks->count() - 1);
	}
	else if (ok && nameExist)
	{
		QMessageBox::warning(this, "Blast Tool", "The name you input is already exist!");
	}
	else if (ok && name.isEmpty())
	{
		QMessageBox::warning(this, "Blast Tool", "You need input a name for the new paint mask!");
	}
}

void FractureVoronoiSettingsPanel::on_btnRemovePaintMasks_clicked()
{
	if (ui->comboBoxPaintMasks->currentIndex() > -1)
	{
		BlastProject::ins().removePaintMasks(ui->comboBoxPaintMasks->currentText().toUtf8().data());
		_updatePaintMaskComboBox();
	}
}

void FractureVoronoiSettingsPanel::on_comboBoxMeshCutter_currentIndexChanged(int index)
{
	if (!_updateData)
		return;

	BPPVoronoi& voronoi = BlastProject::ins().getParams().fracture.voronoi;
	voronoi.activeMeshCutter = index;
}

void FractureVoronoiSettingsPanel::on_btnAddMeshCutter_clicked()
{
	bool ok = false;
	QString name = QInputDialog::getText(this,
		tr("Blast Tool"),
		tr("Please input name for new mesh cutter:"),
		QLineEdit::Normal,
		"",
		&ok);
	bool nameExist = BlastProject::ins().isMeshCutterNameExist(name.toUtf8().data());
	if (ok && !name.isEmpty() && !nameExist)
	{
		BlastProject::ins().addMeshCutter(name.toUtf8().data());
		_updateMeshCutterComboBox();
		ui->comboBoxMeshCutter->setCurrentIndex(ui->comboBoxMeshCutter->count() - 1);
	}
	else if (ok && nameExist)
	{
		QMessageBox::warning(this, "Blast Tool", "The name you input is already exist!");
	}
	else if (ok && name.isEmpty())
	{
		QMessageBox::warning(this, "Blast Tool", "You need input a name for the new cutter mesh!");
	}
}

void FractureVoronoiSettingsPanel::on_btnRemoveMeshCutter_clicked()
{
	if (ui->comboBoxMeshCutter->currentIndex() > -1)
	{
		BlastProject::ins().removeMeshCutter(ui->comboBoxMeshCutter->currentText().toUtf8().data());
		_updateMeshCutterComboBox();
	}
}

void FractureVoronoiSettingsPanel::on_checkBoxFractureInsideCutter_stateChanged(int arg1)
{
	BPPVoronoi& voronoi = BlastProject::ins().getParams().fracture.voronoi;
	voronoi.fractureInsideCutter = (arg1 != 0 ? true : false);
}

void FractureVoronoiSettingsPanel::on_checkBoxFractureOutsideCutter_stateChanged(int arg1)
{
	BPPVoronoi& voronoi = BlastProject::ins().getParams().fracture.voronoi;
	voronoi.fractureOutsideCutter = (arg1 != 0 ? true : false);
}

void FractureVoronoiSettingsPanel::on_btnAddTexture_clicked()
{
	QString texName = AppMainWindow::Inst().OpenTextureFile();

	if (texName.isEmpty())
		return;

	QFileInfo fileInfo(texName);
	QByteArray ba = fileInfo.absoluteFilePath().toLocal8Bit();
	const char* filePath = (const char*)(ba);

	if (!BlastProject::ins().isVoronoiTextureNameExist(texName.toUtf8().data()))
	{
		BlastProject::ins().addVoronoiTexture(filePath);
		_updateTextureListWidget();
		ui->listWidgetTexture->setCurrentRow(ui->listWidgetTexture->count() - 1);
	}
	else
	{
		QMessageBox::warning(this, "Blast Tool", "The texture you selected is already exist!");
	}
}

void FractureVoronoiSettingsPanel::on_btnReloadTexture_clicked()
{

}

void FractureVoronoiSettingsPanel::on_btnRemoveTexture_clicked()
{
	if (ui->listWidgetTexture->currentRow() != -1)
	{
		QListWidgetItem *item = ui->listWidgetTexture->currentItem();
		QString texture = _getTexturePathByName(item->text());
		QByteArray ba = texture.toLocal8Bit();
		BlastProject::ins().removeVoronoiTexture(ba.data());
		_updateTextureListWidget();
	}
}

void FractureVoronoiSettingsPanel::on_spinBoxTextureSites_valueChanged(int arg1)
{
	BPPVoronoi& voronoi = BlastProject::ins().getParams().fracture.voronoi;
	voronoi.numTextureSites = arg1;
}

void FractureVoronoiSettingsPanel::on_listWidgetTexture_itemSelectionChanged()
{

}

void FractureVoronoiSettingsPanel::on_btnTextureMap_clicked()
{

}

void FractureVoronoiSettingsPanel::on_btnApplyFracture_clicked()
{
	BPPVoronoi& voronoi = BlastProject::ins().getParams().fracture.voronoi;
	VoronoiFractureExecutor executor;
	executor.setCellsCount(voronoi.numSites);

	bool multiplyChunksSelected = false;
	std::map<BlastAsset*, std::vector<uint32_t>> selectedChunks = SampleManager::ins()->getSelectedChunks();
	std::map<BlastAsset*, std::vector<uint32_t>>::iterator itrAssetSelectedChunks = selectedChunks.begin();

	if (selectedChunks.size() > 1)
	{
		multiplyChunksSelected = true;
	}
	else if (selectedChunks.size() == 1 && itrAssetSelectedChunks->second.size() > 1)
	{
		multiplyChunksSelected = true;
	}
	else if ((selectedChunks.size() == 1 && itrAssetSelectedChunks->second.size() == 0) || (selectedChunks.size() == 0))
	{
		return;
	}

	if (multiplyChunksSelected)
	{
		QMessageBox::warning(NULL, "Blast Tool", "Now, this tool can only fracture one chunk!");
		return;
	}

	executor.setSourceAsset(itrAssetSelectedChunks->first);
	executor.setTargetChunk(itrAssetSelectedChunks->second.at(0));
	executor.execute();
}

QString FractureVoronoiSettingsPanel::_getTexturePathByName(const QString& name)
{
	BPPVoronoi& voronoi = BlastProject::ins().getParams().fracture.voronoi;
	BPPStringArray& textureArray = voronoi.textureSites;

	int count = textureArray.arraySizes[0];
	for (int i = 0; i < count; ++i)
	{
		QFileInfo fileInfo(textureArray.buf[i].buf);
		if (fileInfo.baseName() == name)
			return textureArray.buf[i].buf;
	}

	return "";
}

void FractureVoronoiSettingsPanel::_updatePaintMaskComboBox()
{
	BPPVoronoi& voronoi = BlastProject::ins().getParams().fracture.voronoi;

	ui->comboBoxPaintMasks->clear();
	QStringList items;
	for (int i = 0; i < voronoi.paintMasks.arraySizes[0]; ++i)
	{
		items.append(voronoi.paintMasks.buf[i].buf);
	}
	ui->comboBoxPaintMasks->addItems(items);
	ui->comboBoxPaintMasks->setCurrentIndex(voronoi.activePaintMask);
}

void FractureVoronoiSettingsPanel::_updateMeshCutterComboBox()
{
	BPPVoronoi& voronoi = BlastProject::ins().getParams().fracture.voronoi;

	ui->comboBoxMeshCutter->clear();
	QStringList items;
	for (int i = 0; i < voronoi.meshCutters.arraySizes[0]; ++i)
	{
		items.append(voronoi.meshCutters.buf[i].buf);
	}
	ui->comboBoxMeshCutter->addItems(items);
	ui->comboBoxMeshCutter->setCurrentIndex(voronoi.activeMeshCutter);
}

void FractureVoronoiSettingsPanel::_updateTextureListWidget()
{
	BPPVoronoi& voronoi = BlastProject::ins().getParams().fracture.voronoi;

	ui->listWidgetTexture->clear();
	QStringList items;
	for (int i = 0; i < voronoi.textureSites.arraySizes[0]; ++i)
	{
		QFileInfo fileInfo(voronoi.textureSites.buf[i].buf);
		QByteArray ba = fileInfo.baseName().toLocal8Bit();
		const char* texture = (const char*)(ba);
		items.append(texture);
	}
	ui->listWidgetTexture->addItems(items);
}
