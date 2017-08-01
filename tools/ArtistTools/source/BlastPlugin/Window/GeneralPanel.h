#ifndef GENERALPANEL_H
#define GENERALPANEL_H

#include <QtWidgets/QWidget>
#include "ProjectParams.h"
#include "BlastSceneTree.h"

namespace Ui {
class GeneralPanel;
}

class GeneralPanel : public QWidget, public ISceneObserver
{
    Q_OBJECT

public:
    explicit GeneralPanel(QWidget *parent = 0);
    ~GeneralPanel();
	void updateValues();

	virtual void dataSelected(std::vector<BlastNode*> selections);

private slots:
    void on_comboBoxUserPreset_currentIndexChanged(int index);

    void on_btnAddUserPreset_clicked();

    void on_btnModifyUserPreset_clicked();

    void on_btnSaveUserPreset_clicked();

	void on_spinBoxMaterialHardness_valueChanged(double arg1);

	void on_spinBoxLinearFactor_valueChanged(double arg1);

	void on_spinBoxAngularFactor_valueChanged(double arg1);

	void on_spinBoxBondIterations_valueChanged(int arg1);

	void on_spinBoxGraphReductionLevel_valueChanged(int arg1);

private:
	BPPStressSolver* _getCurrentUserPresetStressSolver();
	void _updateStressSolverUIs();
	void _updateStressSolverToBlast();

private:
    Ui::GeneralPanel		*ui;
	bool					_updateData;
	std::vector<BPPAsset*>	_selectedAssets;
};

#endif // GENERALPANEL_H
