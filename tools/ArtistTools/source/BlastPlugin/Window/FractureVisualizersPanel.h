#ifndef FRACTUREVISUALIZERSPANEL_H
#define FRACTUREVISUALIZERSPANEL_H

#include <QtWidgets/QWidget>
#include "ProjectParams.h"

namespace Ui {
class FractureVisualizersPanel;
}
class FractureGeneralPanel;

class FractureVisualizersPanel : public QWidget
{
    Q_OBJECT

public:
    explicit FractureVisualizersPanel(QWidget *parent = 0);
    ~FractureVisualizersPanel();
	void updateValues();
	void setFractureGeneralPanel(FractureGeneralPanel* generalPanel) { _generalPanel = generalPanel; }

private slots:
/*
    void on_checkBoxFracturePreview_stateChanged(int arg1);

    void on_checkBoxDisplayFractureWidget_stateChanged(int arg1);
*/	
	void on_checkBoxSelectionDepthTest_stateChanged(int arg1);

private:
	BPPFractureVisualization* _getBPPVisualization();
private:
    Ui::FractureVisualizersPanel		*ui;
	bool								_updateData;
	FractureGeneralPanel*				_generalPanel;
};

#endif // FRACTUREVISUALIZERSPANEL_H
