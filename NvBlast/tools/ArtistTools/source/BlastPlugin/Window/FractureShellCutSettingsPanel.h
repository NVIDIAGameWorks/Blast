#ifndef FRACTURESHELLCUTSETTINGSPANEL_H
#define FRACTURESHELLCUTSETTINGSPANEL_H

#include <QtWidgets/QWidget>

namespace Ui {
class FractureShellCutSettingsPanel;
}

class FractureShellCutSettingsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit FractureShellCutSettingsPanel(QWidget *parent = 0);
    ~FractureShellCutSettingsPanel();
	void updateValues();

private slots:
void on_spinBoxThickness_valueChanged(double arg1);

    void on_spinBoxThicknessVariation_valueChanged(double arg1);

    void on_btnApplyFracture_clicked();

private:
    Ui::FractureShellCutSettingsPanel *ui;
};

#endif // FRACTURESHELLCUTSETTINGSPANEL_H
