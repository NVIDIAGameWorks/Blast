#ifndef DEFAULTDAMAGEPANEL_H
#define DEFAULTDAMAGEPANEL_H

#include <QtWidgets/QWidget>
#include <QtWidgets/QComboBox>
#include "BlastSceneTree.h"

namespace Ui {
class DefaultDamagePanel;
}

class DefaultDamagePanel : public QWidget, public ISceneObserver
{
    Q_OBJECT

public:
    explicit DefaultDamagePanel(QWidget *parent = 0);
    ~DefaultDamagePanel();
	void updateValues();

	static DefaultDamagePanel* ins();
	QComboBox* getDamageProfile();
	void setUpdateData(bool bUpdateData);

	virtual void dataSelected(std::vector<BlastNode*> selections);

private slots:
    void on_spinBoxDamageAmount_valueChanged(double arg1);

    void on_spinBoxExplosiveImpulse_valueChanged(double arg1);

	void on_spinBoxDamageRadius_valueChanged(double arg1);

	void on_spinBoxStressDamageForce_valueChanged(double arg1);

    void on_comboBoxDamageProfile_currentIndexChanged(int index);

	void on_checkBoxDamageContinuously_stateChanged(int arg1);

private:
    Ui::DefaultDamagePanel		*ui;
	bool						_updateData;
	std::vector<BPPAsset*>		_selectedAssets;
};

#endif // DEFAULTDAMAGEPANEL_H
