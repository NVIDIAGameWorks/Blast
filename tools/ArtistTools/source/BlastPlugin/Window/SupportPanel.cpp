#include "SupportPanel.h"
#include "ui_SupportPanel.h"

SupportPanel::SupportPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SupportPanel)
{
    ui->setupUi(this);

	_selectedBonds.clear();
}

SupportPanel::~SupportPanel()
{
    delete ui;

	_selectedBonds.clear();
}

void SupportPanel::updateValues()
{
	if (_selectedBonds.size() > 0)
	{
		BPPBond* bond = _selectedBonds[0];

		ui->comboBoxHealthMask->clear();
		ui->comboBoxHealthMask->addItem(bond->name.buf);
		ui->spinBoxBondStrength->setValue(bond->support.bondStrength);
		ui->checkBoxEnableJoint->setChecked(bond->support.enableJoint);
	}
	else
	{
		ui->comboBoxHealthMask->clear();
		ui->comboBoxHealthMask->addItem("None");
		ui->spinBoxBondStrength->setValue(1.0f);
		ui->checkBoxEnableJoint->setChecked(false);
	}
}

void SupportPanel::dataSelected(std::vector<BlastNode*> selections)
{
	_selectedBonds.clear();

	for (BlastNode* node : selections)
	{
		if (eBond == node->getType())
		{
			BPPBond* bond = static_cast<BPPBond*>(node->getData());
			_selectedBonds.push_back(bond);
		}
	}

	updateValues();
}

void SupportPanel::on_comboBoxHealthMask_currentIndexChanged(int index)
{

}

void SupportPanel::on_btnAddHealthMask_clicked()
{

}

void SupportPanel::on_btnPen_clicked()
{

}

void SupportPanel::on_btnRemove_clicked()
{

}

void SupportPanel::on_spinBoxBondStrength_valueChanged(double arg1)
{
	for (size_t i = 0; i < _selectedBonds.size(); ++i)
	{
		_selectedBonds[i]->support.bondStrength = arg1;
	}
}

void SupportPanel::on_checkBoxEnableJoint_stateChanged(int arg1)
{
	for (size_t i = 0; i < _selectedBonds.size(); ++i)
	{
		_selectedBonds[i]->support.enableJoint = arg1;
	}
}
