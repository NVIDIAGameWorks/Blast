#include "FractureGeneralPanel.h"
#include "ui_FractureGeneralPanel.h"
#include "ProjectParams.h"
#include "BlastPlugin.h"
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include "FractureVoronoiSettingsPanel.h"
#include "FractureSliceSettingsPanel.h"
#include "FractureVisualizersPanel.h"
#include "ExpandablePanel.h"
#include "SampleManager.h"

FractureGeneralPanel* pFractureGeneralPanel = nullptr;
FractureGeneralPanel* FractureGeneralPanel::ins()
{
	return pFractureGeneralPanel;
}

FractureGeneralPanel::FractureGeneralPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FractureGeneralPanel),
	_updateData(true),
	_voronoiPanel(nullptr),
	_slicePanel(nullptr),
	_visualizersPanel(nullptr),
	_fractureVoronoiSettingsExpandlePanel(nullptr),
	_fractureSliceSettingsExpandlePanel(nullptr)
{
    ui->setupUi(this);
	pFractureGeneralPanel = this;
	m_bValid = true;

	QStringList types;
	types << "Voronoi" << "Slice";
	_updateData = false;
	ui->comboBoxFractureType->addItems(types);
	ui->comboBoxFractureType->setCurrentIndex(0);
	_updateData = true;
}

FractureGeneralPanel::~FractureGeneralPanel()
{
    delete ui;
}

void FractureGeneralPanel::updateValues()
{
	_updateData = false;
	m_bValid = false;
	ui->comboBoxApplyMaterial->clear();
	m_bValid = true;

	QStringList materialNames;
	materialNames.append("None");
	BPParams& projectParams = BlastProject::ins().getParams();
	BPPGraphicsMaterialArray& theArray = projectParams.graphicsMaterials;
	int count = theArray.arraySizes[0];
	for (int i = 0; i < count; ++i)
	{
		BPPGraphicsMaterial& item = theArray.buf[i];
		materialNames.append(item.name.buf);
	}

	m_bValid = false;
	ui->comboBoxApplyMaterial->insertItems(0, materialNames);
	m_bValid = true;

	BPPFractureGeneral& fractureGeneral = BlastProject::ins().getParams().fracture.general;

	std::vector<FracturePreset> presets = BlastProject::ins().getFracturePresets();
	ui->comboBoxFracturePreset->clear();
	QStringList presetNames;
	presetNames.append("Default");
	int countPresets = (int)presets.size();
	if (countPresets > 0)
	{
		for (int i = 0; i < countPresets; ++i)
		{
			presetNames.append(presets[i].name.c_str());
		}
	}
	ui->comboBoxFracturePreset->addItems(presetNames);

	if (nullptr == fractureGeneral.fracturePreset.buf
		|| 0 == strlen(fractureGeneral.fracturePreset.buf)
		|| !(BlastProject::ins().isFracturePresetNameExist(fractureGeneral.fracturePreset.buf)))
	{
		ui->comboBoxFracturePreset->setCurrentIndex(0);
	}
	else
	{
		ui->comboBoxFracturePreset->setCurrentText(fractureGeneral.fracturePreset.buf);
	}
	_updateFractureUIs();
	ui->comboBoxApplyMaterial->setCurrentIndex(fractureGeneral.applyMaterial);

	bool checked = BlastProject::ins().getParams().fracture.general.autoSelectNewChunks;
	ui->checkBoxAutoSelectNewChunks->setChecked(checked);
	_updateData = true;
}

void FractureGeneralPanel::setFracturePanels(FractureVoronoiSettingsPanel* voronoiPanel, FractureSliceSettingsPanel* slicePanel, FractureVisualizersPanel* visulizersPanel)
{
	_voronoiPanel = voronoiPanel;
	_slicePanel = slicePanel;
	_visualizersPanel = visulizersPanel;
}

void FractureGeneralPanel::setFractureExpandablePanels(ExpandablePanel* voronoiPanel, ExpandablePanel* slicePanel)
{
	_fractureVoronoiSettingsExpandlePanel = voronoiPanel;
	_fractureSliceSettingsExpandlePanel = slicePanel;
}

void FractureGeneralPanel::on_comboBoxFracturePreset_currentIndexChanged(int index)
{
	if (!_updateData)
		return;

	BPPFractureGeneral& fractureGeneral = BlastProject::ins().getParams().fracture.general;
	copy(fractureGeneral.fracturePreset, ui->comboBoxFracturePreset->currentText().toStdString().c_str());
	_updateFractureUIs();
}


void FractureGeneralPanel::on_btnAddPreset_clicked()
{
	bool ok = false;
	QString name = QInputDialog::getText(this,
		tr("Blast Tool"),
		tr("Please input name for new fracture preset:"),
		QLineEdit::Normal,
		"",
		&ok);
	bool nameExist = BlastProject::ins().isFracturePresetNameExist(name.toUtf8().data());
	if (ok && !name.isEmpty() && !nameExist)
	{
		BlastProject::ins().addFracturePreset(name.toUtf8().data(), (FractureType)ui->comboBoxFractureType->currentIndex());
		updateValues();
		ui->comboBoxFracturePreset->setCurrentIndex(ui->comboBoxFracturePreset->count() - 1);
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

void FractureGeneralPanel::on_btnModifyPreset_clicked()
{
	if (ui->comboBoxFracturePreset->currentIndex() < 1)
	{
		QMessageBox::warning(this, "Blast Tool", "You should select an fracture preset!");
		return;
	}

	QByteArray tmp = ui->comboBoxFracturePreset->currentText().toUtf8();
	const char* oldName = tmp.data();

	bool ok = false;
	QString newName = QInputDialog::getText(this,
		tr("Blast Tool"),
		tr("Please input new name for the selected fracture preset:"),
		QLineEdit::Normal,
		oldName,
		&ok);
	bool nameExist = BlastProject::ins().isUserPresetNameExist(newName.toUtf8().data());
	if (ok && !newName.isEmpty() && !nameExist)
	{
		int curIndex = ui->comboBoxFracturePreset->currentIndex() - 1;
		if(curIndex >= 0)
		{
			std::vector<FracturePreset>& presets = BlastProject::ins().getFracturePresets();
			presets[curIndex].name = newName.toUtf8().data();
			updateValues();
			ui->comboBoxFracturePreset->setCurrentIndex(curIndex + 1);
		}
	}
	else if (ok && nameExist)
	{
		QMessageBox::warning(this, "Blast Tool", "The name you input is already exist!");
	}
	else if (ok && newName.isEmpty())
	{
		QMessageBox::warning(this, "Blast Tool", "You need input a name for the selected preset!");
	}
}

void FractureGeneralPanel::on_btnSavePreset_clicked()
{
	BlastProject::ins().saveFracturePreset();
}

void FractureGeneralPanel::on_comboBoxFractureType_currentIndexChanged(int index)
{
	if (!_updateData)
		return;

	BPPFractureGeneral& fractureGeneral = BlastProject::ins().getParams().fracture.general;
	fractureGeneral.fractureType = index;

	FracturePreset* fracturePreset = getCurrentFracturePreset();
	if (fracturePreset != nullptr)
	{
		fracturePreset->setType((FractureType)index);
	}
	_showFracturePanel(ui->comboBoxFractureType->currentText());
}

void FractureGeneralPanel::on_comboBoxApplyMaterial_currentIndexChanged(int index)
{
	if (!m_bValid)
		return;

	if (!_updateData)
		return;

	BPPFractureGeneral& fractureGeneral = BlastProject::ins().getParams().fracture.general;
	fractureGeneral.applyMaterial = index;
}

void FractureGeneralPanel::on_checkBoxAutoSelectNewChunks_stateChanged(int arg1)
{
	BlastProject::ins().getParams().fracture.general.autoSelectNewChunks = arg1;
}

FracturePreset* FractureGeneralPanel::getCurrentFracturePreset()
{
	int currentPreset = ui->comboBoxFracturePreset->currentIndex();

	if (0 < currentPreset)
	{
		std::vector<FracturePreset>& presets = BlastProject::ins().getFracturePresets();
		if (presets.size() > 0)
		{
			return &(presets[currentPreset - 1]);
		}
	}

	return nullptr;
}

FracturePreset* FractureGeneralPanel::_getFracturePreset(const char* name)
{
	std::vector<FracturePreset>& presets = BlastProject::ins().getFracturePresets();

	for (size_t i = 0; i < presets.size(); ++i)
	{
		if (presets[i].name == name)
			return &(presets[i]);
	}

	return nullptr;
}

void FractureGeneralPanel::_updateFractureUIs()
{
	_updateData = false;

	FracturePreset* fracturePreset = getCurrentFracturePreset();
	if (fracturePreset != nullptr)
	{
		if (eFractureType_Voronoi == fracturePreset->type)
		{
			ui->comboBoxFractureType->setCurrentIndex(0);
		}
		else if (eFractureType_Slice == fracturePreset->type)
		{
			ui->comboBoxFractureType->setCurrentIndex(1);
		}
	}
	else
	{
		BPPFracture& fracture = BlastProject::ins().getParams().fracture;
		ui->comboBoxFractureType->setCurrentIndex(fracture.general.fractureType);
	}

	_showFracturePanel(ui->comboBoxFractureType->currentText());
	_visualizersPanel->updateValues();
	_updateData = true;
}

//////////////////////////////////////////////////////////////////////////////
void FractureGeneralPanel::_showFracturePanel(const QString& fractireType)
{
	_fractureVoronoiSettingsExpandlePanel->hide();
	_fractureSliceSettingsExpandlePanel->hide();
	QString fractireTypeLower = fractireType.toLower();
	if (fractireTypeLower == "voronoi")
	{
		_voronoiPanel->updateValues();
		_fractureVoronoiSettingsExpandlePanel->show();
	}
	else if (fractireTypeLower == "slice")
	{
		_slicePanel->updateValues();
		_fractureSliceSettingsExpandlePanel->show();
	}
}
