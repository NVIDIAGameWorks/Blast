#ifndef FRACTURESLICESETTINGSPANEL_H
#define FRACTURESLICESETTINGSPANEL_H

#include <QtWidgets/QWidget>
#include "ProjectParams.h"

namespace Ui {
class FractureSliceSettingsPanel;
}
class FractureGeneralPanel;

class FractureSliceSettingsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit FractureSliceSettingsPanel(QWidget *parent = 0);
    ~FractureSliceSettingsPanel();
	void updateValues();
	void setFractureGeneralPanel(FractureGeneralPanel* generalPanel) { _generalPanel = generalPanel; }

private slots:
    void on_spinBoxNumSlicesX_valueChanged(int arg1);
	void on_spinBoxNumSlicesY_valueChanged(int arg1);
	void on_spinBoxNumSlicesZ_valueChanged(int arg1);

    void on_spinBoxOffsetVariation_valueChanged(double arg1);

	void on_spinBoxRotationVariation_valueChanged(double arg1);

    void on_spinBoxNoiseAmplitude_valueChanged(double arg1);

    void on_spinBoxNoiseFrequency_valueChanged(double arg1);

	void on_spinBoxNoiseOctaveNumber_valueChanged(int arg1);

    void on_spinBoxNoiseSeed_valueChanged(int arg1);

	void on_spinBoxSurfaceResolution_valueChanged(int arg1);

    void on_btnApplyFracture_clicked();

private:
	BPPSlice* _getBPPSlice();

private:
    Ui::FractureSliceSettingsPanel		*ui;
	bool								_updateData;
	FractureGeneralPanel*				_generalPanel;
};

#endif // FRACTURESLICESETTINGSPANEL_H
