#ifndef FRACTUREVISUALIZERSPANEL_H
#define FRACTUREVISUALIZERSPANEL_H

#include <QtWidgets/QWidget>

namespace Ui {
class FractureVisualizersPanel;
}

class FractureVisualizersPanel : public QWidget
{
    Q_OBJECT

public:
    explicit FractureVisualizersPanel(QWidget *parent = 0);
    ~FractureVisualizersPanel();
	void updateValues();

private slots:
    void on_checkBoxFracturePreview_stateChanged(int arg1);

    void on_checkBoxDisplayFractureWidget_stateChanged(int arg1);

private:
    Ui::FractureVisualizersPanel *ui;
};

#endif // FRACTUREVISUALIZERSPANEL_H
