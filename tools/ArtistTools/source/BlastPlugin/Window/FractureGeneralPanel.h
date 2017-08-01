#ifndef FRACTUREGENERALPANEL_H
#define FRACTUREGENERALPANEL_H

#include <QtWidgets/QWidget>
#include "ProjectParams.h"

namespace Ui {
class FractureGeneralPanel;
}

class FractureVoronoiSettingsPanel;
class FractureSliceSettingsPanel;
class FractureVisualizersPanel;
class ExpandablePanel;

class FractureGeneralPanel : public QWidget
{
    Q_OBJECT

public:
	static FractureGeneralPanel* ins();

    explicit FractureGeneralPanel(QWidget *parent = 0);
    ~FractureGeneralPanel();
	void updateValues();
	void setFracturePanels(FractureVoronoiSettingsPanel* voronoiPanel, FractureSliceSettingsPanel* slicePanel, FractureVisualizersPanel* visulizersPanel);
	void setFractureExpandablePanels(ExpandablePanel* voronoiPanel, ExpandablePanel* slicePanel);

	FracturePreset* getCurrentFracturePreset();

private slots:
    void on_comboBoxFracturePreset_currentIndexChanged(int index);

	void on_btnAddPreset_clicked();

	void on_btnModifyPreset_clicked();

	void on_btnSavePreset_clicked();

    void on_comboBoxFractureType_currentIndexChanged(int index);

    void on_comboBoxApplyMaterial_currentIndexChanged(int index);
	
	void on_checkBoxAutoSelectNewChunks_stateChanged(int arg1);

private:
	FracturePreset* _getFracturePreset(const char* name);
	void _updateFractureUIs();
	void _showFracturePanel(const QString& fractireType);

private:
    Ui::FractureGeneralPanel		*ui;
	bool							m_bValid;
	bool							_updateData;
	FractureVoronoiSettingsPanel*	_voronoiPanel;
	FractureSliceSettingsPanel*		_slicePanel;
	FractureVisualizersPanel*		_visualizersPanel;
	ExpandablePanel*				_fractureVoronoiSettingsExpandlePanel;
	ExpandablePanel*				_fractureSliceSettingsExpandlePanel;
};

#endif // FRACTUREGENERALPANEL_H
