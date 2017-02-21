#include "DefaultDamagePanel.h"
#include "ui_DefaultDamagePanel.h"
#include "ProjectParams.h"
#include "BlastSceneTree.h"

DefaultDamagePanel::DefaultDamagePanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DefaultDamagePanel)
{
    ui->setupUi(this);
}

DefaultDamagePanel::~DefaultDamagePanel()
{
    delete ui;
}

void DefaultDamagePanel::updateValues()
{
	if (_selectedAssets.size() > 0)
	{
		BPPDefaultDamage& damage = _selectedAssets[0]->defaultDamage;

		ui->spinBoxMinRadius->setValue(damage.minRadius);
		ui->spinBoxMaxRadius->setValue(damage.maxRadius);
		ui->comboBoxFallOff->setCurrentIndex(damage.FallOff);
		ui->spinBoxMaxChunkSpeed->setValue(damage.maxChunkSpeed);
	}
	else
	{
		ui->spinBoxMinRadius->setValue(0.0f);
		ui->spinBoxMaxRadius->setValue(0.0f);
		ui->checkBoxMaxRadius->setChecked(false);
		ui->comboBoxFallOff->setCurrentIndex(-1);
		ui->spinBoxMaxChunkSpeed->setValue(0.0f);
	}
}

void DefaultDamagePanel::dataSelected(std::vector<BlastNode*> selections)
{
	_selectedAssets.clear();

	for (BlastNode* node: selections)
	{
		if (eAsset == node->getType())
		{
			BPPAsset* asset = static_cast<BPPAsset*>(node->getData());
			_selectedAssets.push_back(asset);
		}
	}

	updateValues();
}

void DefaultDamagePanel::on_spinBoxMinRadius_valueChanged(double arg1)
{
	for (size_t i = 0; i < _selectedAssets.size(); ++i)
	{
		BPPDefaultDamage& damage = _selectedAssets[i]->defaultDamage;
		damage.minRadius = arg1;
	}
}

void DefaultDamagePanel::on_spinBoxMaxRadius_valueChanged(double arg1)
{
	for (size_t i = 0; i < _selectedAssets.size(); ++i)
	{
		BPPDefaultDamage& damage = _selectedAssets[i]->defaultDamage;

		damage.maxRadius = arg1;

		if (arg1 < damage.minRadius)
		{
			damage.maxRadius = damage.minRadius;
		}
	}
}

void DefaultDamagePanel::on_checkBoxMaxRadius_stateChanged(int arg1)
{
	for (size_t i = 0; i < _selectedAssets.size(); ++i)
	{
		BPPDefaultDamage& damage = _selectedAssets[i]->defaultDamage;
		damage.maxRadiusEnable = (arg1 != 0 ? true: false);
	}
}

void DefaultDamagePanel::on_comboBoxFallOff_currentIndexChanged(int index)
{
	for (size_t i = 0; i < _selectedAssets.size(); ++i)
	{
		BPPDefaultDamage& damage = _selectedAssets[i]->defaultDamage;
		damage.FallOff = index;
	}
}

void DefaultDamagePanel::on_spinBoxMaxChunkSpeed_valueChanged(double arg1)
{
	for (size_t i = 0; i < _selectedAssets.size(); ++i)
	{
		BPPDefaultDamage& damage = _selectedAssets[i]->defaultDamage;
		damage.maxChunkSpeed = arg1;
	}
}
