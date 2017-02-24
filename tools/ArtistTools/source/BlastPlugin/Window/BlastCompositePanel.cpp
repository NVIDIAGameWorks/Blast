#include "BlastCompositePanel.h"
#include "ui_BlastCompositePanel.h"
#include "ProjectParams.h"
#include <QtCore/QFileInfo>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>

BlastCompositePanel::BlastCompositePanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BlastCompositePanel),
	_selectedAsset(-1)
{
    ui->setupUi(this);

	ui->btnRemoveAsset->setEnabled(false);
	ui->btnModifyLandmark->setEnabled(false);
	ui->btnRemoveLandmark->setEnabled(false);
}

BlastCompositePanel::~BlastCompositePanel()
{
    delete ui;
}

void BlastCompositePanel::updateValues()
{
	BPParams& projectParams = BlastProject::ins().getParams();
	BPPComposite& composite = projectParams.blast.composite;

	ui->lineEditComposite->setText(composite.composite.buf);

	_updateAssetComboBox();

	_updateAssetInstanceListWidget();

	ui->spinBoxBondByThreshold->setValue(composite.bondThreshold);
	ui->spinBoxNewBondStrength->setValue(composite.bondStrength);

	_updateLandmarkListWidget();

	ui->checkBoxEnableLandmark->setChecked(false);
	ui->spinBoxLandmarkRadius->setValue(false);
}

void BlastCompositePanel::on_btnCollapse_clicked()
{

}

void BlastCompositePanel::on_btnSave_clicked()
{
	BPParams& projectParams = BlastProject::ins().getParams();
	BPPComposite& composite = projectParams.blast.composite;

	QByteArray tmp = ui->lineEditComposite->text().toUtf8();
	copy(composite.composite, tmp.data());
}

void BlastCompositePanel::on_comboBoxAsset_currentIndexChanged(int index)
{
	_selectedAsset = index;
}

void BlastCompositePanel::on_btnAddAsset_clicked()
{
	bool ok = false;
	QString name = QInputDialog::getText(this,
		tr("Blast Tool"),
		tr("Please input name for new blast instance:"),
		QLineEdit::Normal,
		"",
		&ok);
	bool nameExist = BlastProject::ins().isAssetInstanceNameExist(name.toUtf8().data());
	if (ok && !name.isEmpty() && !nameExist)
	{
		BlastProject::ins().addAssetInstance(_selectedAsset, name.toUtf8().data());
		_updateAssetInstanceListWidget();
		ui->listWidgetBlastAsset->setCurrentRow(ui->listWidgetBlastAsset->count() - 1);
	}
	else if (ok && nameExist)
	{
		QMessageBox::warning(this, "Blast Tool", "The name you input is already exist!");
	}
	else if (ok && name.isEmpty())
	{
		QMessageBox::warning(this, "Blast Tool", "You need input a name for the new preset!");
	}
}

void BlastCompositePanel::on_btnRemoveAsset_clicked()
{
	QList<QListWidgetItem*> items = ui->listWidgetBlastAsset->selectedItems();
	BlastProject::ins().removeAssetInstance(items.at(0)->text().toUtf8().data());
}

void BlastCompositePanel::on_listWidgetBlastAsset_itemSelectionChanged()
{
	QList<QListWidgetItem*> items = ui->listWidgetBlastAsset->selectedItems();
	if (items.count() > 0)
		ui->btnRemoveAsset->setEnabled(true);
	else
		ui->btnRemoveAsset->setEnabled(false);
}

void BlastCompositePanel::on_spinBoxBondByThreshold_valueChanged(double arg1)
{
	BPParams& projectParams = BlastProject::ins().getParams();
	BPPComposite& composite = projectParams.blast.composite;
	composite.bondThreshold = arg1;
}

void BlastCompositePanel::on_spinBoxNewBondStrength_valueChanged(double arg1)
{
	BPParams& projectParams = BlastProject::ins().getParams();
	BPPComposite& composite = projectParams.blast.composite;
	composite.bondStrength = arg1;
}

void BlastCompositePanel::on_btnAddLandmark_clicked()
{
	bool ok = false;
	QString name = QInputDialog::getText(this,
		tr("Blast Tool"),
		tr("Please input name for new landmark:"),
		QLineEdit::Normal,
		"",
		&ok);
	bool nameExist = BlastProject::ins().isLandmarkNameExist(name.toUtf8().data());
	if (ok && !name.isEmpty() && !nameExist)
	{
		BlastProject::ins().addLandmark(name.toUtf8().data());
		_updateLandmarkListWidget();
		ui->listWidgetLandmark->setCurrentRow(ui->listWidgetLandmark->count() - 1);
	}
	else if (ok && nameExist)
	{
		QMessageBox::warning(this, "Blast Tool", "The name you input is already exist!");
	}
	else if (ok && name.isEmpty())
	{
		QMessageBox::warning(this, "Blast Tool", "You need input a name for the new landmark!");
	}
}

void BlastCompositePanel::on_btnModifyLandmark_clicked()
{
	QList<QListWidgetItem*> items = ui->listWidgetLandmark->selectedItems();
	QByteArray tem = items.at(0)->text().toUtf8();
	const char* oldName = tem.data();

	bool ok = false;
	QString newName = QInputDialog::getText(this,
		tr("Blast Tool"),
		tr("Please input new name for the selected landmark:"),
		QLineEdit::Normal,
		oldName,
		&ok);
	bool nameExist = BlastProject::ins().isLandmarkNameExist(newName.toUtf8().data());
	if (ok && !newName.isEmpty() && !nameExist)
	{
		int selectIndex = ui->listWidgetLandmark->currentRow();
		BlastProject::ins().renameLandmark(oldName, newName.toUtf8().data());
		_updateLandmarkListWidget();
		ui->listWidgetLandmark->setCurrentRow(selectIndex);
	}
	else if (ok && nameExist)
	{
		QMessageBox::warning(this, "Blast Tool", "The name you input is already exist!");
	}
	else if (ok && newName.isEmpty())
	{
		QMessageBox::warning(this, "Blast Tool", "You need input a name for the new landmark!");
	}
}

void BlastCompositePanel::on_btnRemoveLandmark_clicked()
{
	QList<QListWidgetItem*> items = ui->listWidgetLandmark->selectedItems();
	QByteArray tem = items.at(0)->text().toUtf8();
	const char* name = tem.data();
	BlastProject::ins().removeLandmark(name);
	_updateLandmarkListWidget();
}

void BlastCompositePanel::on_listWidgetLandmark_itemSelectionChanged()
{
	_updateLandmarkUIs();
}

void BlastCompositePanel::on_checkBoxEnableLandmark_stateChanged(int arg1)
{
	QList<QListWidgetItem*> items = ui->listWidgetLandmark->selectedItems();

	int count = items.count();
	for (int i = 0; i < count; ++i)
	{
		BPPLandmark* landmark = BlastProject::ins().getLandmark(items.at(0)->text().toUtf8().data());
		if (landmark != nullptr)
		{
			landmark->enable = ui->checkBoxEnableLandmark->isChecked();
		}
	}
}

void BlastCompositePanel::on_spinBoxLandmarkRadius_valueChanged(double arg1)
{
	QList<QListWidgetItem*> items = ui->listWidgetLandmark->selectedItems();

	int count = items.count();
	for (int i = 0; i < count; ++i)
	{
		BPPLandmark* landmark = BlastProject::ins().getLandmark(items.at(0)->text().toUtf8().data());
		if (landmark != nullptr)
		{
			landmark->radius = ui->spinBoxLandmarkRadius->value();
		}
	}
}

void BlastCompositePanel::_updateAssetComboBox()
{
	BPParams& projectParams = BlastProject::ins().getParams();
	BPPAssetArray& blastAssetArray = projectParams.blast.blastAssets;
	BPPComposite& composite = projectParams.blast.composite;

	ui->comboBoxAsset->clear();
	int countAssets = blastAssetArray.arraySizes[0];
	if (countAssets > 0)
	{
		QStringList assets;
		for (int i = 0; i < countAssets; ++i)
		{
			QFileInfo fileInfo(blastAssetArray.buf[i].path.buf);
			assets.append(fileInfo.baseName());
		}
		ui->comboBoxAsset->addItems(assets);
	}
	else
	{
		ui->btnAddAsset->setEnabled(false);
		ui->btnRemoveAsset->setEnabled(false);
	}
}

void BlastCompositePanel::_updateAssetInstanceListWidget()
{
	BPParams& projectParams = BlastProject::ins().getParams();
	BPPComposite& composite = projectParams.blast.composite;

	ui->listWidgetBlastAsset->clear();
	int countAssetInstances = composite.blastAssetInstances.arraySizes[0];
	if (countAssetInstances > 0)
	{
		QStringList assetInstances;

		for (int i = 0; i < countAssetInstances; ++i)
		{
			assetInstances.append(composite.blastAssetInstances.buf[i].name.buf);
		}
		ui->listWidgetBlastAsset->addItems(assetInstances);
	}
}

void BlastCompositePanel::_updateLandmarkListWidget()
{
	BPParams& projectParams = BlastProject::ins().getParams();
	BPPComposite& composite = projectParams.blast.composite;

	ui->listWidgetLandmark->clear();
	int countJoints = composite.landmarks.arraySizes[0];
	if (countJoints > 0)
	{
		QStringList landmarks;

		for (int i = 0; i < countJoints; ++i)
		{
			landmarks.append(composite.landmarks.buf[i].name.buf);
		}
		ui->listWidgetLandmark->addItems(landmarks);
	}
}

void BlastCompositePanel::_updateLandmarkUIs()
{
	QList<QListWidgetItem*> items = ui->listWidgetLandmark->selectedItems();

	if (items.count() > 0)
	{
		BPPLandmark* landmark = BlastProject::ins().getLandmark(items.at(0)->text().toUtf8().data());
		if (landmark != nullptr)
		{
			ui->btnModifyLandmark->setEnabled(true);
			ui->btnRemoveLandmark->setEnabled(true);
			ui->checkBoxEnableLandmark->setChecked(landmark->enable);
			ui->spinBoxLandmarkRadius->setValue(landmark->radius);
		}
	}
	else
	{
		ui->btnModifyLandmark->setEnabled(false);
		ui->btnRemoveLandmark->setEnabled(false);
		ui->checkBoxEnableLandmark->setChecked(false);
		ui->spinBoxLandmarkRadius->setValue(0.0f);
	}
}
