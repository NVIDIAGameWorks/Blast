#include "DefaultDamagePanel.h"
#include "ui_DefaultDamagePanel.h"
#include "ProjectParams.h"
#include "BlastSceneTree.h"
#include <float.h>

#include "DamageToolController.h"

DefaultDamagePanel* gDefaultDamagePanel = nullptr;
DefaultDamagePanel* DefaultDamagePanel::ins()
{
	return gDefaultDamagePanel;
}

QComboBox* DefaultDamagePanel::getDamageProfile()
{
	return ui->comboBoxDamageProfile;
}

void DefaultDamagePanel::setUpdateData(bool bUpdateData)
{
	_updateData = bUpdateData;
}

DefaultDamagePanel::DefaultDamagePanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DefaultDamagePanel),
	_updateData(true)
{
	gDefaultDamagePanel = this;

    ui->setupUi(this);

	_updateData = false;
	ui->spinBoxDamageAmount->setRange(0.0f, DBL_MAX);
	ui->spinBoxExplosiveImpulse->setRange(0.0f, DBL_MAX);
	ui->spinBoxDamageRadius->setRange(0.0f, DBL_MAX);
	ui->spinBoxStressDamageForce->setRange(0.0f, DBL_MAX);
	ui->checkBoxDamageContinuously->setChecked(false);
	_updateData = true;
}

DefaultDamagePanel::~DefaultDamagePanel()
{
    delete ui;
}

void DefaultDamagePanel::updateValues()
{
	_updateData = false;
	BPPDefaultDamage& damage = BlastProject::ins().getParams().defaultDamage;
	if (damage.damageStructs.arraySizes[0] > 0)
	{		
		uint32_t damageProfile = damage.damageProfile;
		BPPDamageStruct& damageStruct = damage.damageStructs.buf[damageProfile];
		ui->spinBoxDamageAmount->setValue(damage.damageAmount);
		ui->spinBoxExplosiveImpulse->setValue(damage.explosiveImpulse);
		ui->spinBoxStressDamageForce->setValue(damage.stressDamageForce);
		ui->comboBoxDamageProfile->setCurrentIndex(damageProfile);
		ui->spinBoxDamageRadius->setValue(damageStruct.damageRadius);
		ui->checkBoxDamageContinuously->setChecked(damageStruct.continuously);
	}
	else
	{
		ui->spinBoxDamageAmount->setValue(0);
		ui->spinBoxExplosiveImpulse->setValue(0);
		ui->spinBoxDamageRadius->setValue(0);
		ui->spinBoxStressDamageForce->setValue(0);
		ui->checkBoxDamageContinuously->setChecked(false);
		ui->comboBoxDamageProfile->setCurrentIndex(0);
	}
	_updateData = true;
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

void DefaultDamagePanel::on_spinBoxDamageAmount_valueChanged(double arg1)
{
	if (!_updateData)
		return;

	BPPDefaultDamage& damage = BlastProject::ins().getParams().defaultDamage;
	damage.damageAmount = arg1;

	DamageToolController::ins()->setDamageAmount(arg1);
}

void DefaultDamagePanel::on_spinBoxExplosiveImpulse_valueChanged(double arg1)
{
	if (!_updateData)
		return;

	BPPDefaultDamage& damage = BlastProject::ins().getParams().defaultDamage;
	damage.explosiveImpulse = arg1;

	DamageToolController::ins()->setExplosiveImpulse(arg1);
}

void DefaultDamagePanel::on_spinBoxDamageRadius_valueChanged(double arg1)
{
	if (!_updateData)
		return;

	BPPDefaultDamage& damage = BlastProject::ins().getParams().defaultDamage;
	uint32_t damageProfile = damage.damageProfile;
	BPPDamageStruct& damageStruct = damage.damageStructs.buf[damageProfile];
	damageStruct.damageRadius = arg1;

	DamageToolController::ins()->setRadius(arg1);
}

void DefaultDamagePanel::on_spinBoxStressDamageForce_valueChanged(double arg1)
{
	if (!_updateData)
		return;

	BPPDefaultDamage& damage = BlastProject::ins().getParams().defaultDamage;
	damage.stressDamageForce = arg1;

	DamageToolController::ins()->setStressForceFactor(arg1);
}

void DefaultDamagePanel::on_comboBoxDamageProfile_currentIndexChanged(int index)
{
	if (!_updateData)
		return;

	BPPDefaultDamage& damage = BlastProject::ins().getParams().defaultDamage;
	damage.damageProfile = index;

	DamageToolController::ins()->setDamagerIndex(index);

	updateValues();
}

void DefaultDamagePanel::on_checkBoxDamageContinuously_stateChanged(int arg1)
{
	if (!_updateData)
		return;

	BPPDefaultDamage& damage = BlastProject::ins().getParams().defaultDamage;
	uint32_t damageProfile = damage.damageProfile;
	BPPDamageStruct& damageStruct = damage.damageStructs.buf[damageProfile];
	damageStruct.continuously = arg1;

	DamageToolController::ins()->setDamageWhilePressed(arg1);
}
