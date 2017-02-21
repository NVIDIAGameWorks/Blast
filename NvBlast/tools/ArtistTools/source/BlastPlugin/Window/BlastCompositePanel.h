#ifndef BLASTCOMPOSITEPANEL_H
#define BLASTCOMPOSITEPANEL_H

#include <QtWidgets/QWidget>

namespace Ui {
class BlastCompositePanel;
}

class BlastCompositePanel : public QWidget
{
    Q_OBJECT

public:
    explicit BlastCompositePanel(QWidget *parent = 0);
    ~BlastCompositePanel();
	void updateValues();

private slots:
    void on_btnCollapse_clicked();

    void on_btnSave_clicked();

    void on_comboBoxAsset_currentIndexChanged(int index);

    void on_btnAddAsset_clicked();

    void on_btnRemoveAsset_clicked();

    void on_listWidgetBlastAsset_itemSelectionChanged();

    void on_spinBoxBondByThreshold_valueChanged(double arg1);

    void on_spinBoxNewBondStrength_valueChanged(double arg1);

	void on_btnAddLandmark_clicked();

	void on_btnModifyLandmark_clicked();

	void on_btnRemoveLandmark_clicked();

	void on_listWidgetLandmark_itemSelectionChanged();

	void on_checkBoxEnableLandmark_stateChanged(int arg1);

    void on_spinBoxLandmarkRadius_valueChanged(double arg1);

private:
	void _updateAssetComboBox();
	void _updateAssetInstanceListWidget();
	void _updateLandmarkListWidget();
	void _updateLandmarkUIs();

private:
    Ui::BlastCompositePanel *ui;
	int						_selectedAsset;
};

#endif // BLASTCOMPOSITEPANEL_H
