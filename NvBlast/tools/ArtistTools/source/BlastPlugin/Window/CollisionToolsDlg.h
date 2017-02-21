#ifndef COLLISIONTOOLSDLG_H
#define COLLISIONTOOLSDLG_H

#include <QtWidgets/QDialog>

namespace Ui {
class CollisionToolsDlg;
}

class CollisionToolsDlg : public QDialog
{
    Q_OBJECT

public:
    explicit CollisionToolsDlg(QWidget *parent = 0);
    ~CollisionToolsDlg();

private slots:
    void on_comboBoxApplyFilter_currentIndexChanged(int index);

    void on_comboBoxCollisionShape_currentIndexChanged(int index);

    void on_spinBoxQuality_valueChanged(int arg1);

    void on_spinBoxMaxHulls_valueChanged(int arg1);

    void on_spinBoxTrimHulls_valueChanged(int arg1);

    void on_comboBoxTargetPlatform_currentIndexChanged(int index);

    void on_btnReset_clicked();

    void on_btnApply_clicked();

private:
    Ui::CollisionToolsDlg *ui;
};

#endif // COLLISIONTOOLSDLG_H
