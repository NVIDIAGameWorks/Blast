#ifndef GENERALPANEL_H
#define GENERALPANEL_H

#include <QtWidgets/QWidget>
#include "ProjectParams.h"

namespace Ui {
class GeneralPanel;
}

class GeneralPanel : public QWidget
{
    Q_OBJECT

public:
    explicit GeneralPanel(QWidget *parent = 0);
    ~GeneralPanel();
	void updateValues();

private slots:
    void on_comboBoxUserPreset_currentIndexChanged(int index);

    void on_btnAddUserPreset_clicked();

    void on_btnModifyUserPreset_clicked();

    void on_btnSaveUserPreset_clicked();

    void on_comboBoxSolverMode_currentIndexChanged(int index);

	void on_spinBoxLinearFactor_valueChanged(double arg1);

	void on_spinBoxAngularFactor_valueChanged(double arg1);

	void on_spinBoxMeanError_valueChanged(double arg1);

	void on_spinBoxVarianceError_valueChanged(double arg1);

    void on_spinBoxBondsPerFrame_valueChanged(int arg1);

    void on_spinBoxBondsIterations_valueChanged(int arg1);

private:
	BPPStressSolver* _getCurrentStressSolver();
	BPPStressSolver* _getUserPreset(const char* name);
	void _updateStressSolverUIs();

private:
    Ui::GeneralPanel *ui;
};

#endif // GENERALPANEL_H
