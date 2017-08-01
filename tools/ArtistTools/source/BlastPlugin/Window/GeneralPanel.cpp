#include "GeneralPanel.h"
#include "ui_GeneralPanel.h"
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include "SampleManager.h"

GeneralPanel::GeneralPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GeneralPanel),
	_updateData(true)
{
    ui->setupUi(this);
}

GeneralPanel::~GeneralPanel()
{
    delete ui;
}

void GeneralPanel::updateValues()
{
	_updateData = false;
	ui->comboBoxUserPreset->clear();
	QStringList userPresets;
	userPresets.append("Default");
	std::vector<StressSolverUserPreset> presets = BlastProject::ins().getUserPresets();
	int countUserPresets = (int)presets.size();
	if (countUserPresets > 0)
	{
		for (int i = 0; i < countUserPresets; ++i)
		{
			userPresets.append(presets[i].name.c_str());
		}
	}
	ui->comboBoxUserPreset->addItems(userPresets);

	if (_selectedAssets.size() > 0)
	{
		if (nullptr == _selectedAssets[0]->activeUserPreset.buf
			|| 0 == strlen(_selectedAssets[0]->activeUserPreset.buf)
			|| !BlastProject::ins().isUserPresetNameExist(_selectedAssets[0]->activeUserPreset.buf))
		{
			ui->comboBoxUserPreset->setCurrentIndex(0);
		}
		else
		{
			ui->comboBoxUserPreset->setCurrentText(_selectedAssets[0]->activeUserPreset.buf);
		}

		_updateStressSolverUIs();
	}
	else
	{
		ui->comboBoxUserPreset->setCurrentIndex(0);
	}
	_updateData = true;
}

void GeneralPanel::dataSelected(std::vector<BlastNode*> selections)
{
	_selectedAssets.clear();

	for (BlastNode* node : selections)
	{
		if (eAsset == node->getType())
		{
			BPPAsset* asset = static_cast<BPPAsset*>(node->getData());
			_selectedAssets.push_back(asset);
		}
	}

	updateValues();
}

void GeneralPanel::on_comboBoxUserPreset_currentIndexChanged(int index)
{
	if (!_updateData)
		return;

	for (size_t i = 0; i < _selectedAssets.size(); ++i)
	{
		QByteArray tem = ui->comboBoxUserPreset->currentText().toUtf8();
		copy(_selectedAssets[i]->activeUserPreset, tem.data());
	}
	_updateStressSolverUIs();
	_updateStressSolverToBlast();
}

void GeneralPanel::on_btnAddUserPreset_clicked()
{
	bool ok = false;
	QString name = QInputDialog::getText(this,
		tr("Blast Tool"),
		tr("Please input name for new User preset:"),
		QLineEdit::Normal,
		"",
		&ok);
	bool nameExist = BlastProject::ins().isUserPresetNameExist(name.toUtf8().data());
	if (ok && !name.isEmpty() && !nameExist)
	{
		BlastProject::ins().addUserPreset(name.toUtf8().data());
		updateValues();
		ui->comboBoxUserPreset->setCurrentIndex(ui->comboBoxUserPreset->count() - 1);
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

void GeneralPanel::on_btnModifyUserPreset_clicked()
{
	if (ui->comboBoxUserPreset->currentIndex() < 1)
	{
		QMessageBox::warning(this, "Blast Tool", "You should select an user preset!");
		return;
	}

	QByteArray tmp = ui->comboBoxUserPreset->currentText().toUtf8();
	const char* oldName = tmp.data();

	bool ok = false;
	QString newName = QInputDialog::getText(this,
		tr("Blast Tool"),
		tr("Please input new name for the selected user preset:"),
		QLineEdit::Normal,
		oldName,
		&ok);
	bool nameExist = BlastProject::ins().isUserPresetNameExist(newName.toUtf8().data());
	if (ok && !newName.isEmpty() && !nameExist)
	{
		int curIndex = ui->comboBoxUserPreset->currentIndex() - 1;
		if (curIndex >= 0)
		{
			std::vector<StressSolverUserPreset>& presets = BlastProject::ins().getUserPresets();
			presets[curIndex].name = newName.toUtf8().data();
			updateValues();
			ui->comboBoxUserPreset->setCurrentIndex(curIndex + 1);
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

void GeneralPanel::on_btnSaveUserPreset_clicked()
{
	BlastProject::ins().saveUserPreset();
}

void GeneralPanel::on_spinBoxMaterialHardness_valueChanged(double arg1)
{
	if (!_updateData)
		return;

	BPPStressSolver* stressSolver = _getCurrentUserPresetStressSolver();

	if (stressSolver)
	{
		stressSolver->hardness = arg1;
	}
	else
	{
		for (BPPAsset* asset : _selectedAssets)
		{
			asset->stressSolver.hardness = arg1;
		}
	}

	_updateStressSolverToBlast();
}

void GeneralPanel::on_spinBoxLinearFactor_valueChanged(double arg1)
{
	if (!_updateData)
		return;

	BPPStressSolver* stressSolver = _getCurrentUserPresetStressSolver();

	if (stressSolver)
	{
		stressSolver->linearFactor = arg1;
	}
	else
	{
		for (BPPAsset* asset : _selectedAssets)
		{
			asset->stressSolver.linearFactor = arg1;
		}
	}

	_updateStressSolverToBlast();
}

void GeneralPanel::on_spinBoxAngularFactor_valueChanged(double arg1)
{
	if (!_updateData)
		return;

	BPPStressSolver* stressSolver = _getCurrentUserPresetStressSolver();

	if (stressSolver)
	{
		stressSolver->angularFactor = arg1;
	}
	else
	{
		for (BPPAsset* asset : _selectedAssets)
		{
			asset->stressSolver.angularFactor = arg1;
		}
	}

	_updateStressSolverToBlast();
}

void GeneralPanel::on_spinBoxBondIterations_valueChanged(int arg1)
{
	if (!_updateData)
		return;

	BPPStressSolver* stressSolver = _getCurrentUserPresetStressSolver();

	if (stressSolver)
	{
		stressSolver->bondIterationsPerFrame = arg1;
	}
	else
	{
		for (BPPAsset* asset : _selectedAssets)
		{
			asset->stressSolver.bondIterationsPerFrame = arg1;
		}
	}

	_updateStressSolverToBlast();
}

void GeneralPanel::on_spinBoxGraphReductionLevel_valueChanged(int arg1)
{
	if (!_updateData)
		return;

	BPPStressSolver* stressSolver = _getCurrentUserPresetStressSolver();

	if (stressSolver)
	{
		stressSolver->graphReductionLevel = arg1;
	}
	else
	{
		for (BPPAsset* asset : _selectedAssets)
		{
			asset->stressSolver.graphReductionLevel = arg1;
		}
	}

	_updateStressSolverToBlast();
}

BPPStressSolver* GeneralPanel::_getCurrentUserPresetStressSolver()
{
	int currentUserPreset = ui->comboBoxUserPreset->currentIndex();

	if (0 < currentUserPreset)
	{
		std::vector<StressSolverUserPreset>& presets = BlastProject::ins().getUserPresets();
		return &(presets[currentUserPreset - 1].stressSolver);
	}

	return nullptr;
}

void GeneralPanel::_updateStressSolverUIs()
{
	BPPStressSolver noStressSolver;
	init(noStressSolver);
	BPPStressSolver* stressSolver = _getCurrentUserPresetStressSolver();
	if (stressSolver == nullptr)
	{
		if (0 < _selectedAssets.size())
		{
			copy(noStressSolver, _selectedAssets[0]->stressSolver);
		}
		stressSolver = &noStressSolver;
	}

	_updateData = false;
	ui->spinBoxMaterialHardness->setValue(stressSolver->hardness);
	ui->spinBoxLinearFactor->setValue(stressSolver->linearFactor);
	ui->spinBoxAngularFactor->setValue(stressSolver->angularFactor);
	ui->spinBoxBondIterations->setValue(stressSolver->bondIterationsPerFrame);
	ui->spinBoxGraphReductionLevel->setValue(stressSolver->graphReductionLevel);
	_updateData = true;
}

void GeneralPanel::_updateStressSolverToBlast()
{
	BPPStressSolver* stressSolver = _getCurrentUserPresetStressSolver();

	for (BPPAsset* asset : _selectedAssets)
	{
		SampleManager::ins()->updateAssetFamilyStressSolver(asset, nullptr != stressSolver? *stressSolver : asset->stressSolver);
	}
}
