#ifndef DEFAULTDAMAGEPANEL_H
#define DEFAULTDAMAGEPANEL_H

#include <QtWidgets/QWidget>
#include "BlastSceneTree.h"

namespace Ui {
class DefaultDamagePanel;
}

class DefaultDamagePanel : public QWidget, public ISceneObserver
{
    Q_OBJECT

public:
    explicit DefaultDamagePanel(QWidget *parent = 0);
    ~DefaultDamagePanel();
	void updateValues();

	virtual void dataSelected(std::vector<BlastNode*> selections);

private slots:
    void on_spinBoxMinRadius_valueChanged(double arg1);

    void on_spinBoxMaxRadius_valueChanged(double arg1);

    void on_checkBoxMaxRadius_stateChanged(int arg1);

    void on_comboBoxFallOff_currentIndexChanged(int index);

    void on_spinBoxMaxChunkSpeed_valueChanged(double arg1);

private:
    Ui::DefaultDamagePanel *ui;
	std::vector<BPPAsset*> _selectedAssets;
};

#endif // DEFAULTDAMAGEPANEL_H
