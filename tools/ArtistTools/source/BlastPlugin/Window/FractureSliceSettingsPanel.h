#ifndef FRACTURESLICESETTINGSPANEL_H
#define FRACTURESLICESETTINGSPANEL_H

#include <QtWidgets/QWidget>

namespace Ui {
class FractureSliceSettingsPanel;
}

class FractureSliceSettingsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit FractureSliceSettingsPanel(QWidget *parent = 0);
    ~FractureSliceSettingsPanel();
	void updateValues();

private slots:
    void on_spinBoxNumSlices_valueChanged(int arg1);

    void on_spinBoxOffsetVariation_valueChanged(double arg1);

	void on_spinBoxRotationVariation_valueChanged(double arg1);

    void on_spinBoxNoiseAmplitude_valueChanged(double arg1);

    void on_spinBoxNoiseFrequency_valueChanged(double arg1);

    void on_spinBoxNoiseSeed_valueChanged(int arg1);

    void on_btnApplyFracture_clicked();

private:
    Ui::FractureSliceSettingsPanel *ui;
};

#endif // FRACTURESLICESETTINGSPANEL_H
