#include "GeneralPanel.h"
#include "ui_GeneralPanel.h"
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>

GeneralPanel::GeneralPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GeneralPanel)
{
    ui->setupUi(this);
}

GeneralPanel::~GeneralPanel()
{
    delete ui;
}

void GeneralPanel::updateValues()
{
	std::vector<StressSolverUserPreset> presets = BlastProject::ins().getUserPresets();
	ui->comboBoxUserPreset->clear();
	int countUserPresets = (int)presets.size();
	if (countUserPresets > 0)
	{
		QStringList userPresets;
		for (int i = 0; i < countUserPresets; ++i)
		{
			userPresets.append(presets[i].name.c_str());
		}
		ui->comboBoxUserPreset->addItems(userPresets);
	}

	std::vector<BPPAsset*> selectedAssets = BlastProject::ins().getSelectedBlastAssets();
	if (selectedAssets.size() > 0)
	{
		ui->comboBoxUserPreset->setCurrentText(selectedAssets[0]->activePreset.buf);

		_updateStressSolverUIs();
	}
	else
	{
		ui->comboBoxUserPreset->setCurrentIndex(-1);

	}
}

void GeneralPanel::on_comboBoxUserPreset_currentIndexChanged(int index)
{
	std::vector<BPPAsset*> assets = BlastProject::ins().getSelectedBlastAssets();
	for (size_t i = 0; i < assets.size(); ++i)
	{
		QByteArray tem = ui->comboBoxUserPreset->currentText().toUtf8();
		copy(assets[i]->activePreset, tem.data());

		BPPStressSolver* preset = _getUserPreset(tem.data());
		if (preset)
		{
			copy(assets[i]->stressSolver, *preset);
		}
	}
	_updateStressSolverUIs();
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
	if (ui->comboBoxUserPreset->currentIndex() == -1)
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
		int curIndex = ui->comboBoxUserPreset->currentIndex();

		std::vector<StressSolverUserPreset>& presets = BlastProject::ins().getUserPresets();
		presets[curIndex].name = newName.toUtf8().data();
		updateValues();
		ui->comboBoxUserPreset->setCurrentIndex(curIndex);
	}
	else if (ok && nameExist)
	{
		QMessageBox::warning(this, "Blast Tool", "The name you input is already exist!");
	}
	else if (ok && newName.isEmpty())
	{
		QMessageBox::warning(this, "Blast Tool", "You need input a name for the selected graphics material!");
	}
}

void GeneralPanel::on_btnSaveUserPreset_clicked()
{
	int currentUserPreset = ui->comboBoxUserPreset->currentIndex();
	if (-1 != currentUserPreset)
	{
		std::vector<StressSolverUserPreset>& presets = BlastProject::ins().getUserPresets();
		BPPStressSolver& stressSolver = presets[currentUserPreset].stressSolver;
		stressSolver.solverMode = ui->comboBoxSolverMode->currentIndex();
		stressSolver.linearFactor = ui->spinBoxLinearFactor->value();
		stressSolver.angularFactor = ui->spinBoxAngularFactor->value();
		stressSolver.meanError = ui->spinBoxMeanError->value();
		stressSolver.varianceError = ui->spinBoxVarianceError->value();
		stressSolver.bondsPerFrame = ui->spinBoxBondsPerFrame->value();
		stressSolver.bondsIterations = ui->spinBoxBondsIterations->value();
	}

	BlastProject::ins().saveUserPreset();
}

void GeneralPanel::on_comboBoxSolverMode_currentIndexChanged(int index)
{
	BPPStressSolver* stressSolver = _getCurrentStressSolver();

	if (stressSolver)
	{
		stressSolver->solverMode = index;
	}
}

void GeneralPanel::on_spinBoxLinearFactor_valueChanged(double arg1)
{
	BPPStressSolver* stressSolver = _getCurrentStressSolver();

	if (stressSolver)
	{
		stressSolver->linearFactor = arg1;
	}
	
}

void GeneralPanel::on_spinBoxAngularFactor_valueChanged(double arg1)
{
	BPPStressSolver* stressSolver = _getCurrentStressSolver();

	if (stressSolver)
	{
		stressSolver->angularFactor = arg1;
	}
}

void GeneralPanel::on_spinBoxMeanError_valueChanged(double arg1)
{
	BPPStressSolver* stressSolver = _getCurrentStressSolver();

	if (stressSolver)
	{
		stressSolver->meanError = arg1;
	}
}

void GeneralPanel::on_spinBoxVarianceError_valueChanged(double arg1)
{
	BPPStressSolver* stressSolver = _getCurrentStressSolver();

	if (stressSolver)
	{
		stressSolver->varianceError = arg1;
	}
}

void GeneralPanel::on_spinBoxBondsPerFrame_valueChanged(int arg1)
{
	BPPStressSolver* stressSolver = _getCurrentStressSolver();

	if (stressSolver)
	{
		stressSolver->bondsPerFrame = arg1;
	}
}

void GeneralPanel::on_spinBoxBondsIterations_valueChanged(int arg1)
{
	BPPStressSolver* stressSolver = _getCurrentStressSolver();

	if (stressSolver)
	{
		stressSolver->bondsIterations = arg1;
	}
}

BPPStressSolver* GeneralPanel::_getCurrentStressSolver()
{
	int currentUserPreset = ui->comboBoxUserPreset->currentIndex();

	if (-1 != currentUserPreset)
	{
		std::vector<StressSolverUserPreset>& presets = BlastProject::ins().getUserPresets();
		return &(presets[currentUserPreset].stressSolver);
	}
	else
	{
		std::vector<BPPAsset*> assets = BlastProject::ins().getSelectedBlastAssets();

		if (assets.size() > 0)
		{
			return &(assets[0]->stressSolver);
		}
	}

	return nullptr;
}

BPPStressSolver* GeneralPanel::_getUserPreset(const char* name)
{
	std::vector<StressSolverUserPreset>& presets = BlastProject::ins().getUserPresets();

	for (size_t i = 0; i < presets.size(); ++i)
	{
		if (presets[i].name == name)
			return &(presets[i].stressSolver);
	}

	return nullptr;
}

void GeneralPanel::_updateStressSolverUIs()
{
	BPPStressSolver* stressSolver = _getCurrentStressSolver();
	if (stressSolver != nullptr)
	{
		ui->comboBoxSolverMode->setCurrentIndex(stressSolver->solverMode);
		ui->spinBoxLinearFactor->setValue(stressSolver->linearFactor);
		ui->spinBoxAngularFactor->setValue(stressSolver->angularFactor);
		ui->spinBoxMeanError->setValue(stressSolver->meanError);
		ui->spinBoxVarianceError->setValue(stressSolver->varianceError);
		ui->spinBoxBondsPerFrame->setValue(stressSolver->bondsPerFrame);
		ui->spinBoxBondsIterations->setValue(stressSolver->bondsIterations);
	}
	else
	{
		BPPStressSolver noStressSolver;
		init(noStressSolver);
		
		ui->comboBoxSolverMode->setCurrentIndex(noStressSolver.solverMode);
		ui->spinBoxLinearFactor->setValue(noStressSolver.linearFactor);
		ui->spinBoxAngularFactor->setValue(noStressSolver.angularFactor);
		ui->spinBoxMeanError->setValue(noStressSolver.meanError);
		ui->spinBoxVarianceError->setValue(noStressSolver.varianceError);
		ui->spinBoxBondsPerFrame->setValue(noStressSolver.bondsPerFrame);
		ui->spinBoxBondsIterations->setValue(noStressSolver.bondsIterations);
	}

}
