#ifndef FRACTUREVORONOISETTINGSPANEL_H
#define FRACTUREVORONOISETTINGSPANEL_H

#include <QtWidgets/QWidget>
#include "ProjectParams.h"

namespace Ui {
class FractureVoronoiSettingsPanel;
}
class FractureGeneralPanel;

class FractureVoronoiSettingsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit FractureVoronoiSettingsPanel(QWidget *parent = 0);
    ~FractureVoronoiSettingsPanel();
	void updateValues();
	void setFractureGeneralPanel(FractureGeneralPanel* generalPanel) { _generalPanel = generalPanel; }

private slots:
    void on_checkBoxGridPreview_stateChanged(int arg1);

    void on_checkBoxFracturePreview_stateChanged(int arg1);

    void on_checkBoxCutterMesh_stateChanged(int arg1);

	void on_comboBoxSiteGeneration_currentIndexChanged(int index);

    void on_spinBoxNumberOfSites_valueChanged(int arg1);

    void on_spinBoxNumberOfClusters_valueChanged(int arg1);

    void on_spinBoxSitesPerCluster_valueChanged(int arg1);

	void on_spinBoxClusterRadius_valueChanged(double arg1);

    void on_btnApplyFracture_clicked();

private:
	BPPVoronoi* _getBPPVoronoi();
	void _showCurrentSiteGenerationUI();

private:
    Ui::FractureVoronoiSettingsPanel	*ui;
	bool								_updateData;
	FractureGeneralPanel*				_generalPanel;
};

#endif // FRACTUREVORONOISETTINGSPANEL_H
