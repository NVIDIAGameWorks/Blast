#ifndef FRACTURECUTOUTSETTINGSPANEL_H
#define FRACTURECUTOUTSETTINGSPANEL_H

#include <QtWidgets/QWidget>

namespace Ui {
class FractureCutoutSettingsPanel;
}

class FractureCutoutSettingsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit FractureCutoutSettingsPanel(QWidget *parent = 0);
    ~FractureCutoutSettingsPanel();
	void updateValues();

private slots:
    void on_btnAddTexture_clicked();

    void on_btnReloadTexture_clicked();

    void on_btnRemoveTexture_clicked();

    void on_listWidget_currentRowChanged(int currentRow);

    void on_btnTextureMap_clicked();

    void on_comboBoxCutoutType_currentIndexChanged(int index);

    void on_spinBoxPixelThreshold_valueChanged(int arg1);

    void on_checkBoxTiled_stateChanged(int arg1);

    void on_checkBoxInvertU_stateChanged(int arg1);

    void on_checkBoxInvertV_stateChanged(int arg1);

    void on_btnFitToObject_clicked();

    void on_btnApplyFracture_clicked();

private:
	QString _getTexturePathByName(const QString& name);
	void _updateTextureListWidget();

private:
    Ui::FractureCutoutSettingsPanel *ui;
};

#endif // FRACTURECUTOUTSETTINGSPANEL_H
