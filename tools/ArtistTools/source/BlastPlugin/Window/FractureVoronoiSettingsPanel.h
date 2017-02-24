#ifndef FRACTUREVORONOISETTINGSPANEL_H
#define FRACTUREVORONOISETTINGSPANEL_H

#include <QtWidgets/QWidget>

namespace Ui {
class FractureVoronoiSettingsPanel;
}

class FractureVoronoiSettingsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit FractureVoronoiSettingsPanel(QWidget *parent = 0);
    ~FractureVoronoiSettingsPanel();
	void updateValues();

private slots:
    void on_checkBoxGridPreview_stateChanged(int arg1);

    void on_checkBoxFracturePreview_stateChanged(int arg1);

    void on_checkBoxCutterMesh_stateChanged(int arg1);

    void on_spinBoxNumberOfSites_valueChanged(int arg1);

    void on_comboBoxSiteGeneration_currentIndexChanged(int index);

    void on_spinBoxGridSize_valueChanged(int arg1);

    void on_spinBoxGridScale_valueChanged(double arg1);

	void on_spinBoxAmplitude_valueChanged(double arg1);

    void on_spinBoxFrequency_valueChanged(int arg1);

    void on_comboBoxPaintMasks_currentIndexChanged(int index);

    void on_btnAddPaintMasks_clicked();

    void on_btnRemovePaintMasks_clicked();

    void on_comboBoxMeshCutter_currentIndexChanged(int index);

    void on_btnAddMeshCutter_clicked();

    void on_btnRemoveMeshCutter_clicked();

    void on_checkBoxFractureInsideCutter_stateChanged(int arg1);

    void on_checkBoxFractureOutsideCutter_stateChanged(int arg1);

    void on_btnAddTexture_clicked();

    void on_btnReloadTexture_clicked();

	void on_btnRemoveTexture_clicked();

	void on_spinBoxTextureSites_valueChanged(int arg1);

    void on_listWidgetTexture_itemSelectionChanged();

    void on_btnTextureMap_clicked();

    void on_btnApplyFracture_clicked();

private:
	QString _getTexturePathByName(const QString& name);
	void _updatePaintMaskComboBox();
	void _updateMeshCutterComboBox();
	void _updateTextureListWidget();

private:
    Ui::FractureVoronoiSettingsPanel	*ui;
	bool								_updateData;
};

#endif // FRACTUREVORONOISETTINGSPANEL_H
