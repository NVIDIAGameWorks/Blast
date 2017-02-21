#ifndef NOISETOOLSDLG_H
#define NOISETOOLSDLG_H

#include <QtWidgets/QDialog>

namespace Ui {
class NoiseToolsDlg;
}

class NoiseToolsDlg : public QDialog
{
    Q_OBJECT

public:
    explicit NoiseToolsDlg(QWidget *parent = 0);
    ~NoiseToolsDlg();

private slots:
    void on_comboBoxApplyByMaterial_currentIndexChanged(int index);

    void on_spinBoxAmplitude_valueChanged(double arg1);

    void on_spinBoxFrequency_valueChanged(double arg1);

    void on_spinBoxAmplitude_3_valueChanged(double arg1);

    void on_btnApply_clicked();

private:
    Ui::NoiseToolsDlg *ui;
};

#endif // NOISETOOLSDLG_H
