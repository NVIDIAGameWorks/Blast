#ifndef FRACTUREGENERALPANEL_H
#define FRACTUREGENERALPANEL_H

#include <QtWidgets/QWidget>

namespace Ui {
class FractureGeneralPanel;
}

class FractureGeneralPanel : public QWidget
{
    Q_OBJECT

public:
    explicit FractureGeneralPanel(QWidget *parent = 0);
    ~FractureGeneralPanel();
	void updateValues();

private slots:
    void on_comboBoxFracturePreset_currentIndexChanged(int index);

    void on_comboBoxFractureType_currentIndexChanged(int index);

    void on_checkBoxAddDepth_stateChanged(int arg1);

    void on_checkBoxPerChunk_stateChanged(int arg1);

    void on_checkBoxNewMatID_stateChanged(int arg1);

    void on_comboBoxApplyMaterial_currentIndexChanged(int index);

private:
    Ui::FractureGeneralPanel *ui;
};

#endif // FRACTUREGENERALPANEL_H
